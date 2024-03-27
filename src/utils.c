#include <stdint.h>
#include <stdio.h>
#include "utils.h"


uint64_t atoi_u64(const char* string){
    uint64_t output = 0;
    while (true){
        if (string[0] == '\x00'){
            return output;
        }
        output *= 10;
        unsigned char character = (((unsigned char)string[0]) - '0');
        if (character > 9){
            assert_true(0, "Invalid character");
        }
        output += character;
        string += sizeof(char);
    }
}