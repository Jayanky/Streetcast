#ifndef STREETCAST_VM_H_
#define STREETCAST_VM_H_

#include <common.h>
#include <scir.h>

/* 
    Compiles Scir Block `block` to MIPS machine code.
    `block_op_array_elements` should be set to the number of operations.
    `block_op_degree_array` should have the same number of elements as the number of operations.
*/
void vmCompileScirBlock(ScirBlock *block, u16 *block_op_degree_array, u16 block_op_array_elements);

#endif