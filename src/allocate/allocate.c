#include <common.h>
#include <allocate.h>
#include <debug.h>

AllocateBuffer allocateBump(AllocateBumpState *bump, usize size, usize alignment) {
    debugBlock(
        debugAssert(
            (alignment & (alignment - 1)) == 0, 
            "Alignment %zd is not a power of two!", 
            alignment
        );
    )

    const uptr align_mask = alignment - 1;

    bump->position -= (size - (size & align_mask)) + ((uptr)bump->position & align_mask);

    debugBlock(
        debugAssert(
            bump->position >= bump->buffer->base, 
            "Bump position %p less than buffer base %p!", 
            bump->position, bump->buffer->base
        );
    )

    return (AllocateBuffer){
        .base = bump->position,
        .capacity = size,
    };
}

