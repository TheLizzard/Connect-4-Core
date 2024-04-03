#pragma once
#include "ai.h"

float nn_network(register const Vector input);
void nn_encode(Vector r, uint64_t bb1, uint64_t bb2, uint8_t player);