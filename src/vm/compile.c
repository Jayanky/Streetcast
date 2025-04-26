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

static void vmCompileScirBlockOpReorder(ScirBlock *block, u16 *op_ordered_index_array, u16 *op_highest_dependent_array, u16 op_code_last_index, u16 *op_code_order_start) {
    u16 op_use_count = block->op_use_array_elements[op_code_last_index];
    u16 op_use_offset = block->op_use_arrays[op_code_last_index];
    u16 *op_use_array = &block->use_array[op_use_offset];
    u16 use_use_count = 0;

    for (usize i = 0; i < op_use_count; i += 1) {
        // Only assign a location if op has not already been located.
        if (op_ordered_index_array[op_use_array[i]] != (u16)-1) {
            continue;
        }

        use_use_count = block->op_use_array_elements[op_use_array[i]];

        if (use_use_count > 0) {
            vmCompileScirBlockOpReorder(block, op_ordered_index_array, op_highest_dependent_array, op_use_array[i], op_code_order_start);
        }
    }

    for (usize i = 0; i < op_use_count; i += 1) {
        if (op_ordered_index_array[op_use_array[i]] != (u16)-1) {
            continue;
        }

        use_use_count = block->op_use_array_elements[op_use_array[i]];

        if (use_use_count == 0) {
            vmCompileScirBlockOpReorder(block, op_ordered_index_array, op_highest_dependent_array, op_use_array[i], op_code_order_start);
        }
    }

    for (usize i = 0; i < op_use_count; i += 1) {
        // For some reason the loop this statement goes in matters.
        op_highest_dependent_array[op_use_array[i]] = op_code_last_index;
    }

    op_ordered_index_array[op_code_last_index] = *op_code_order_start;
    *op_code_order_start += 1;
}

static void vmCompileDegreeCalculate(u16 *op_degree_array, u16 *op_index_order_array, u16 *op_highest_dependent_array, u16 op_code_array_elements) {
    u16 op_reordered_array[op_code_array_elements];

    // Put the operations in the order specified.
    for (usize i = 0; i < op_code_array_elements; i += 1) {
        op_reordered_array[op_index_order_array[i]] = i;
    }

    for (usize current_op_index = 0; current_op_index < op_code_array_elements; ++current_op_index) {
        u16 current_op = op_reordered_array[current_op_index];
        u16 current_op_degree = op_degree_array[current_op];
        u16 compare_op_initial = op_highest_dependent_array[current_op];

        if (compare_op_initial == (u16)-1) {
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

void vmCompileScirBlock(ScirBlock *block, u16 op_code_array_elements) {
    debugBlock(
        debugAssert(op_code_array_elements > 0, "No operations in block!", NULL);
    )

    u16 op_index_order_array[op_code_array_elements];
    u16 op_highest_dependent_array[op_code_array_elements];
    u16 op_degree_array[op_code_array_elements];
    
    memset(op_index_order_array, -1, sizeof(op_index_order_array));
    memset(op_highest_dependent_array, -1, sizeof(op_highest_dependent_array));
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
        for (usize j = 0; j < op_degree_array[op_reordered_array[i]]; ++j) {
            sc_printf("-");
        }
        sc_printf("%d\n", op_degree_array[op_reordered_array[i]]);
    }

    u32 function_memory[256];
    usize current_instruction = 0;

    for (usize current_op = 0; current_op < op_code_array_elements && current_instruction < 256; current_op += 1) {
        u16 op_used = op_reordered_array[current_op];
        u16 op_degree = op_degree_array[op_used];

        u16 op_use_array_offset = block->op_use_arrays[op_used];
        u16 *op_use_array = block->use_array + op_use_array_offset;

        u16 op_const_array_offset = block->op_const_arrays[op_used];
        u32 *op_const_array = block->const_array + op_const_array_offset;

        switch (block->op_code_array[op_used]) {
            case SCIR_OP_CODE_LOADIMM: {
                u32 op_const = op_const_array[0];
                sc_printf("loadimm %zd, %d\n", current_op, op_const);

                function_memory[current_instruction] = emitMipsLUI(1, op_const >> 16);
                function_memory[current_instruction + 1] = emitMipsORI(op_degree + 8, 1, op_const & 0xFFFF);
                current_instruction += 2;
                break;
            }
            case SCIR_OP_CODE_STORE: {
                u16 op_use = op_use_array[0];
                u32 op_const = op_const_array[0];
                sc_printf("store %zd, %p\n", current_op, (void*)(uptr)op_const);

                function_memory[current_instruction] = emitMipsLUI(1, op_const >> 16);
                function_memory[current_instruction + 1] = emitMipsORI(1, 1, op_const & 0xFFFF);
                function_memory[current_instruction + 2] = emitMipsSW(op_degree_array[op_use] + 8, 0, 1);
                current_instruction += 3;
                break;
            }
            case SCIR_OP_CODE_ADDI: {
                u16 op_uses[2] = {op_use_array[0], op_use_array[1]};
                sc_printf("addi %zd, %d, %d\n", current_op, op_index_order_array[op_uses[0]], op_index_order_array[op_uses[1]]);

                function_memory[current_instruction] = emitMipsADD(op_degree + 8, op_degree_array[op_uses[0]] + 8, op_degree_array[op_uses[1]] + 8);
                current_instruction += 1;
                break;
            }
        }
    }

    function_memory[current_instruction] = emitMipsJR(31);
    function_memory[current_instruction + 1] = 0;
    
    for (usize i = 0; i < current_instruction; i += 1) {
        sc_printf("[%3zd]: %08x\n", i, function_memory[i]);
    }

    #ifdef SC_PLATFORM_PSP_OPTION_

    vmCompileExecutableSet((uptr)function_memory, (uptr)function_memory + sizeof(function_memory));
    ((VmFunc)function_memory)();

    #endif
}