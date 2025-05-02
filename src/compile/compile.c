#include <compile.h>

#include <common.h>
#include <scir.h>

static void compileScirInfoCalculateRecurse(ScirBlock *block, u16 op_last_index, u16 *out_op_order_index_array, u16 *out_op_order_index) {
    u16 *op_use_array = &block->use_array[block->op_use_arrays[op_last_index]];
    u16 op_use_count = block->op_use_array_elements[op_last_index];

    for (usize i = 0; i < op_use_count; i += 1) {
        compileScirInfoCalculateRecurse(block, op_use_array[i], out_op_order_index_array, out_op_order_index);
    }

    if (out_op_order_index_array[op_last_index] == (u16)-1) {
        out_op_order_index_array[op_last_index] = *out_op_order_index;
        *out_op_order_index += 1;        
    }
}

void compileScirBlockInfoCalculate(ScirBlock *block, u16 *out_op_ordered_array, u16 *out_op_last_used_array) {
    for (usize i = 0; i < block->op_code_array_elements; i += 1) {
        // All operations that this operation depends on.
        u16 *op_use_array = &block->use_array[block->op_use_arrays[i]];

        // The number of operations this operation depends on.
        u16 op_use_count = block->op_use_array_elements[i];

        // Sorting in ascending order improves allocation.
        u16 *sorted_use_array = alloca(op_use_count * sizeof(*op_use_array));

        memset(sorted_use_array, 0, op_use_count * sizeof(*op_use_array));

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

    // For each operation, gives the order that it should be executed.
    u16 *op_order_index_array = alloca(block->op_code_array_elements * sizeof(*op_order_index_array));

    memset(op_order_index_array, -1, block->op_code_array_elements * sizeof(*op_order_index_array));
    memset(out_op_last_used_array, -1, block->op_code_array_elements * sizeof(*out_op_last_used_array));

    {
        u16 op_order_index = 0;

        for (isize i = block->op_code_array_elements - 1; i >= 0; i -= 1) {
            if (op_order_index_array[i] == (u16)-1) {
                compileScirInfoCalculateRecurse(block, i, op_order_index_array, &op_order_index);
            }
        }        
    }   

    for (usize i = 0; i < block->op_code_array_elements; i += 1) {
        out_op_ordered_array[op_order_index_array[i]] = i;
    }

    for (usize i = 0; i < block->op_code_array_elements; i += 1) {
        u16 current_op = out_op_ordered_array[i];
        u16 *op_use_array = &block->use_array[block->op_use_arrays[current_op]];
        u16 op_use_count = block->op_use_array_elements[current_op];

        for (usize j = 0; j < op_use_count; j += 1) {
            u16 use_order = op_order_index_array[op_use_array[j]];
            
            out_op_last_used_array[use_order] = i;
        }
    }
}

void compileScirBlockDegreeCalculate(ScirBlock *block, u16 *op_ordered_array, u16 *op_last_used_array, u16 *op_code_set_array, u16 *out_op_degree_array) {
    memset(out_op_degree_array, 0, block->op_code_array_elements * sizeof(*out_op_degree_array));

    for (usize i = 0; i < block->op_code_array_elements; i += 1) {
        u16 current_set = op_code_set_array[block->op_code_array[op_ordered_array[i]]];
        u16 current_degree = out_op_degree_array[i];
        u16 current_last_use = op_last_used_array[i];

        if (current_last_use != (u16)-1) {
            for (usize j = i + 1; j < current_last_use; j += 1) {
                u16 compare_set = op_code_set_array[block->op_code_array[op_ordered_array[j]]];
                u16 compare_degree = out_op_degree_array[j];
    
                if (compare_degree == current_degree && compare_set == current_set) {
                    out_op_degree_array[j] += 1;
                }
            }
        }
    }
}