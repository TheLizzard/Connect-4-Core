#include <stdio.h>
#include "ai/ai_math.h"
#include "encoder.h"
#include "random.h"


BitBoard _encode_bb(register const BitBoard bb){
    register BitBoard output = 0;
    for (register Column column=0; column<COLUMNS; column++){
        output += (bb & COLUMN_MASKS[column]) >> column;
    }
    return output;
}


// __uint128_t old_inner_encode_board(register const Board* board){
//     register const __uint128_t player = board_get_player(board);
//     register const __uint128_t sbb1 = _encode_bb(board->player1_bb);
//     register const __uint128_t sbb2 = _encode_bb(board->player2_bb);
//     register const __uint128_t sbb3 = 0x40000000000 + (~(__uint128_t)(sbb1|sbb2));
//     return (sbb1<<86) | (sbb2<<44) | (sbb3<<2) | (player<<1) | (1-player);
// }

uint128_t inner_encode_board(register const Board* board){
    register const bool player = board_get_player(board);
    register const uint64_t sbb1 = _encode_bb(board->player1_bb);
    register const uint64_t sbb2 = _encode_bb(board->player2_bb);
    register const uint64_t sbb3 = 0x40000000000 + (~(uint64_t)(sbb1|sbb2));

    register const uint64_t high = (sbb1 << 22) | (sbb2 >> 20);
    register const uint64_t low = (sbb2 << 44) | (sbb3<<2) | (player<<1) | (1-player);

    return uint128_init(high, low);
}


// void old_outer_encode_board(register const __uint128_t encoded_board, register uint8_t* result){
//     for (register uint8_t i=0; i<128; i++){
//         result[127-i] = (encoded_board>>i)&1;
//     }
// }

void outer_encode_board(register const uint128_t encoded_board, register uint8_t* result){
    for (register uint8_t i=0; i<128; i++){
        result[127-i] = uint128_get_bit(encoded_board, i);
    }
}


// void old_encode_board(register const Board* board, register Scalar* result){
//     uint8_t temp[128];
//     old_outer_encode_board(old_inner_encode_board(board), temp);
//     for (register uint8_t i=0; i<128; i++){
//         result[i] = temp[i];
//     }
// }

void encode_board(register const Board* board, register Scalar* result){
    uint8_t temp[128];
    outer_encode_board(inner_encode_board(board), temp);
    for (register uint8_t i=0; i<128; i++){
        result[i] = temp[i];
    }
}


Column random_move(register const uint8_t mask){
    register uint8_t idx = 0;
    Column moves[COLUMNS];
    for (register uint8_t column=0; column<COLUMNS; column++){
        if (mask&(1<<column)){
            moves[idx++] = column;
        }
    }
    return moves[randint(0, idx-1)];
}


Board random_board_state(register const uint8_t moves_left, register const double avoid_win){
    Board boards[COLUMNS*ROWS+1];
    init_board(boards);
    if (moves_left == 42){
        return boards[0];
    }
    register uint8_t idx = 0;
    while (!board_is_game_over(&boards[idx])){
        register const Board* board = &boards[idx];

        uint8_t moves = 0b01111111;
        for (register Column column=0; column<COLUMNS; column++){
            if (!board_is_move_possible(board, column)){
                moves &= ~(uint8_t)(1<<column);
                continue;
            }
        }
        if (random_double() < avoid_win){
            uint8_t old_moves = moves;
            for (register Column column=0; column<COLUMNS; column++){
                if (board_is_win_after(board, column)){
                    moves &= ~(uint8_t)(1<<column);
                    continue;
                }
            }
            moves = (moves==0) ? old_moves : moves;
        }

        Column move = random_move(moves);
        board_deepcopy(&boards[idx], &boards[idx+1])
        board_move(&boards[idx+1], move);
        idx++;
    }
    if (moves_left > idx){
        return boards[0];
    }
    return boards[idx-moves_left];
}


/*
int main(){
    // srand(42);
    Scalar encoded[128];
    Board b;
    init_board(&b);
    board_move(&b, 5);
    board_move(&b, 5);
    board_move(&b, 3);
    encode_board(&b, encoded);
    for (register uint8_t i=0; i<128; i++){
        printf("%i", (uint8_t)encoded[i]);
    }
    puts("");
    Board board = random_board_state(15, (double)0.8);
    board_print(&board);
}
// */


// Test uint128_t
/*
int main(){
    Board board;
    bool same;
    for (int i=0; i<10000000; i++){
        board = random_board_state((uint8_t)randint(0, 42), 0.5);
        same = (inner_encode_board(&board) == old_inner_encode_board(&board));
        assert_true(same, "TestError");
        if ((i+1) % 100000 == 0){
            printf("[TEST]: Passed test %i\n", i+1);
        }
    }
    puts("[TEST]: Passed all tests");
    return 0;
}
// */
/*
int main(){
    Board board;
    Scalar old[128];
    Scalar new[128];
    for (int i=0; i<10000000; i++){
        board = random_board_state((uint8_t)randint(0, 42), 0.5);
        old_encode_board(&board, old);
        encode_board(&board, new);
        for (int j=0; j<128; j++){
            assert_true(old[j] == new[j], "TestError");
        }
        if ((i+1) % 100000 == 0){
            printf("[TEST]: Passed test %i\n", i+1);
        }
    }
    puts("[TEST]: Passed all tests");
    return 0;
}
// */