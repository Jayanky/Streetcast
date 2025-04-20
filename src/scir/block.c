#include <common.h>
#include <scir.h>
#include <allocate.h>
#include <debug.h>

ScirBlock scirBlockAllocate(AllocateBuffer *buffer, usize op_array_capacity, usize use_array_capacity, usize const_array_capacity) {
    debugBlock(
        debugAssert(buffer != NULL, "Buffer is null!", NULL);
        const usize requested_capacity = op_array_capacity + (op_array_capacity >> 1) + use_array_capacity + const_array_capacity;
        debugAssert(
            requested_capacity <= buffer->capacity, 
            "Requested capacity of %zd larger than buffer capacity of %zd!", 
            requested_capacity, buffer->capacity
        );
    )

    AllocateBumpState allocate_state = allocateBufferBumpStateBind(buffer);

    return (ScirBlock) {
        .op_array = allocateBump(&allocate_state, op_array_capacity, 16).base,
        .op_lifetime_array = allocateBump(&allocate_state, op_array_capacity >> 1, sizeof(u16)).base,
        .use_array = allocateBump(&allocate_state, use_array_capacity, sizeof(u16)).base,
        .const_array = allocateBump(&allocate_state, const_array_capacity, sizeof(u32)).base,
        .op_array_capacity = op_array_capacity,
        .use_array_capacity = use_array_capacity,
        .const_array_capacity = const_array_capacity,
    };
}

void scirBlockOpDegreeDetermine(ScirBlock *block, AllocateBuffer *buffer, u16 block_op_array_elements) {
    debugBlock(
        debugAssert(block != NULL, "Block is null!", NULL);
        debugAssert(buffer != NULL, "Buffer is null!", NULL);
        debugAssert((uptr)buffer % 2 == 0, "Buffer not aligned to a 2 byte boundary!", NULL);
        debugAssert(buffer->base != NULL, "Buffer base is null!", NULL);    
        debugAssert(
            buffer->capacity >= (block_op_array_elements * 2), 
            "Buffer capacity of %zd bytes is too small for required capacity of %d bytes!", 
            buffer->capacity, block_op_array_elements * 2
        );
    )

    memset(buffer->base, 0, buffer->capacity);

    // Store all register allocation indices
    u16 *block_op_degree_array = (u16*)buffer->base;

    // First op always takes register 0
    block_op_degree_array[0] = 0;

    for (usize i = 0; i < block_op_array_elements; ++i) {
        // The (max) number of operations this operation interferes with.
        u16 op_lifetime = block->op_lifetime_array[i] - i;

        // Type and degree, which are used to determine if operations within the lifetime interfere.
        ScirOpType op_type = (block->op_array[i].code & 0x3800);
        u16 op_degree = block_op_degree_array[i]; 

        // Bump the register index for each interfering operation.
        // (> 1) and (i - 1) are to prevent incrementing the current op and the last op in the lifetime.
        while (op_lifetime > 1) {
            usize compare_op_index = (i - 1) + op_lifetime;

            // Different types use different register sets, so they don't interfere.
            u16 compare_op_type = block->op_array[i].code & 0x3800;
            u16 *compare_op_degree = block_op_degree_array + compare_op_index;

            *compare_op_degree += (*compare_op_degree >= op_degree) && (compare_op_type == op_type);
            
            op_lifetime--;
        }
        // Died so young...
    }
}

u16 scirBlockOpAppend(ScirBlockAppendState *state, ScirOpCode code, u16 *use_array, u16 use_array_elements) {
    debugBlock(
        debugAssert(state != NULL, "State is null!", NULL);
        debugAssert(use_array != NULL, "Use array is null!", NULL);
        debugAssert(state->op_array_position != NULL, "State op array position is null!", NULL);
        debugAssert(state->use_array_position != NULL, "State use array position is null!", NULL);
        debugAssert(state->block->op_array != NULL, "Block op array is null!", NULL);
        debugAssert(state->block->use_array != NULL, "Block use array is null!", NULL);
    )

    // Find how many elements in the pointer is.
    const u16 op_array_index = state->op_array_position - state->block->op_array;
    const u16 use_array_index = state->use_array_position - state->block->use_array;

    // Since the current operation starts at its own index, initialize its lifetime to it's start.
    state->block->op_lifetime_array[op_array_index] = op_array_index;

    // Copy elements from use argument to block use array.
    // Also update the lifetimes of uses to go up to the current op.
    for (usize i = 0; i < use_array_elements; ++i) {
        u16 use = use_array[i];
        state->use_array_position[i] = use;
        state->block->op_lifetime_array[use] = op_array_index; 
    }

    // Add the operation data itself.
    *state->op_array_position = (ScirOp){
        .code = (u16)code,
        .use_array = use_array_index,
    };

    // Move pointers to the next free position.
    state->op_array_position++;
    state->op_lifetime_array_position++;
    state->use_array_position += use_array_elements;

    return op_array_index;
}

u16 scirBlockConstAppend(ScirBlockAppendState *state, u32 *const_array, u16 const_array_elements) {
    debugBlock(
        debugAssert(state != NULL, "State is null!", NULL);
        debugAssert(const_array != NULL, "Const array is null!", NULL);
        debugAssert(state->const_array_position != NULL, "State const array position is null!", NULL);
        debugAssert(state->block->const_array != NULL, "Block const array is null!", NULL);    
    )

    // Find how many elements in the pointer is.
    const u16 const_array_offset = state->const_array_position - state->block->const_array;

    // Copy const data from argument to block const array.
    for (usize i = 0; i < const_array_elements; ++i) {
        state->const_array_position[i] = const_array[i];
    }

    // Increment pointer to the next free position.
    state->const_array_position += const_array_elements;

    return const_array_offset;
}