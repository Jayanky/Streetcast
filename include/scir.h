#ifndef SC_SCIR_H_
#define SC_SCIR_H_

#include <common.h>
#include <allocate.h>

typedef enum {
    SCIR_OP_WIDTH_8 = 0x0000,
    SCIR_OP_WIDTH_16 = 0x4000,
    SCIR_OP_WIDTH_32 = 0x8000,
    SCIR_OP_WIDTH_64 = 0xC000,
} ScirOpWidth;

typedef enum {
    SCIR_OP_CODE_LOAD,
    SCIR_OP_CODE_IMMLOAD,
    SCIR_OP_CODE_OUTLINK,
    SCIR_OP_CODE_INLINK,
    SCIR_OP_CODE_STORE,
    SCIR_OP_CODE_ADDI,
    SCIR_OP_CODE_ADDF,
    SCIR_OP_CODE_IMMADDI,
    SCIR_OP_CODE_IMMADDF,
} ScirOpCode;

typedef struct {
    u16 code;                       // ScirOpCode | ScirOpType | ScirOpWidth
    u16 use_array;                  // The indices this operation depends on.
} ScirOp;

typedef struct {
    ScirOp *op_array;
    u16 *op_lifetime_array;
    u16 *use_array;
    u32 *const_array;
    u32 op_array_capacity;
    u32 use_array_capacity;
    u32 const_array_capacity;
} ScirBlock;

// Initialize with `scirOpBlockAppendStateBind`.
typedef struct {
    ScirBlock *block;
    ScirOp *op_array_position;
    u16 *op_lifetime_array_position;
    u16 *use_array_position;
    u32 *const_array_position;
} ScirBlockAppendState;

inline ScirBlockAppendState scirBlockAppendStateBind(ScirBlock *block) {
    return (ScirBlockAppendState){
        .block = block,
        .op_array_position = block->op_array,
        .op_lifetime_array_position = block->op_lifetime_array,
        .use_array_position = block->use_array,
        .const_array_position = block->const_array,
    };
}

inline void scirBlockAppendStateReset(ScirBlockAppendState *state) {
    state->op_array_position = state->block->op_array;
    state->op_lifetime_array_position = state->block->op_lifetime_array,
    state->use_array_position = state->block->use_array;
    state->const_array_position = state->block->const_array;
}

/* Get the number of operations in the block bound to `state` */
inline usize scirBlockAppendStateOpElements(ScirBlockAppendState *state) {
    return state->op_array_position - state->block->op_array;
}

/* 
    Get the number operation lifetimes in the block bound to `state`.
    This number should be the same as the operation count.
*/
inline usize scirBlockAppendStateOpLifetimeElements(ScirBlockAppendState *state) {
    return state->op_lifetime_array_position - state->block->op_lifetime_array;
}

/* Get the number of uses in the block bound to `state` */
inline usize scirBlockAppendStateUseElements(ScirBlockAppendState *state) {
    return state->use_array_position - state->block->use_array;
}

/* Get the number of constants in the block bound to `state` */
inline usize scirBlockAppendStateConstElements(ScirBlockAppendState *state) {
    return state->const_array_position - state->block->const_array;
}

/*
    Allocate a new ScirBlock.
    Memory allocation byte total: `op_array_size` + `op_array_size >> 1` + `use_array_size` + `const_array_size`
*/
ScirBlock scirBlockAllocate(AllocateBuffer *buffer, usize op_array_size, usize use_array_size, usize const_array_size);

/*
    Determine the degree that each operation in `block` occupies, and place the results in `buffer`.
    `block_op_array_elements` should be set to the number of operations in `block`.
*/
void scirBlockOpDegreeDetermine(ScirBlock *block, AllocateBuffer *buffer, u16 block_op_array_elements);

/*
    Append an operation with the code `code` and `use_array_elements` elements in `use_array` to the block bound to `state`.
*/
u16 scirBlockOpAppend(ScirBlockAppendState *state, ScirOpCode code, u16 *use_array, u16 use_array_elements);

/*
    Append `const_array_elements` elements in `const_array` to the block bound to `state`.
*/
u16 scirBlockConstAppend(ScirBlockAppendState *state, u32 *const_array, u16 const_array_elements);

#endif