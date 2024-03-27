#pragma once
#include <assert.h>
#include <stdio.h>

typedef uint8_t bool;
#define false 0
#define true 1

#define ASSERTS

#if defined(ASSERTS)
    #define assert_true(flag, text) {if ((flag) == 0){puts(text); assert(0);}}
    #define assert_false(flag, text) {if ((flag) != 0){puts(text); assert(0);}}
#else
    #define assert_true(flag, text) {}
    #define assert_false(flag, text) {}
#endif
uint64_t atoi_u64(register const char* string);