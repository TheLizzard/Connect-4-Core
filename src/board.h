#pragma once
#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include "utils.h"

// #define SYMETRIC_BOARD_MOVE_NOT_ALLOW
#define ADV_HASH

#define COLUMNS ((Column)7)
#define ROWS ((Row)6)

typedef uint8_t Row;
typedef uint8_t Column;
typedef uint64_t BitBoard;
typedef BitBoard BoardHash;
typedef uint8_t ColumnState;
typedef uint8_t Flags;


#define GAME_OVER ((Flags)1)
#define GAME_DRAW (((Flags)2)|GAME_OVER)
#if defined(SYMETRIC_BOARD_MOVE_NOT_ALLOW)
#define SYMETRIC ((Flags)4)
#define CAN_BE_SYMETRIC ((Flags)8)
#endif
typedef struct{
    BitBoard player1_bb;
    BitBoard player2_bb;
    Flags flags;
} Board;


#define _bb_get_row_column_mask(row, column) (1ULL << ((column)*(ROWS+1)+(row)))
#define FULL_BOARD_MASK (_bb_get_row_column_mask(0, COLUMNS) - 1)
#define BOARD_SIZE ((ROWS+1)*COLUMNS)
#define DRAW_MASK 141845657554976
#define PLAYER_1_BIT_MASK _bb_get_row_column_mask(0, COLUMNS+1)

// bool _bb_get(register const BitBoard state, register const Row row, register const Column column);
// BitBoard _bb_toggle(register const BitBoard state, register const Row row, register const Column column);
// ColumnState _bb_get_column(register const BitBoard state, register const Column column);
// Row _bb_get_next_empty_row(register const BitBoard state, register const Column column);
// BitBoard _bb_mirror(register const BitBoard b);
// bool _bb_detect_win(register const BitBoard b);


// init/reset/del board
void init_board(register Board* board);
void board_load(register Board* board, register const BitBoard player1_bb, register const BitBoard player2_bb);
// getters
bool board_is_move_possible(register const Board* board, register const Column column);
char* board_string(register const Board* board);
BoardHash board_hash(register const Board* board);
// move/unmove
Row board_move(register Board* board, register const Column column);
void board_unmove(register Board* board, register const Row row, register const Column column);
bool board_is_win_after(register const Board* board, register const Column column);
// misc
void board_switch_players(register Board* board);
uint8_t board_num_empty_squares(register const Board* board);


#define board_print(board) {char*string=board_string(board); puts(string); free(string);}
#define board_is_game_over(board) (((board)->flags & GAME_OVER) == GAME_OVER)
#define board_is_game_draw(board) (((board)->flags & GAME_DRAW) == GAME_DRAW)
#define board_get_player(board) (((board)->player1_bb & PLAYER_1_BIT_MASK) != 0)
#define board_switch_players(board) {BitBoard temp=board->player1_bb; board->player1_bb=board->player2_bb; board->player2_bb=temp;}
#define board_deepcopy(src, des) {des->player1_bb=src->player1_bb; des->player2_bb=src->player2_bb; des->flags=src->flags;}