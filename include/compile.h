#ifndef SC_COMPILE_H_
#define SC_COMPILE_H_

#include <common.h>
#include <call_convention.h>

typedef void (*CompileFunc)();

/*
    For `block`:
    Get the ordering of instruction execution output to `out_op_order_index_array`.
    Get the last time each operation is used output to `out_op_last_used_array`.
*/
void compileScirBlockInfoCalculate(ScirBlock *block, u16 *out_op_ordered_array, u16 *out_op_last_used_array);

/*
    Get the unique position each operation in `block` must occupy in the order of `op_ordered_array` to not conflict.
    `op_code_set_array` specifies the register set of each possible operation (i.e. int and float operations normally use different registers).
*/
void compileScirBlockDegreeCalculate(ScirBlock *block, u16 *op_ordered_array, u16 *op_last_used_array, u16 *op_code_set_array, u16 *out_op_degree_array);


#endif