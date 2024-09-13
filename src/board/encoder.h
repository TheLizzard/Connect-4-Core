#pragma once
#include "uint128.h"
#include "board.h"


BitBoard _encode_bb(register const BitBoard bb);
uint128_t inner_encode_board(register const Board* board);
void outer_encode_board(register const uint128_t encoded_board, register uint8_t* result);
void encode_board(register const Board* board, register Scalar* result);
Board random_board_state(register const uint8_t moves_left, register const double avoid_win);