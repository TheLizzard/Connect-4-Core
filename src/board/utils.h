#pragma once
#include <stdbool.h>
#include <stdint.h>
#include <assert.h>
#include <stdio.h>

#define ASSERTS

#if defined(ASSERTS)
    #define assert_true(flag, text) {if ((flag) == 0){puts(text); assert(0);}}
    #define assert_false(flag, text) {if ((flag) != 0){puts(text); assert(0);}}
#else
    #define assert_true(flag, text) {}
    #define assert_false(flag, text) {}
#endif

#define atoi_u64(string) ({ \
    register const char* _atoi_u64_string = (string); \
    register uint64_t _atoi_u64_output = 0; \
    while (true){ \
        if (_atoi_u64_string[0] == '\x00'){ \
            break; \
        } \
        _atoi_u64_output *= 10; \
        register unsigned char _atoi_u64_character = (((unsigned char)_atoi_u64_string[0]) - '0'); \
        if (_atoi_u64_character > 9){ \
            assert_true(0, "Invalid character"); \
        } \
        _atoi_u64_output += _atoi_u64_character; \
        _atoi_u64_string += sizeof(char); \
    } \
    _atoi_u64_output; \
})

// Comment