#include <string.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include "board.h"

// #define TEST

static const char* RED_STRING = "\x1b[91m⬤\x1b[0m";
static const char* YELLOW_STRING = "\x1b[93m⬤\x1b[0m";
static const char* EMPTY_STRING = "-";
static const char* ERROR_STRING = "ERROR";

BitBoard COLUMN_MASKS[] = {
                                         127,
                                       16256,
                                     2080768,
                                   266338304,
                                 34091302912,
                               4363686772736,
                             558551906910208
                           };
Row NEXT_EMPTY_ROW[] = {0,1,ROWS,2,ROWS,ROWS,ROWS,3,ROWS,ROWS,ROWS,ROWS,ROWS,
                        ROWS,ROWS,4,ROWS,ROWS,ROWS,ROWS,ROWS,ROWS,ROWS,ROWS,
                        ROWS,ROWS,ROWS,ROWS,ROWS,ROWS,ROWS,5,ROWS,ROWS,ROWS,
                        ROWS,ROWS,ROWS,ROWS,ROWS,ROWS,ROWS,ROWS,ROWS,ROWS,ROWS,
                        ROWS,ROWS,ROWS,ROWS,ROWS,ROWS,ROWS,ROWS,ROWS,ROWS,ROWS,
                        ROWS,ROWS,ROWS,ROWS,ROWS,ROWS,6};
const BitBoard BOARD_HASH_MAGIC = 4432676798593ULL;


bool _bb_get(register const BitBoard state, register const Row row, register const Column column){
    return (state & _bb_get_row_column_mask(row, column)) != 0;
}

BitBoard _bb_toggle(register const BitBoard state, register const Row row, register const Column column){
    return state ^ _bb_get_row_column_mask(row, column);
}

ColumnState _bb_get_column(register const BitBoard state, register const Column column){
    return ((state & COLUMN_MASKS[column]) >> ((ROWS+1)*column)) & 0b111111;
}

Row _bb_get_next_empty_row(register const BitBoard state, register const Column column){
    return NEXT_EMPTY_ROW[_bb_get_column(state, column)];
}

BitBoard _bb_mirror(register const BitBoard b){
    BitBoard output = 0;
    for (Column column=0; column<COLUMNS; column++){
        output |= ((b & COLUMN_MASKS[column]) >> column*(ROWS+1)) << (BOARD_SIZE-(column+1)*(ROWS+1));
    }
    return output;
}

bool _bb_detect_win(register const BitBoard b){
    // Copied from:
    //     https://github.com/denkspuren/BitboardC4/blob/master/BitboardDesign.md
    BitBoard bb;
    bb = b & (b >> 1);
    if (bb & (bb >> 2)){
        return true;
    }
    bb = b & (b >> (COLUMNS-1));
    if (bb & (bb >> (2 * (COLUMNS-1)))){
        return true;
    }
    bb = b & (b >> COLUMNS);
    if (bb & (bb >> (2 * COLUMNS))){
        return true;
    }
    bb = b & (b >> (COLUMNS+1));
    if (bb & (bb >> (2 * (COLUMNS+1)))){
        return true;
    }
    return false;
}


void init_board(register Board* board){
    board->player1_bb = PLAYER_1_BIT_MASK;
    board->player2_bb = 0;
    #if defined(SYMETRIC_BOARD_MOVE_NOT_ALLOW)
    board->flags = SYMETRIC | CAN_BE_SYMETRIC;
    #else
    board->flags = 0;
    #endif
}

#if defined(SYMETRIC_BOARD_MOVE_NOT_ALLOW)
Flags board_is_symetric(register const Board* board){
    BitBoard bb1 = board->player1_bb & FULL_BOARD_MASK;
    BitBoard bb2 = board->player2_bb & FULL_BOARD_MASK;
    assert_false(bb1&bb2, "Invalid bitboards");
    BitBoard bb1m = _bb_mirror(bb1);
    BitBoard bb2m = _bb_mirror(bb2);
    // Can't be symetric
    if (bb1m & bb2){
        return 0;
    }
    // Check if they are symetric
    if ((bb1m == bb1) && (bb2m == bb2)){
        return SYMETRIC | CAN_BE_SYMETRIC;
    }
    return CAN_BE_SYMETRIC;
}
#endif

void board_load(register Board* board, register const BitBoard player1_bb, register const BitBoard player2_bb){
    board->player1_bb = player1_bb;
    board->player2_bb = player2_bb;
    assert_false(player1_bb & player2_bb, "Invalid bitboards (overlapping)");
    #if defined(SYMETRIC_BOARD_MOVE_NOT_ALLOW)
    board->flags &= (Flags)(~(SYMETRIC|CAN_BE_SYMETRIC));
    board->flags |= board_is_symetric(board);
    #endif
}

bool board_is_move_possible(register const Board* board, register const Column column){
    if (column >= COLUMNS){return false;}
    #if defined(SYMETRIC_BOARD_MOVE_NOT_ALLOW)
    if ((board->flags & SYMETRIC) && (column < 3)){return false;}
    #endif
    return !_bb_get(board->player1_bb|board->player2_bb, ROWS-1, column);
}

char* board_string(register const Board* board){
    char output[1024];
    memset(&output[0], 0, sizeof(output));

    strncpy(output, "======= Board =======\n", 23);
    char* o = output + strlen(output)*sizeof(char);

    bool player = board_get_player(board);
    assert_true((player == 0) || (player == 1), "player not in {0,1}");

    BitBoard bb1 = (player) ? board->player1_bb : board->player2_bb;
    BitBoard bb2 = (player) ? board->player2_bb : board->player1_bb;
    for (Row row=0; row<ROWS; row++){
        memset(o, ' ', 4*sizeof(char));
        o += 4*sizeof(char);
        for (Column column=0; column<COLUMNS; column++){
            bool p1 = _bb_get(bb1, ROWS-row-1, column);
            bool p2 = _bb_get(bb2, ROWS-row-1, column);
            if (p1 && p2){
                strcpy(o, ERROR_STRING);
                o += strlen(ERROR_STRING)*sizeof(char);
            }else if (p1){
                strcpy(o, RED_STRING);
                o += strlen(RED_STRING)*sizeof(char);
            }else if (p2){
                strcpy(o, YELLOW_STRING);
                o += strlen(YELLOW_STRING)*sizeof(char);
            }else{
                strcpy(o, EMPTY_STRING);
                o += strlen(EMPTY_STRING)*sizeof(char);
            }
            o[0] = ' ';
            o += sizeof(char);
        }
        memset(o, ' ', 3*sizeof(char));
        o[3] = '\n';
        o += 4*sizeof(char);
    }
    memset(o, '=', 21*sizeof(char));

    assert_true(strlen(output) < 1024, "Overflow")
    char* real_output = (char*) malloc((strlen(output)+1)*sizeof(char));
    assert_false(real_output == NULL, "malloc failed");
    strcpy(real_output, output);
    return real_output;
}

Row board_move(register Board* board, register const Column column){
    assert_true(column < COLUMNS, "Invalid column");
    assert_false(board_is_game_over(board), "Game already over");
    BitBoard fullboard = board->player1_bb | board->player2_bb;
    Row row = _bb_get_next_empty_row(fullboard, column);
    assert_true(row < ROWS, "Column already full");
    board->player1_bb = _bb_toggle(board->player1_bb, row, column);
    if (_bb_detect_win(board->player1_bb)){
        board->flags ^= GAME_OVER;
        return row;
    }
    fullboard = _bb_toggle(fullboard, row, column);
    if ((fullboard & DRAW_MASK) == DRAW_MASK){
        board->flags ^= GAME_DRAW;
        return row;
    }
    #if defined(SYMETRIC_BOARD_MOVE_NOT_ALLOW)
    if (board->flags & CAN_BE_SYMETRIC){
        if (board->flags & SYMETRIC){
            board->flags &= (Flags)(~SYMETRIC);
        }else{
            board->flags |= board_is_symetric(board);
        }
    }
    #endif
    board_switch_players(board);
    return row;
}


void board_unmove(register Board* board, register const Row row, register const Column column){
    // WARNING: This doesn't check if the move has actually been made
    if (board_is_game_over(board)){
        board->flags = 0;
    }else{
        board_switch_players(board);
    }
    // Undo move
    board->player1_bb = _bb_toggle(board->player1_bb, row, column);
    #if defined(SYMETRIC_BOARD_MOVE_NOT_ALLOW)
    board->flags &= (Flags)(~(SYMETRIC|CAN_BE_SYMETRIC));
    board->flags |= board_is_symetric(board);
    #endif
}

bool board_is_win_after(register const Board* board, register const Column column){
    Row row = _bb_get_next_empty_row(board->player1_bb | board->player2_bb, column);
    return _bb_detect_win(_bb_toggle(board->player1_bb, row, column));
}

BoardHash board_hash(register const Board* board){
    BitBoard bb1 = board->player1_bb & FULL_BOARD_MASK;
    BitBoard bb2 = board->player2_bb & FULL_BOARD_MASK;
    assert_false(bb1&bb2, "Invalid bitboards");
    #if defined(ADV_HASH)
    BitBoard bb1m = _bb_mirror(bb1);
    if (bb1 > bb1m){
        bb1 = bb1m;
        bb2 = _bb_mirror(bb2);
    }else if (bb1 == bb1m){
        BitBoard bb2m = _bb_mirror(bb2);
        if (bb2 > bb2m){
            bb1 = bb1m;
            bb2 = bb2m;
        }
    }
    #endif
    BitBoard fullboard = bb1 | bb2;
    return (fullboard+BOARD_HASH_MAGIC) | bb1;
}

uint8_t board_num_empty_squares(register const Board* board){
    uint8_t output = 0;
    BitBoard fullboard = board->player1_bb | board->player2_bb;
    for (Column column=0; column<COLUMNS; column++){
        output += (Row)(ROWS - _bb_get_next_empty_row(fullboard, column));
    }
    return output;
}


/*
=========================== Testing ==================================
*/
#if defined(TEST)
void test_draw_player_switching(){
    int player = 1;
    Board board;
    init_board(&board);
    assert_false(board_is_game_over(&board), "Game shouldn't be over");
    Column moves[] = {0,1,2,4,5,6,3};
    for (int j=0; j<7; j++){
        Row row = 0;
        Column move = moves[j];
        assert_false(((board.player1_bb|board.player2_bb) & ~FULL_BOARD_MASK)^PLAYER_1_BIT_MASK, "FULL_BOARD_MASK failed");
        for (int i=0; i<6; i++){
            assert_true(player == board_get_player(&board), "Players don't switch");
            player = 1-player;
            row = board_move(&board, move);
            assert_false(board_is_game_over(&board), "Game shouldn't be over");
        }
        assert_true(row == 5, "Row should be 5");
        if (move == 6){
            board_unmove(&board, row, move);
            player = 1-player;
        }
        assert_false(board_is_game_over(&board), "Game shouldn't be over");
    }
    board_move(&board, 6);
    assert_true(board_is_game_over(&board), "Game should be over");
    assert_true(board_is_game_draw(&board), "Game should be a draw");
}

void test_win(){
    // Only tests horizontal and vertical

    for (Column column=0; column<COLUMNS; column++){
        Board board;
        init_board(&board);
        for (int i=0; i<4; i++){
            assert_false(board_is_game_over(&board), "Game shouldn't be over");
            board_move(&board, column);
            if (i != 3){
                assert_false(board_is_game_over(&board), "Game shouldn't be over");
                board_move(&board, (Column)((column+1)%COLUMNS));
            }
        }
        assert_true(board_is_game_over(&board), "Game should be over");
    }

    for (Column column=0; column<COLUMNS; column++){
        Board board;
        init_board(&board);
        for (int i=0; i<4; i++){
            assert_false(board_is_game_over(&board), "Game shouldn't be over");
            board_move(&board, (Column)((column+i)%COLUMNS));
            if (i != 3){
                assert_false(board_is_game_over(&board), "Game shouldn't be over");
                board_move(&board, (Column)((column+i)%COLUMNS));
            }
        }
        if (column+3 >= COLUMNS){
            assert_false(board_is_game_over(&board), "Game shouldn't be over");
        }else{
            assert_true(board_is_game_over(&board), "Game should be over");
        }
    }
}


void test_equal_hash(){
    Board board1, board2;
    BoardHash hash1, hash2;

    init_board(&board1);
    init_board(&board2);
    board_move(&board1, 3);
    board_move(&board2, 3);
    hash1 = board_hash(&board1);
    hash2 = board_hash(&board2);
    assert_true(hash1 == hash2, "Mirrored board should have equal hash");

    init_board(&board1);
    init_board(&board2);
    board_move(&board1, 0);
    board_move(&board2, 6);
    hash1 = board_hash(&board1);
    hash2 = board_hash(&board2);
    assert_true(hash1 == hash2, "Mirrored board should have equal hash");

    init_board(&board1);
    init_board(&board2);
    board_move(&board1, 2);
    board_move(&board2, 4);
    hash1 = board_hash(&board1);
    hash2 = board_hash(&board2);
    assert_true(hash1 == hash2, "Mirrored board should have equal hash");

    init_board(&board1);
    init_board(&board2);
    board_move(&board1, 1);
    board_move(&board2, 5);
    hash1 = board_hash(&board1);
    hash2 = board_hash(&board2);
    assert_true(hash1 == hash2, "Mirrored board should have equal hash");
}


int _test_randint(int low, int high){
    return (rand() % (high+1 - low)) + low;
}

void _test_move_random(Board* board){
    while (1){
        Column move = (Column) _test_randint(0, COLUMNS-1);
        if (board_is_move_possible(board, move)){
            board_move(board, move);
            return;
        }
    }
}

void test_board_hash(int number_of_tests){
    BoardHash h1, h2;
    Board b1, b2;
    int m;
    bool same_hash, same_board;
    srand(42);

    for (int i=0; i<number_of_tests; i++){
        init_board(&b1);
        init_board(&b2);
        m = _test_randint(0, 43);
        for (int j=0; j<m; j++){
            _test_move_random(&b1);
            if (board_is_game_over(&b1)){
                break;
            }
        }
        m = _test_randint(0, 43);
        for (int j=0; j<m; j++){
            _test_move_random(&b2);
            if (board_is_game_over(&b2)){
                break;
            }
        }
        h1 = board_hash(&b1);
        h2 = board_hash(&b2);
        same_hash = (h1 == h2);
        same_board = ((b1.player1_bb&FULL_BOARD_MASK) == (b2.player1_bb&FULL_BOARD_MASK)) & \
                     ((b1.player2_bb&FULL_BOARD_MASK) == (b2.player2_bb&FULL_BOARD_MASK)) & \
                     (board_get_player(&b1) == board_get_player(&b2)) & \
                     (b1.flags == b2.flags);
        #if defined(ADV_HASH)
        same_board |= (_bb_mirror(b1.player1_bb&FULL_BOARD_MASK) == (b2.player1_bb&FULL_BOARD_MASK)) & \
                      (_bb_mirror(b1.player2_bb&FULL_BOARD_MASK) == (b2.player2_bb&FULL_BOARD_MASK)) & \
                      (board_get_player(&b1) == board_get_player(&b2)) & \
                      (b1.flags == b2.flags);
        #endif
        if (same_hash != same_board){
            board_print(&b1);
            board_print(&b2);
        }
        if (same_hash){
            assert_true(same_board, "Hash clash");
        }else{
            assert_false(same_board, "hash(board) inconsistent");
        }
        if ((i+1) % 10000000 == 0){
            printf("Done %i%%\n", (int)(100*(i+1.0)/number_of_tests));
        }
    }
}
/*
=======================================================================
*/


int main(){
    test_draw_player_switching();
    test_win();
    #if defined(ADV_HASH)
    test_equal_hash();
    #endif
    clock_t begin = clock();
    test_board_hash(1000000000);
    clock_t end = clock();
    printf("Took %f sec\n", (double)(end - begin) / CLOCKS_PER_SEC);
    printf("All tests passed\n");

    return 0;
}
#endif