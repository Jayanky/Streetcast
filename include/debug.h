#ifndef SC_DEBUG_H_
#define SC_DEBUG_H_

#include <common.h>

#ifdef SC_DEBUG_BOOL_OPTION_

#define debugBlock(statement) do { statement } while(0);

#define debugAssert(statement, format, ...) if (!(statement)) { \
    const usize message_length = 128; \
    char message_buffer[message_length]; \
    snprintf(message_buffer, message_length, format, __VA_ARGS__); \
    message_buffer[message_length - 1] = '\0'; \
    fprintf(stderr, "File: %s\nLine: %d\nMessage: %s\n", __FILE__, __LINE__, message_buffer); \
    exit(EXIT_FAILURE); \
} \

#else

#define debugBlock(statement) (void*){0};
#define debugAssert(statement, format, ...) (void*){0}

#endif

#endif