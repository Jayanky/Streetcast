#include <common.h>
#include <allocate.h>
#include <scir.h>
#include <debug.h>
#include <defines.h>
#include <emit/mips.h>

typedef void (*VmFunc)();

// I've tried my best and I still only have a vague idea of what this cache stuff actually does.
// But it does indeed.
static inline void vmCompileExecutableSet(uptr address_start, uptr address_end) {
    debugBlock(
        debugAssert(address_start <= address_end, "Start address %zd less than end address %zd!", address_start, address_end);
    )

	address_start = address_start & ~0b111111;
	address_end = (address_end + 0b111111) & ~0b111111;

    for (usize address = address_start; address < address_end; address += 64) {
        __asm__ volatile (
            "cache 0b11010, 0(%[address])\n" // Tertiary Cache Hit Writeback
            "cache 0b01000, 0(%[address])\n" // Instruction Cache Store Tag
            :
            : [address] "r" (address)
        );
    }
}

void vmScirBlockCompile(ScirBlock *block, u16 block_op_array_elements) {
    debugBlock(
        debugAssert(block != NULL, "Block is null!", NULL);
        debugAssert(block->op_array != NULL, "Block op array is null!", NULL);
        debugAssert(block->op_lifetime_array != NULL, "Block op lifetime array is null!", NULL);
    )

    // Holds the degree of each operation.
    // Used to determine register placement.
    u16 block_op_degree_array[block_op_array_elements];

    // Holds which op lives the furthest after the given index.
    // Used to determine live registers to spill.
    u16 block_op_furthest_life_array[block_op_array_elements];

    // To store the function itself
    // Temp solution, will replace later.
    u32 function_memory[256];

    /* Calculate operation degrees. */

    // Zero out array so starting degrees can be accurate.
    memset(block_op_degree_array, 0, block_op_array_elements * sizeof(u16));

    for (usize i = 0; i < block_op_array_elements; ++i) {
        // Current op and the op being compared against.
        u16 op_degree = block_op_degree_array[i];
        usize compare_op_index = block->op_lifetime_array[i] - 1;
        
        // Bump the register index for each interfering operation.
        while (compare_op_index > i) {
            // Different types use different register sets, so they don't interfere.
            u16 *compare_op_degree = block_op_degree_array + compare_op_index;

            *compare_op_degree += (*compare_op_degree >= op_degree);
            compare_op_index--;
        }
        // Died so young...
    }

    /* Calculate longest lifetimes at each index. */

    block_op_furthest_life_array[0] = 0;

    {
        // Only needed for this calculation.
        u16 current_furthest_life = 0;

        for (usize i = 1; i < block_op_array_elements; ++i) {
            // All ones if true, all zeros if false.
            usize mask = !(block->op_lifetime_array[current_furthest_life] < block->op_lifetime_array[i]) - 1;

            // If condition is true, set the current op as having a longer lifetime than the previous furthest.
            current_furthest_life = (i & mask) | (current_furthest_life & ~mask);
            
            // Add resulting index to array.
            block_op_furthest_life_array[i] = current_furthest_life;

        }        
    }

    sc_printf("Furthest Lifetimes:\n");
    for (usize i = 0; i < block_op_array_elements; ++i) {
        sc_printf("[%zd]: %d\n", i, block_op_furthest_life_array[i]);
    }

    /* Compile the function itself. */
    ScirOp *op_array = block->op_array;

    usize current_function = 0;

    for (usize i = 0, imm = 0; i < block_op_array_elements; ++i) {
        ScirOp op = op_array[i];
        u16 *use_array = block->use_array + op.use_array;

        switch ((ScirOpCode)(op.code & ~0xF800)) {
            case SCIR_OP_CODE_LOADIMM: {
                u32 const_value = block->const_array[use_array[0]];

                if (const_value <= 0xFFFF) {
                    function_memory[current_function] = emitMipsORI(9 + block_op_degree_array[i], 0, const_value);
                    current_function++;
                } else {
                    function_memory[current_function] = emitMipsLUI(1, const_value >> 16);
                    function_memory[current_function + 1] = emitMipsORI(9 + block_op_degree_array[i], 1, const_value & 0xFFFF);
                    current_function += 2;
                }
                
                imm++;
                continue;
            }
            case SCIR_OP_CODE_STORE: {
                u32 const_value = block->const_array[use_array[0]];
                u16 operand_degrees[2] = {block_op_degree_array[use_array[1]], block_op_degree_array[use_array[2]]};

                if (const_value <= 0xFFFF) {
                    function_memory[current_function] = emitMipsSW(9 + operand_degrees[1], const_value, 9 + operand_degrees[0]);
                    current_function++;
                }

                continue;
            }
            case SCIR_OP_CODE_ADDI: {
                u16 operand_degrees[2] = {block_op_degree_array[use_array[0]], block_op_degree_array[use_array[1]]};

                function_memory[current_function] = emitMipsADD(9 + block_op_degree_array[i], 9 + operand_degrees[0], 9 + operand_degrees[1]);
                current_function++;

                continue;
            }
            default:
                continue;
        }
    }

    function_memory[current_function] = emitMipsJR(31);
    function_memory[current_function + 1] = 0;

    current_function += 2;

    #ifdef SC_PLATFORM_PSP_OPTION_

    vmCompileExecutableSet((uptr)function_memory, (uptr)function_memory + sizeof(function_memory));
    ((VmFunc)function_memory)();

    #endif
}