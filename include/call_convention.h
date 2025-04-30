#ifndef SC_CALL_H_
#define SC_CALL_H_

#include <common.h>

typedef struct {
    // Callee saved.
    const u16 *saved_register_start_array;
    const u16 *saved_register_length_array;

    // Caller saved.
    const u16 *temporary_register_start_array;
    const u16 *temporary_register_length_array;

    // Arguments for calls.
    const u16 *argument_register_start_array;
    const u16 *argument_register_length_array;

    // Return values from calls.
    const u16 *return_register_start_array;
    const u16 *return_argument_length_array;

    // Size of each array.
    const u16 saved_register_start_array_elements;
    const u16 temporary_register_start_array_elements;
    const u16 argument_register_start_array_elements;
    const u16 return_register_start_array_elements;

    // Other Register locations.
    const u16 stack_pointer_register;
    const u16 frame_pointer_register;
    const u16 link_register;
} CallConventionMips;

/* Returns a filled-out `CallConventionMips` struct for ABI information. */
CallConventionMips callConventionMipsFetch();

#endif