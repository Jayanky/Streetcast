#ifndef SC_VM_H_
#define SC_VM_H_

#include <common.h>
#include <scir.h>

/* 
    Compiles Scir Block `block` to MIPS machine code.
*/
void vmCompileScirBlock(ScirBlock *block);

#endif