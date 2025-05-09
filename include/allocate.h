#ifndef SC_ALLOCATE_H_
#define SC_ALLOCATE_H_

#include <common.h>

typedef struct {
    void *base;
    usize capacity;
} AllocateBuffer;

typedef struct {
    AllocateBuffer *buffer;
    void *position;
} AllocateBumpState;

/*
    Creates and initializes a new Bump State with `buffer` bound to it.
*/
static inline AllocateBumpState allocateBufferBumpStateBind(AllocateBuffer *buffer) {
    return (AllocateBumpState){
        .buffer = buffer,
        .position = buffer->base + buffer->capacity,
    };
}

/* 
    Reset `state` to initial conditions.
    Effectively clears the bound buffer for reuse, though it doesn't zero out elements.
*/
static inline void allocateBumpStateReset(AllocateBumpState *state) {
    state->position = state->buffer->base + state->buffer->capacity;
}

/* Allocate `size` bytes aligned to `alignment` from the buffer bound to `state`. */
AllocateBuffer allocateBump(AllocateBumpState *state, usize size, usize alignment);

#endif