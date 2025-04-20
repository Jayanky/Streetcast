#include <common.h>
#include <allocate.h>
#include <scir.h>

typedef i32 (*VmFunc)();

// I've tried my best and I still only have a vague idea of what this cache stuff actually does.
// But it does indeed.
static inline void vmFuncExecutableSet(void *address_start, void *address_end) {
	address_start = (void*)((uptr)address_start & ~0b111111);
	address_end = (void*)(((uptr)address_end + 0b111111) & ~0b111111);

    for (usize address = (uptr)address_start; address < (uptr)address_end; address += 64) {
        __asm__ volatile (
            "cache 0b11010, 0(%[address])\n" // Tertiary Cache Hit Writeback
            "cache 0b01000, 0(%[address])\n" // Instruction Cache Store Tag
            :
            : [address] "r" ((usize)address)
        );
    }
}

void vmCompileScirBlock(ScirBlock *block, u16 *block_op_degree_array, u16 block_op_array_elements) {
    u32 function_memory[256];
    u32 immediate_memory[128];

    sc_printf("Immediate Memory:\n%p\n", immediate_memory);

    ScirOp *op_array = block->op_array;

    usize current_function = 0;

    // Currently just loads the base of the immediate array for the function to use in $t0.
    function_memory[current_function] = 0b001111 << 26 | 0 << 21 | 8 << 16 | (((uptr)immediate_memory >> 16) & 0xFFFF);
    function_memory[current_function + 1] = 0b001101 << 26 | 8 << 21 | 8 << 16 | ((uptr)immediate_memory & 0xFFFF);

    current_function += 2;

    for (usize i = 0, imm = 0; i < block_op_array_elements; ++i) {
        ScirOp op = op_array[i];

        switch ((ScirOpCode)(op.code & ~0xF800)) {
            case SCIR_OP_CODE_IMMLOAD: {
                u16 const_array_index = (block->use_array + op.use_array)[0];
                immediate_memory[imm] = block->const_array[const_array_index];
                sc_printf("immediate_memory[%zd], %i\n", imm, immediate_memory[imm]);

                function_memory[current_function++] = 0b100011 << 26 | 8 << 21 | (9 + block_op_degree_array[i]) << 16 | imm << 2;

                imm++;
                continue;
            }
            case SCIR_OP_CODE_ADDI: {
                u16 *use_array = (block->use_array + op.use_array);
                u16 operand_degrees[2] = {block_op_degree_array[use_array[0]], block_op_degree_array[use_array[1]]};

                switch ((ScirOpType)(op.code & 0x3800)) {
                    // In the middle of detangling types from register sets, so this will probably change.
                    case SCIR_OP_TYPE_GENERAL: {
                        function_memory[current_function] = 0b000000 << 26 
                        | (9 + operand_degrees[0]) << 21 
                        | (9 + operand_degrees[1]) << 16 
                        | (9 + block_op_degree_array[i]) << 11 
                        | 0 << 6
                        | 0b100000;

                        // Dumb way of seeing the result of the last add (ADDIU $v0, [add_register], 0).
                        function_memory[current_function + 1] = 0b001001 << 26 | (9 + block_op_degree_array[i]) << 21 | 2 << 16 | 0;
                        current_function += 2;
                    }
                    default:
                        break;
                }

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

    #ifdef STREETCAST_PLATFORM_PSP_OPTION_

    vmFuncExecutableSet(&function_memory[0], function_memory + sizeof(function_memory));
    i32 val = ((VmFunc)&function_memory[0])();

    sc_printf("val:\n%i\n", val);

    #endif

}