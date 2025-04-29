#include <common.h>
#include <allocate.h>
#include <scir.h>
#include <debug.h>
#include <defines.h>
#include <emit/mips.h>

typedef void (*VmFunc)();

typedef struct {
    u32 *function_memory;
    u16 *op_register_assignments;
    u16 *op_stack_assignments;
    u16 *save_register_stack_assignments;
    usize *current_instruction;
    usize *stack_offset;
} VmCompileState;

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

/*
    Calculate the order that each operation in `block` should be executed in machine code, and place the result in `op_index_order_array`.
    `op_highest_dependent_array` is the last operation that depends on a given operation.
    `op_code_last_index` points to the last instruction of the block.
    `op_code_order_start` is the first index that will be assigned in the order array (set to 0).
*/
static void vmCompileScirBlockOpReorder(ScirBlock *block, u16 *op_index_order_array, u16 *op_highest_dependent_array, u16 op_code_last_index, u16 *op_code_order_start) {
    u16 op_use_count = block->op_use_array_elements[op_code_last_index];
    u16 op_use_offset = block->op_use_arrays[op_code_last_index];
    u16 *op_use_array = &block->use_array[op_use_offset];

    if (op_use_count > 0) {
        u16 sorted_op_uses[op_use_count];

        memset(sorted_op_uses, SC_VM_UNDEFINED_, sizeof(sorted_op_uses));

        switch (op_use_count) {
            case 0: break;
            case 1: {
                sorted_op_uses[0] = op_use_array[0];
                break;
            }
            case 2: {
                if (op_use_array[0] > op_use_array[1]) {
                    sorted_op_uses[0] = op_use_array[1];
                    sorted_op_uses[1] = op_use_array[0];
                } else {
                    sorted_op_uses[0] = op_use_array[0];
                    sorted_op_uses[1] = op_use_array[1];
                }
                break;
            }
            default: {
                sorted_op_uses[0] = op_use_array[0];

                for (usize i = 1; i < op_use_count; i += 1) {
                    u16 current_use = op_use_array[i];
                    
                    for (usize j = i; j > 0; j -= 1) {
                        if (sorted_op_uses[j - 1] < current_use) {
                            break;
                        } else {
                            sorted_op_uses[j] = sorted_op_uses[j - 1];
                            sorted_op_uses[j - 1] = current_use;
                        }
                    }
                }
            }
        }

        for (usize i = 0; i < op_use_count; i += 1) {
            op_highest_dependent_array[sorted_op_uses[i]] = op_code_last_index;
            // Only assign a location if op has not already been located.
            if (op_index_order_array[sorted_op_uses[i]] != SC_VM_UNDEFINED_) {
                continue;
            }

            vmCompileScirBlockOpReorder(block, op_index_order_array, op_highest_dependent_array, sorted_op_uses[i], op_code_order_start);
        }
    }

    op_index_order_array[op_code_last_index] = *op_code_order_start;
    *op_code_order_start += 1;
}

/*
    Calculate the degree (a unique position specifier) of each operation and place the result in `op_degree_array`.
    `op_index_order_array` contains the placement of each operation in the finish machine code.
    `op_highest_dependent_array` holds the last time each operation is used.
    `op_code_array_elements` is the number of operations in the block.
*/
static void vmCompileDegreeCalculate(u16 *op_degree_array, u16 *op_index_order_array, u16 *op_highest_dependent_array, u16 op_code_array_elements) {
    u16 op_reordered_array[op_code_array_elements];

    // Put the operations in the order specified.
    for (usize i = 0; i < op_code_array_elements; i += 1) {
        op_reordered_array[op_index_order_array[i]] = i;
    }

    for (usize current_op_index = 0; current_op_index < op_code_array_elements; current_op_index += 1) {
        u16 current_op = op_reordered_array[current_op_index];
        u16 current_op_degree = op_degree_array[current_op];
        u16 compare_op_initial = op_highest_dependent_array[current_op];

        if (compare_op_initial == SC_VM_UNDEFINED_) {
            compare_op_initial = op_code_array_elements - 1;
        }

        u16 compare_op_index = op_index_order_array[compare_op_initial] - 1;

        while (compare_op_index > current_op_index) {
            u16 *compare_op_degree = &op_degree_array[op_reordered_array[compare_op_index]];

            *compare_op_degree = current_op_degree + 1;
            compare_op_index -= 1;
        }
    }
}

/*
    Obtain a register index based on the `use_op_index`, and spill the operation at `spill_op_index` if use operation is on the stack.
*/
static u16 vmCompileUseRegisterFetch(VmCompileState *state, u16 use_op_index, u16 spilled_op_index) {
    u16 op_register_assignment = state->op_register_assignments[use_op_index];

    if (op_register_assignment != SC_VM_UNDEFINED_) {
        return op_register_assignment;
    } else {
        u16 use_op_stack = state->op_stack_assignments[use_op_index];
        u16 spilled_op_register = state->op_register_assignments[spilled_op_index];

        state->op_stack_assignments[use_op_index] = spilled_op_register;
        state->op_register_assignments[spilled_op_index] = use_op_stack;

        state->function_memory[*state->current_instruction] = emitMipsADD(1, spilled_op_register, 0);
        state->function_memory[*state->current_instruction + 1] = emitMipsLW(spilled_op_register, use_op_stack, 30);
        state->function_memory[*state->current_instruction + 2] = emitMipsSW(1, use_op_stack, 30);

        *state->current_instruction += 3;

        return spilled_op_register;
    }
}

/*
    Obtain a register index based on `op_index` and `op_degree`, and spill `spilled_op` if no space is available.
*/
static u16 vmCompileOpRegisterFetch(VmCompileState *state, u16 op_index, u16 op_degree, u16 spilled_op) {
    // Assigns a hardware register based on the degree of the operation.
    if (op_degree < 18) {
        if (op_degree < 8) {
            state->op_register_assignments[op_index] = op_degree + 8;
        } else if (op_degree < 10) {
            state->op_register_assignments[op_index] = op_degree + 16;
        } else {
            state->op_register_assignments[op_index] = op_degree + 6;
        }

        if (op_degree >= 10) {
            state->function_memory[*state->current_instruction] = emitMipsSW(state->op_register_assignments[op_index], -*state->stack_offset, 30);
            state->save_register_stack_assignments[state->op_register_assignments[op_index] - 16] = *state->stack_offset;
            *state->stack_offset += 4;
            *state->current_instruction += 1;
        }
    } else {
        u16 spilled_op_register = state->op_register_assignments[spilled_op];

        state->function_memory[*state->current_instruction] = emitMipsSW(state->op_register_assignments[spilled_op], -*state->stack_offset, 30);

        state->op_register_assignments[op_index] = spilled_op_register;
        state->op_register_assignments[spilled_op] = SC_VM_UNDEFINED_;
        state->op_stack_assignments[spilled_op] = *state->stack_offset;

        *state->stack_offset += 4;
        *state->current_instruction += 1;
    }

    return state->op_register_assignments[op_index];
}

void vmCompileScirBlock(ScirBlock *block, u16 op_code_array_elements) {
    debugBlock(
        debugAssert(block != NULL, "Block is null!", NULL);
        debugAssert(op_code_array_elements > 0, "No operations in block!", NULL);
    )

    u16 op_index_order_array[op_code_array_elements];
    u16 op_highest_dependent_array[op_code_array_elements];
    u16 op_degree_array[op_code_array_elements];
    
    memset(op_index_order_array, SC_VM_UNDEFINED_, sizeof(op_index_order_array));
    memset(op_highest_dependent_array, SC_VM_UNDEFINED_, sizeof(op_highest_dependent_array));
    memset(op_degree_array, 0, sizeof(op_degree_array));

    vmCompileScirBlockOpReorder(block, op_index_order_array, op_highest_dependent_array, op_code_array_elements - 1, &(u16){0});
    vmCompileDegreeCalculate(op_degree_array, op_index_order_array, op_highest_dependent_array, op_code_array_elements);

    u16 op_reordered_array[op_code_array_elements];

    for (usize i = 0; i < op_code_array_elements; ++i) {
        op_reordered_array[op_index_order_array[i]] = i;
    }

    sc_printf("Degrees:\n");
    for (usize i = 0; i < op_code_array_elements; ++i) {
        sc_printf("[%3d]:", op_reordered_array[i]);
        for (usize j = 0; j < op_degree_array[op_reordered_array[i]] && j < 20; ++j) {
            sc_printf("-");
        }
        sc_printf("%d\n", op_degree_array[op_reordered_array[i]]);
    }

    const usize save_register_count = 8;

    u32 function_memory[256];
    u16 op_register_assignments[op_code_array_elements];
    u16 op_stack_assignments[op_code_array_elements];
    u16 save_register_stack_assignments[save_register_count]; // $s0 through $s7 must be returned by the end of the function.
    u16 degree_operation_indices[op_code_array_elements];

    memset(op_register_assignments, SC_VM_UNDEFINED_, sizeof(op_register_assignments));
    memset(op_stack_assignments, SC_VM_UNDEFINED_, sizeof(op_stack_assignments));
    memset(save_register_stack_assignments, SC_VM_UNDEFINED_, sizeof(save_register_stack_assignments));
    memset(degree_operation_indices, SC_VM_UNDEFINED_, sizeof(degree_operation_indices));

    usize current_instruction = 0;
    usize stack_offset = 0;

    VmCompileState compile_state = {
        .function_memory = function_memory,
        .op_register_assignments = op_register_assignments,
        .op_stack_assignments = op_stack_assignments,
        .save_register_stack_assignments = save_register_stack_assignments,
        .stack_offset = &stack_offset,
        .current_instruction = &current_instruction,
    };

    for (usize current_op = 0; current_op < op_code_array_elements && current_instruction < 256; current_op += 1) {
        u16 op_block_index = op_reordered_array[current_op];
        u16 op_degree = op_degree_array[op_block_index];

        u16 op_use_array_offset = block->op_use_arrays[op_block_index];
        u16 *op_use_array = block->use_array + op_use_array_offset;

        u16 op_const_array_offset = block->op_const_arrays[op_block_index];
        u32 *op_const_array = block->const_array + op_const_array_offset;

        degree_operation_indices[op_degree] = op_block_index;

        switch (block->op_code_array[op_block_index]) {
            case SCIR_OP_CODE_LOADIMMI: {
                u32 op_const = op_const_array[0];
                u16 op_register = vmCompileOpRegisterFetch(&compile_state, op_block_index, op_degree, degree_operation_indices[0]);

                if (op_const <= 0xFFFF) {
                    function_memory[current_instruction] = emitMipsORI(op_register, 0, op_const);
                    current_instruction += 1;
                } else {
                    function_memory[current_instruction] = emitMipsLUI(1, op_const >> 16);
                    function_memory[current_instruction + 1] = emitMipsORI(op_register, 1, op_const);
                    current_instruction += 2;                    
                }
                break;
            }
            case SCIR_OP_CODE_STORE: {
                u32 op_const = op_const_array[0];
                u16 op_use_register0 = vmCompileUseRegisterFetch(&compile_state, op_use_array[0], degree_operation_indices[0]);
                u16 op_use_register1 = vmCompileUseRegisterFetch(&compile_state, op_use_array[1], degree_operation_indices[1]);

                if (op_const <= 0xFFFF) {
                    function_memory[current_instruction] = emitMipsSW(op_use_register0, op_const, op_use_register1);
                    current_instruction += 1;
                } else {
                    function_memory[current_instruction] = emitMipsLUI(1, op_const >> 16);
                    function_memory[current_instruction + 1] = emitMipsADD(1, 1, op_use_register1);
                    function_memory[current_instruction + 2] = emitMipsSW(op_use_register0, op_const, 1);
                    current_instruction += 3;
                }
                break;
            }
            case SCIR_OP_CODE_ADDI: {
                u16 op_use_register0 = vmCompileUseRegisterFetch(&compile_state, op_use_array[0], degree_operation_indices[0]);
                u16 op_use_register1 = vmCompileUseRegisterFetch(&compile_state, op_use_array[1], degree_operation_indices[1]);
                u16 op_register = vmCompileOpRegisterFetch(&compile_state,op_block_index, op_degree, degree_operation_indices[2]);

                function_memory[current_instruction] = emitMipsADD(op_register, op_use_register0, op_use_register1);
                current_instruction += 1;
                break;
            }
            case SCIR_OP_CODE_ADDIMMI: {
                u32 op_const = op_const_array[0];
                u16 op_use_register = vmCompileUseRegisterFetch(&compile_state, op_use_array[0], op_register_assignments[degree_operation_indices[0]]);
                u16 op_register = vmCompileOpRegisterFetch(&compile_state, op_block_index, op_degree, degree_operation_indices[1]);

                if (op_const <= 0xFFFF) {
                    function_memory[current_instruction] = emitMipsADDI(op_register, op_use_register, op_const);
                    current_instruction += 1;
                } else {
                    function_memory[current_instruction] = emitMipsLUI(1, op_const >> 16);
                    function_memory[current_instruction + 1] = emitMipsORI(1, 1, op_const);
                    function_memory[current_instruction + 2] = emitMipsADD(op_register, op_use_register, 1);
                    current_instruction += 3;
                }
                break;
            }
        }
    }

    // Restore callee save registers if any were used.
    for (usize i = 0; i < save_register_count; i += 1) {
        u16 stack_assignment = save_register_stack_assignments[i];
        if (stack_assignment != SC_VM_UNDEFINED_) {
            function_memory[current_instruction] = emitMipsLW(16 + i, -stack_assignment, 30);
            current_instruction += 1;
        }
    }

    // Don't forget to return!
    // Branch delay slot gets a NOP for now.
    function_memory[current_instruction] = emitMipsJR(31);
    function_memory[current_instruction + 1] = 0;

    current_instruction += 2;

    #ifdef SC_PLATFORM_PSP_OPTION_

    vmCompileExecutableSet((uptr)function_memory, (uptr)function_memory + sizeof(function_memory));
    ((VmFunc)function_memory)();

    #endif
}