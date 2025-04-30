#include <call_convention.h>

#include <common.h>

#ifdef SC_PLATFORM_PSP_OPTION_

static const u16 saved_register_start_array[] = {16};
static const u16 saved_register_length_array[] = {8};

static const u16 temporary_register_start_array[] = {12, 24};
static const u16 temporary_register_length_array[] = {4, 2};

static const u16 argument_register_start_array[] = {4};
static const u16 argument_register_length_array[] = {8};

static const u16 return_register_start_array[] = {1};
static const u16 return_register_length_array[] = {2};

static const u16 stack_pointer_register = 29;
static const u16 frame_pointer_register = 30;
static const u16 link_register = 31;

#else

static const u16 saved_register_start_array[] = {16};
static const u16 saved_register_length_array[] = {8};

static const u16 temporary_register_start_array[] = {8, 24};
static const u16 temporary_register_length_array[] = {8, 2};

static const u16 argument_register_start_array[] = {4};
static const u16 argument_register_length_array[] = {4};

static const u16 return_register_start_array[] = {1};
static const u16 return_register_length_array[] = {2};

static const u16 stack_pointer_register = 29;
static const u16 frame_pointer_register = 30;
static const u16 link_register = 31;

#endif

CallConventionMips callConventionMipsFetch() {
    return (CallConventionMips){
        .saved_register_start_array = saved_register_start_array,
        .saved_register_length_array = saved_register_length_array,

        .temporary_register_start_array = temporary_register_start_array,
        .temporary_register_length_array = temporary_register_length_array,

        .argument_register_start_array = argument_register_start_array,
        .argument_register_length_array = argument_register_length_array,

        .return_register_start_array = return_register_start_array,
        .return_argument_length_array = return_register_length_array,

        .saved_register_start_array_elements = arrayelements(saved_register_start_array),
        .temporary_register_start_array_elements = arrayelements(temporary_register_start_array),
        .argument_register_start_array_elements = arrayelements(argument_register_start_array),
        .return_register_start_array_elements = arrayelements(return_register_start_array),

        .stack_pointer_register = stack_pointer_register,
        .frame_pointer_register = frame_pointer_register,
        .link_register = link_register,
    };
}