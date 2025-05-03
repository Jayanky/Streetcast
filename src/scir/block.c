#include <scir.h>

#include <common.h>
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
        .op_code_array_elements = 0,
        .use_array_elements = 0,
        .const_array_elements = 0,
    };
}

u16 scirBlockOpAppend(ScirBlock *block, ScirOpCode code, u16 *use_array, u16 use_array_elements, u32 *const_array, u16 const_array_elements) {
    debugBlock(
        debugAssert(block != NULL, "State is null!", NULL);
        debugAssert(block->op_code_array != NULL, "Block op code array is null!", NULL);
        debugAssert(block->op_use_arrays != NULL, "Block op use arrays are null!", NULL);
        debugAssert(block->op_use_array_elements != NULL, "Block op use array elements is null!", NULL);
        debugAssert(block->op_const_arrays != NULL, "Block op const arrays are null!", NULL);
        debugAssert(block->op_const_array_elements != NULL, "Block op const array elements are null!", NULL);
        debugAssert(block->use_array != NULL, "Block use array is null!", NULL);
        debugAssert(block->const_array != NULL, "Block const array is null!", NULL);
    )

    const u16 op_code_array_index = block->op_code_array_elements;

    block->op_use_arrays[op_code_array_index] = block->use_array_elements;
    block->op_const_arrays[op_code_array_index] = block->const_array_elements;

    block->op_use_array_elements[op_code_array_index] = use_array_elements;
    block->op_const_array_elements[op_code_array_index] = const_array_elements;

    // Copy elements from use and const arguments to block use and const arrays.
    for (usize i = 0; i < use_array_elements; i += 1) {
        block->use_array[block->use_array_elements + i] = use_array[i];
    }

    for (usize i = 0; i < const_array_elements; i += 1) {
        block->const_array[block->const_array_elements + i] = const_array[i];
    }

    // Add the operation data itself.
    block->op_code_array[op_code_array_index] = (u16)code;

    // Move pointers to the next free position.
    block->op_code_array_elements += 1;
    block->use_array_elements += use_array_elements;
    block->const_array_elements += const_array_elements;

    return op_code_array_index;
}