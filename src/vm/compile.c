#include <common.h>
#include <allocate.h>
#include <scir.h>
#include <debug.h>
#include <defines.h>

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

    u16 block_op_degree_array[block_op_array_elements];

    // Zero out array so starting degrees can be accurate
    memset(block_op_degree_array, 0, block_op_array_elements * sizeof(u16));

    for (usize i = 0; i < block_op_array_elements; ++i) {
        // Current op and the op being compared against.
        u16 op_degree = block_op_degree_array[i];
        usize compare_op_index = block->op_lifetime_array[i] - 1;
        
        // Bump the register index for each interfering operation.
        // (> 1) and (i - 1) are to prevent incrementing the current op and the last op in the lifetime.
        while (compare_op_index > i) {
            // Different types use different register sets, so they don't interfere.
            u16 *compare_op_degree = block_op_degree_array + compare_op_index;

            *compare_op_degree += (*compare_op_degree >= op_degree);
            compare_op_index--;
        }
        // Died so young...
    }

    sc_printf("Degrees:\n");
    for (usize i = 0; i < block_op_array_elements; ++i) {
        sc_printf("%d\n", block_op_degree_array[i]);
    }

    // To store the function itself
    // Temp solution, will replace later.
    u32 function_memory[256];

    ScirOp *op_array = block->op_array;

    usize current_function = 0;

    for (usize i = 0, imm = 0; i < block_op_array_elements; ++i) {
        ScirOp op = op_array[i];
        u16 *use_array = block->use_array + op.use_array;

        switch ((ScirOpCode)(op.code & ~0xF800)) {
            case SCIR_OP_CODE_IMMLOAD: {
                u32 const_value = block->const_array[use_array[0]];

                if (const_value <= 0xFFFF) {
                    function_memory[current_function] = 0b001101 << 26 | 0 << 21 | (9 + block_op_degree_array[i]) << 16 | const_value;
                    current_function++;
                } else {
                    function_memory[current_function] = 0b001111 << 26 | 0 << 21 | 1 << 16 | const_value >> 16;
                    function_memory[current_function + 1] = 0b001101 << 26 | 1 << 21 | (9 + block_op_degree_array[i]) << 16 | (const_value & 0xFFFF);
                    current_function += 2;
                }
                
                imm++;
                continue;
            }
            case SCIR_OP_CODE_STORE: {
                u32 const_value = block->const_array[use_array[0]];
                u16 operand_degrees[2] = {block_op_degree_array[use_array[1]], block_op_degree_array[use_array[2]]};

                if (const_value <= 0xFFFF) {
                    function_memory[current_function] = 0b101011 << 26 | (9 + operand_degrees[0]) << 21 | (9 + operand_degrees[1]) << 16 | const_value;
                    current_function++;
                }

                continue;
            }
            case SCIR_OP_CODE_ADDI: {
                u16 operand_degrees[2] = {block_op_degree_array[use_array[0]], block_op_degree_array[use_array[1]]};

                function_memory[current_function] = 0b000000 << 26 
                | (9 + operand_degrees[0]) << 21 
                | (9 + operand_degrees[1]) << 16 
                | (9 + block_op_degree_array[i]) << 11 
                | 0 << 6
                | 0b100000;

                // Dumb way of seeing the result of the last add (ADDIU $v0, [add_register], 0).
                // function_memory[current_function + 1] = 0b001001 << 26 | (9 + block_op_degree_array[i]) << 21 | 2 << 16 | 0;
                current_function ++;

                continue;
            }
            default:
                continue;
        }
    }

    function_memory[current_function] = 0b000000 << 26 | 31 << 21 | 0 << 6 | 0b001000;
    function_memory[current_function + 1] = 0;

    current_function += 2;

    for (usize i = 0; i < (current_function); ++i) {
        sc_printf("%08x\n", function_memory[i]);
    }

    #ifdef SC_PLATFORM_PSP_OPTION_

    vmCompileExecutableSet((uptr)function_memory, (uptr)function_memory + sizeof(function_memory));
    ((VmFunc)function_memory)();

    #endif
}