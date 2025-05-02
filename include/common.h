#ifndef SC_COMMON_H_
#define SC_COMMON_H_

#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <stdlib.h>
#include <alloca.h>

#ifdef SC_PLATFORM_PSP_OPTION_

#include <pspkernel.h>
#include <pspdebug.h>
#include <pspdisplay.h>

#define scPrintf pspDebugScreenPrintf

#else

#define scPrintf printf

#endif

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

typedef int8_t i8;
typedef int16_t i16;
typedef int32_t i32;
typedef int64_t i64;

typedef float f32;
typedef double f64;

typedef uintmax_t usize;
typedef intmax_t isize;
typedef uintptr_t uptr;

#define arrayelements(array) (sizeof((array)) / sizeof(*(array)))

#endif