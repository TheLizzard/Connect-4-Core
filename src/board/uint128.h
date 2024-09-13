#pragma once
#include <stdint.h>


// Use GCC's __uint128_t type if it's defined
#if defined(__SIZEOF_INT128__)
typedef __uint128_t uint128_t;

#define uint128_init(high, low) ({ \
    register const __uint128_t _uint128_init_high = (__uint128_t)(high); \
    register const __uint128_t _uint128_init_low = (__uint128_t)(low); \
    (_uint128_init_high << 64) | _uint128_init_low; \
})

#define uint128_get_bit(x, index) ({ \
    register const __uint128_t _uint128_get_bit_x = (__uint128_t)(x); \
    register const uint8_t _uint128_get_bit_index = (uint8_t)(index); \
    (uint8_t)((_uint128_get_bit_x >> _uint128_get_bit_index) & 1); \
})


// If __uint128_t isn't defined, define the functions ourselves
#else
typedef struct{
    uint64_t high;
    uint64_t low;
} uint128_t;

#define uint128_init(high, low) ({ \
    register const uint64_t _uint128_init_high = (uint64_t)(high); \
    register const uint64_t _uint128_init_low = (uint64_t)(low); \
    (uint128_t){_uint128_init_high, _uint128_init_low}; \
})

#define uint128_get_bit(x, index) ({ \
    register const uint128_t _uint128_get_bit_x = (uint128_t)(x); \
    register const uint8_t _uint128_get_bit_index = (uint8_t)(index); \
    register uint8_t _uint128_output = 0; \
    if (_uint128_get_bit_index < 64){ \
        _uint128_output = (_uint128_get_bit_x.low >> _uint128_get_bit_index) & 1; \
    }else{ \
        _uint128_output = (_uint128_get_bit_x.high >> (_uint128_get_bit_index-64)) & 1; \
    } \
    _uint128_output; \
})

#endif