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
    SCIR_OP_CODE_LOADIMMI,
    SCIR_OP_CODE_LINKOUT,
    SCIR_OP_CODE_LINKIN,
    SCIR_OP_CODE_STORE,
    SCIR_OP_CODE_ADDI,
    SCIR_OP_CODE_ADDF,
    SCIR_OP_CODE_ADDIMMI,
    SCIR_OP_CODE_ADDIMMF,
} ScirOpCode;

typedef struct {
    u16 *op_code_array;
    u16 *op_use_arrays;
    u16 *op_const_arrays;
    u16 *op_use_array_elements;
    u16 *op_const_array_elements;
    u16 *use_array;
    u32 *const_array;
} ScirBlock;

// Initialize with `scirOpBlockAppendStateBind`.
typedef struct {
    ScirBlock *block;
    u16 op_code_array_position;
    u16 use_array_position;
    u16 const_array_position;
} ScirBlockAppendState;

static inline ScirBlockAppendState scirBlockAppendStateBind(ScirBlock *block) {
    return (ScirBlockAppendState){
        .block = block,
        .op_code_array_position = 0,
        .use_array_position = 0,
        .const_array_position = 0,
    };
}

static inline void scirBlockAppendStateReset(ScirBlockAppendState *state) {
    state->op_code_array_position = 0;
    state->use_array_position = 0;
    state->const_array_position = 0;
}

/*
    Allocate a new ScirBlock.
    Memory allocation byte total: `op_array_size` + `op_array_size >> 1` + `use_array_size` + `const_array_size`
*/
ScirBlock scirBlockAllocate(AllocateBuffer *buffer, usize op_array_max_elements, usize use_array_max_elements, usize const_array_max_elements);

/*
    Append an operation with the code `code` and `use_array_elements` elements in `use_array` to the block bound to `state`.
*/
u16 scirBlockOpAppend(ScirBlockAppendState *state, ScirOpCode code, u16 *use_array, u16 use_array_elements, u32 *const_array, u16 const_array_elements);

static inline u16 scirBlockOpAppendAddi(ScirBlockAppendState *state, u16 op0, u16 op1) {
    return scirBlockOpAppend(state, SCIR_OP_CODE_ADDI, (u16[]){op0, op1}, 2, NULL, 0);
}

static inline u16 scirBlockOpAppendAddimmi(ScirBlockAppendState *state, u16 op0, i32 immediate) {
    return scirBlockOpAppend(state, SCIR_OP_CODE_ADDIMMI, (u16[]){op0}, 1, (u32[]){immediate}, 1);
}

static inline u16 scirBlockOpAppendLoadimmi(ScirBlockAppendState *state, u32 immediate) {
    return scirBlockOpAppend(state, SCIR_OP_CODE_LOADIMMI, NULL, 0, (u32[]){immediate}, 1);
}

static inline u16 scirBlockOpAppendStore(ScirBlockAppendState *state, u16 op0, u16 op1, i32 offset) {
    return scirBlockOpAppend(state, SCIR_OP_CODE_STORE, (u16[]){op0, op1}, 2, (u32[]){offset}, 1);
}

#endif