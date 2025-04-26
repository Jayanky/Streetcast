#include <common.h>
#include <scir.h>
#include <allocate.h>
#include <debug.h>
#include <defines.h>

ScirBlock scirBlockAllocate(AllocateBuffer *buffer, usize op_array_max_elements, usize use_array_max_elements, usize const_array_max_elements) {
    debugBlock(
        debugAssert(buffer != NULL, "Buffer is null!", NULL);
        debugAssert(
            SC_SCIR_BLOCK_SIZE_INT_ <= buffer->capacity, 
            "Requested capacity of %zd larger than buffer capacity of %zd!", 
            SC_SCIR_BLOCK_SIZE_INT_, buffer->capacity
        );
    )

    AllocateBumpState allocate_state = allocateBufferBumpStateBind(buffer);

    return (ScirBlock) {
        .op_code_array = allocateBump(&allocate_state, op_array_max_elements * sizeof(u16), sizeof(u16)).base,
        .op_use_arrays = allocateBump(&allocate_state, op_array_max_elements * sizeof(u16), sizeof(u16)).base,
        .op_const_arrays = allocateBump(&allocate_state, op_array_max_elements * sizeof(u16), sizeof(u16)).base,
        .op_use_array_elements = allocateBump(&allocate_state, op_array_max_elements * sizeof(u16), sizeof(u16)).base,
        .op_const_array_elements = allocateBump(&allocate_state, op_array_max_elements * sizeof(u16), sizeof(u16)).base,
        .use_array = allocateBump(&allocate_state, use_array_max_elements * sizeof(u16), sizeof(u16)).base,
        .const_array = allocateBump(&allocate_state, const_array_max_elements * sizeof(u32), sizeof(u32)).base,
    };
}

u16 scirBlockOpAppend(ScirBlockAppendState *state, ScirOpCode code, u16 *use_array, u16 use_array_elements, u32 *const_array, u16 const_array_elements) {
    debugBlock(
        debugAssert(state != NULL, "State is null!", NULL);
        debugAssert(state->block->op_code_array != NULL, "Block op code array is null!", NULL);
        debugAssert(state->block->op_use_arrays != NULL, "Block op use arrays are null!", NULL);
        debugAssert(state->block->op_use_array_elements != NULL, "Block op use array elements is null!", NULL);
        debugAssert(state->block->op_const_arrays != NULL, "Block op const arrays are null!", NULL);
        debugAssert(state->block->op_const_array_elements != NULL, "Block op const array elements are null!", NULL);
        debugAssert(state->block->use_array != NULL, "Block use array is null!", NULL);
        debugAssert(state->block->const_array != NULL, "Block const array is null!", NULL);
    )

    // Find how many elements in the pointer is.
    const u16 op_code_array_index = state->op_code_array_position;
    const u16 use_array_index = state->use_array_position;
    const u16 const_array_index = state->const_array_position;

    state->block->op_use_arrays[op_code_array_index] = use_array_index;
    state->block->op_const_arrays[op_code_array_index] = const_array_index;

    state->block->op_use_array_elements[op_code_array_index] = use_array_elements;
    state->block->op_const_array_elements[op_code_array_index] = const_array_elements;

    // Copy elements from use and const arguments to block use and const arrays.
    for (usize i = 0; i < use_array_elements; ++i) {
        state->block->use_array[state->use_array_position + i] = use_array[i];
    }

    for (usize i = 0; i < const_array_elements; ++i) {
        state->block->const_array[state->const_array_position + i] = const_array[i];
    }

    // Add the operation data itself.
    state->block->op_code_array[op_code_array_index] = (u16)code;

    // Move pointers to the next free position.
    state->op_code_array_position++;
    state->use_array_position += use_array_elements;
    state->const_array_position += const_array_elements;

    return op_code_array_index;
}

u16 scirBlockConstAppend(ScirBlockAppendState *state, u32 *const_array, u16 const_array_elements) {
    debugBlock(
        debugAssert(state != NULL, "State is null!", NULL);
        debugAssert(const_array != NULL, "Const array is null!", NULL);
        debugAssert(state->block->const_array != NULL, "Block const array is null!", NULL);    
    )

    // Find how many elements in the pointer is.
    const u16 op_code_array_index = state->op_code_array_position;
    const u16 const_array_index = state->const_array_position;

    // Copy const data from argument to block const array.
    for (usize i = 0; i < const_array_elements; ++i) {
        state->block->const_array[state->const_array_position] = const_array[i];
    }

    // Increment pointer to the next free position.
    state->const_array_position += const_array_elements;

    return const_array_index;
}