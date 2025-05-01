#include <compile.h>

#include <common.h>
#include <scir.h>

static void compileScirInfoCalculateRecurse(ScirBlock *block, u16 op_last_index, u16 *op_order_index_array, u16 *op_lifetime_array, u16 *op_order_index) {
    u16 *op_use_array = &block->use_array[block->op_use_arrays[op_last_index]];
    u16 op_use_count = block->op_use_array_elements[op_last_index];

    for (usize i = 0; i < op_use_count; i += 1) {
        compileScirInfoCalculateRecurse(block, op_use_array[i], op_order_index_array, op_lifetime_array, op_order_index);

        op_lifetime_array[op_use_array[i]] = op_last_index;
    }

    if (op_order_index_array[op_last_index] == (u16)-1) {
        op_order_index_array[op_last_index] = *op_order_index;
        *op_order_index += 1;        
    }
}

void compileScirInfoCalculate(ScirBlock *block, u16 *op_ordered_array, u16 *op_degree_ordered_array) {
    for (usize i = 0; i < block->op_code_array_elements; i += 1) {
        u16 *op_use_array = &block->use_array[block->op_use_arrays[i]];
        u16 op_use_count = block->op_use_array_elements[i];
        u16 sorted_use_array[op_use_count];

        memset(sorted_use_array, 0, sizeof(sorted_use_array));

        sorted_use_array[0] = op_use_array[0];

        for (usize use = 1; use < op_use_count; use += 1) {
            u16 current_use = op_use_array[use];

            for (isize compare = use - 1; compare >= -1; compare -= 1) {
                if (compare == -1) {
                    sorted_use_array[0] = current_use;
                    break;
                }
                if (sorted_use_array[compare] <= current_use) {
                    sorted_use_array[compare + 1] = current_use;
                    break;
                } else {
                    sorted_use_array[compare + 1] = sorted_use_array[compare];
                }
            }
        }

        for (usize use = 0; use < op_use_count; use += 1) {
            op_use_array[use] = sorted_use_array[use];
        }
    }

    u16 op_order_index_array[block->op_code_array_elements];
    u16 op_lifetime_array[block->op_code_array_elements];
    u16 op_lifetime_degree_subtraction_array[block->op_code_array_elements];
    u16 op_degree_array[block->op_code_array_elements];

    memset(op_order_index_array, -1, sizeof(op_order_index_array));
    memset(op_lifetime_degree_subtraction_array, 0, sizeof(op_lifetime_degree_subtraction_array));

    for (usize i = 0; i < block->op_code_array_elements; i += 1) {
        op_lifetime_array[i] = i;
    }

    compileScirInfoCalculateRecurse(block, block->op_code_array_elements - 1, op_order_index_array, op_lifetime_array, &(u16){0});
    
    for (usize i = 0; i < block->op_code_array_elements; i += 1) {
        for (usize j = op_lifetime_array[i]; j < block->op_code_array_elements; j += 1) {
            op_lifetime_degree_subtraction_array[j] += 1;
        }
    }

    for (usize i = 0; i < block->op_code_array_elements; i += 1) {
        op_degree_array[i] = op_order_index_array[i] - op_lifetime_degree_subtraction_array[i];
        
    }

    for (usize i = 0; i < block->op_code_array_elements; i += 1) {
        op_ordered_array[op_order_index_array[i]] = i;
        op_degree_ordered_array[op_order_index_array[i]] = op_degree_array[i];
    }
}