#include <inttypes.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <time.h>
#include "transposition_table.h"
#include "utils.h"

#define TT_DEPTH_THRESHOLD 5
#define CHECK_FORCED false
#define CHECK_WIN false
#define DEEPCOPY false

#define DEEPENING true
#define DEPTH 18

#define DEPLOY false
#define TEST false

#define PRINT_STORED_POSITIONS false
#define DEBUG true


#if TT_DEPTH_THRESHOLD < 0
    #error "Must be a non-negative int"
#endif

#if DEPLOY == true
    #if TEST == true
        #error "Can't deploy and test at the same time"
    #endif
#endif

unsigned long nodes_visited = 0;
TranspositionTable tt;
Depth depth;


// ================================== Helpers ==================================
Column argmax(register const Eval* moves){
    Column max_idx = 0;
    Eval max_eval = NINF;
    for (Column column=0; column<COLUMNS; column++){
        if (moves[column] > max_eval){
            max_eval = moves[column];
            max_idx = column;
        }
    }
    return max_idx;
}

#define _order_moves_swap(order_array, input_array, idx1, idx2) \
    if ((input_array)[(order_array)[idx1]] < (input_array)[(order_array)[idx2]]){ \
        Column temp = (order_array)[idx1]; \
        (order_array)[idx1] = (order_array)[idx2]; \
        (order_array)[idx2] = temp; \
    }


#if COLUMNS != 7
#error "The code bellow is a sorting network that assumes COLUMNS=7"
#endif
void order_moves(register Column* ordered_moves, const Eval* moves){
    // Column ordered_moves[COLUMNS];
    // for (Column column=0; column<COLUMNS; column++){
    //     ordered_moves[column] = column;
    // }
    _order_moves_swap(ordered_moves,moves,0,6);
    _order_moves_swap(ordered_moves,moves,2,3);
    _order_moves_swap(ordered_moves,moves,4,5);
    _order_moves_swap(ordered_moves,moves,0,2);
    _order_moves_swap(ordered_moves,moves,1,4);
    _order_moves_swap(ordered_moves,moves,3,6);
    _order_moves_swap(ordered_moves,moves,0,1);
    _order_moves_swap(ordered_moves,moves,2,5);
    _order_moves_swap(ordered_moves,moves,3,4);
    _order_moves_swap(ordered_moves,moves,1,2);
    _order_moves_swap(ordered_moves,moves,4,6);
    _order_moves_swap(ordered_moves,moves,2,3);
    _order_moves_swap(ordered_moves,moves,4,5);
    _order_moves_swap(ordered_moves,moves,1,2);
    _order_moves_swap(ordered_moves,moves,3,4);
    _order_moves_swap(ordered_moves,moves,5,6);
}

Eval min(register const Eval a, register const Eval b){
    return (a < b) ? a : b;
}
Eval max(register const Eval a, register const Eval b){
    return (a > b) ? a : b;
}
Eval maxs(register const Eval* evals){
    Eval currmax = evals[0];
    for (Column column=0; column<COLUMNS; column++){
        currmax = max(currmax, evals[column]);
    }
    return currmax;
}


#if CHECK_WIN == true
    Column get_winning_move(register const Board* board){
        for (Column column=0; column<COLUMNS; column++){
            if (board_is_move_possible(board, column)){
                if (board_is_win_after(board, column)){
                    return column;
                }
            }
        }
        return COLUMNS;
    }
#endif

#if CHECK_FORCED == true
    Column get_forced_move(register Board* board){
        board_switch_players(board);
        Column column = get_winning_move(board);
        board_switch_players(board);
        return column;
    }
#endif


extern char* moves_to_string(register const Eval* moves){
    char string[1024*COLUMNS];
    memset(&string[0], 0, sizeof(string));
    char* s = string;
    s[0] = '[';
    s += sizeof(char);
    for (Column column=0; column<COLUMNS; column++){
        assert_true(moves[column] >= -99, "Overflow");
        assert_true(moves[column] <= 99, "Overflow");
        char n[1024];
        sprintf(n, "%d", moves[column]);
        assert_true(strlen(n) < 1024, "String overflow");
        strcpy(s, n);
        s += sizeof(char)*strlen(n);
        s[0] = ',';
        s[1] = ' ';
        s += sizeof(char)*2;
    }
    s -= sizeof(char)*2;
    s[0] = ']';
    s[1] = '\x00';

    assert_true(strlen(string) < 1024*COLUMNS, "String overflow");
    char* real_string = malloc((strlen(string)+1)*sizeof(char));
    assert_false(real_string == NULL, "malloc failed");
    strcpy(real_string, string);
    return real_string;
}

void ttentry_print(register const BoardHash hash){
    TTEntry* ttentry = transpositiontable_get(tt, hash);
    if (ttentry == NULL){
        puts("(null)");
    }else{
        printf("depth=%i  flag=%i  %s\n", ttentry->depth, ttentry->flag, moves_to_string(ttentry->moves));
    }
}


// ================================ Actual Code ================================
extern Eval _negamax(register Board* node, register Eval α, register Eval β){
    TTEntry real_ttentry;
    const BoardHash node_hash = board_hash(node);
    Eval alpha_orig = α;
    nodes_visited++;

    // Transposition Table Lookup; node is the lookup key for ttentry
    TTEntry* ttentry = transpositiontable_get(tt, node_hash);
    if (ttentry == NULL){
        init_ttentry(&real_ttentry, depth);
        if (depth >= TT_DEPTH_THRESHOLD){
            ttentry = transpositiontable_add(tt, node_hash, real_ttentry);
        }else{
            ttentry = &real_ttentry;
        }
        assert_true(ttentry != NULL, "ttentry is null");
    }

    if ((ttentry->flag != TT_UNDEFINED) && (ttentry->depth >= depth)){
        assert_false(ttentry->depth > depth, "Should never happen");
        Eval value = maxs(ttentry->moves);
        if (ttentry->flag == TT_EXACT){
            return value;
        }else if (ttentry->flag == TT_LOWERBOUND){
            α = max(α, value);
        }else if (ttentry->flag == TT_UPPERBOUND){
            β = min(β, value);
        }
        if (α >= β){
            return value;
        }
    }

    #if CHECK_WIN == true
        if (ttentry->flag == TT_UNDEFINED){
            Column winning_move = get_winning_move(node);
            if (winning_move != COLUMNS){
                ttentry->moves[winning_move] = 9;
            }else{
                #if CHECK_FORCED == true
                    Column forced_move = get_forced_move(node);
                    if (forced_move != COLUMNS){
                        ttentry->moves[forced_move] = 9;
                    }
                #endif
            }
        }
    #elif CHECK_FORCED == true
        Column forced_move = get_forced_move(node);
        if (forced_move != COLUMNS){
            ttentry->moves[forced_move] = 9;
        }
    #endif

    Eval value = NINF;
    Column ordered_moves[COLUMNS] = {0,1,2,3,4,5,6};
    if (ttentry->flag == TT_LOWERBOUND){
        order_moves(ordered_moves, ttentry->moves);
    }
    depth -= 1;
    for (Column idx=0; idx<COLUMNS; idx++){
        Column move = ordered_moves[idx];
        if (!board_is_move_possible(node, move)){
            continue;
        }
        #if DEEPCOPY == true
            Board real_child;
            Board* child = &real_child;
            board_deepcopy(node, child);
        #else
            Board* child = node;
        #endif
        Row row = board_move(child, move);
        Eval child_value;
        // Calculate the score for the child
        if (board_is_game_over(child) || (depth == 0)){
            if (board_is_game_over(child)){
                if (board_is_game_draw(child)){
                    child_value = 0;
                }else{
                    child_value = 1;
                }
            }else{
                child_value = 0;
            }
        }else{
            child_value = -_negamax(child, -β, -α);
        }
        #if DEEPCOPY != true
            board_unmove(child, row, move);
        #endif
        ttentry->moves[move] = child_value;
        value = max(value, child_value);
        α = max(α, value);
        if (α >= β){
            break;
        }
    }
    depth += 1;

    // Transposition Table Store; node is the lookup key for ttentry
    if (depth >= TT_DEPTH_THRESHOLD){
        if (value <= alpha_orig){
            ttentry->flag = TT_UPPERBOUND;
        }else if (value >= β){
            ttentry->flag = TT_LOWERBOUND;
        }else{
            ttentry->flag = TT_EXACT;
        }
        ttentry->depth = depth;
    }

    return value;
}


Column negamax(register Board* node){
    assert_false(board_is_game_over(node), "Game already over");
    assert_true(depth >= 1, "depth must be +ve");
    Eval eval = _negamax(node, NINF, PINF);
    TTEntry* ttentry = transpositiontable_get(tt, board_hash(node));
    if (ttentry == NULL){
        #if (DEBUG == true) || (DEPLOY == true)
            printf("eval=%i  depth=%i  \tlen_tt=%" PRIu64 "\n", eval, depth, len_transpositiontable(tt));
            fflush(stdout);
            fflush(NULL);
        #endif
        printf("[CRITICAL ERROR]: TT doesn't contain root node depth=%i\n", depth);
        fflush(stdout);
        return COLUMNS;
    }
    Column move = argmax(ttentry->moves);
    #if (DEBUG == true) || (DEPLOY == true)
        printf("eval=%i  move=%i  depth=%i  %s  len_tt=%" PRIu64 "\n", eval, move, depth, moves_to_string(ttentry->moves), len_transpositiontable(tt));
        fflush(stdout);
    #endif
    return move;
}


Column deepening_negamax(register Board* node, register const Depth maxdepth){
    Depth mindepth = (TT_DEPTH_THRESHOLD == 0) ? 1 : TT_DEPTH_THRESHOLD;
    assert_true(mindepth <= maxdepth, "Invalid TT_DEPTH_THRESHOLD, maxdepth combination");
    Column move = COLUMNS;
    for (depth=mindepth; depth<maxdepth+1; depth++){
        move = negamax(node);
    }
    return move;
}


#if TEST == true
struct TestStruct{
    Column moves[COLUMNS*ROWS];
    int moves_size;
    Column correct[COLUMNS];
    int correct_size;
};
typedef struct TestStruct Test;

void test(Test test){
    Board board;
    init_board(&board);
    for (int i=0; i<test.moves_size; i++){
        board_move(&board, test.moves[i]);
    }
    board_print(&board);
    for (depth=4; depth<=DEPTH; depth++){
        if (((TT_DEPTH_THRESHOLD == 0) ? 1 : TT_DEPTH_THRESHOLD) > depth){
            continue;
        }
        bool test_passed = false;
        #if DEEPENING == true
            Column answer = deepening_negamax(&board, depth);
        #else
            Column answer = negamax(&board);
        #endif
        for (int i=0; i<test.correct_size; i++){
            if (test.correct[i] == answer){
                test_passed = true;
            }
        }
        if (!test_passed){
            #if PRINT_STORED_POSITIONS == true
                transpositiontable_dump(tt);
            #endif
            printf("For that board ^, move=%i\n", answer);
            assert_true(test_passed, "Test failed");
        }

        del_transpositiontable(tt);
        tt = new_transpositiontable();
    }
}

int main(){
    tt = new_transpositiontable();
    test((Test) {{0,3,3,4,4,5,5,0,6}, 9, {2}, 1});
    test((Test) {{3,3,4,4,5,5,0,6}, 8, {2}, 1});
    test((Test) {{0,3,3,4}, 4, {2,5}, 2});
    test((Test) {{3,3,4}, 3, {2,5}, 2});
    puts("All tests passed");
    del_transpositiontable(tt);
    return 0;
}
#elif DEPLOY == true

#if DEEPENING == false
    #error "DEEPENING must be true otherwise nothing will be printed"
#endif

int main(int argc, char* argv[]){
    tt = new_transpositiontable();

    if (argc != 3){
        puts("Usage: bot <player1_bb> <player2_bb>");
        printf(">>> ");
        getchar();
        assert(0);
    }
    Board board;
    init_board(&board);
    board_load(&board, atoi_u64(argv[1]), atoi_u64(argv[2]));
    board_print(&board);
    deepening_negamax(&board, 64);
    del_transpositiontable(tt);
    return 0;
}
#else

int main(){
    tt = new_transpositiontable();

    Board board;
    init_board(&board);


    // 033445506 => 2
    // 33445506 => 2
    // 0334 => 25
    // 334 => 25

    // board_move(&board, 0);
    // board_move(&board, 3);
    // board_move(&board, 3);
    // board_move(&board, 4);

    board_move(&board, 0);
    board_move(&board, 3);
    board_move(&board, 3);
    board_move(&board, 4);
    board_move(&board, 4);
    board_move(&board, 5);
    board_move(&board, 5);
    board_move(&board, 0);
    board_move(&board, 6);

    board.player1_bb = 4398587592704;
    board.player2_bb = 72057628668198915;

    // board_move(&board, 6);
    // board_move(&board, 3);
    // board_move(&board, 3);
    // board_move(&board, 2);
    // board_move(&board, 2);
    // board_move(&board, 1);
    // board_move(&board, 1);
    // board_move(&board, 6);
    // board_move(&board, 0);
    board_print(&board);


    clock_t begin = clock();
    #if DEEPENING == true
        Column move = deepening_negamax(&board, DEPTH);
    #else
        depth = DEPTH;
        Column move = negamax(&board);
    #endif
    clock_t end = clock();
    printf("move=%i\n", move);

    printf("Took %f  visited=%lu  tt_len=%" PRIu64 "\n", (double)(end - begin) / CLOCKS_PER_SEC, nodes_visited, len_transpositiontable(tt));

    #if PRINT_STORED_POSITIONS == true
        transpositiontable_dump(tt);
    #endif
    del_transpositiontable(tt);
    printf(">>> ");
    getchar();

    // ttentry_print(4432704077954ULL);
    return 0;
}
#endif

// ./bot 4399141240981 72057697932967978