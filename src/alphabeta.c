#include <inttypes.h>
#include <stdint.h>
#include <string.h>
#include <signal.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>

#include <unistd.h>
#include <execinfo.h>
#include <sys/wait.h>
#include <sys/prctl.h>

#include "utils.h"
#include "transposition_table.h"

#define MIN_REQUIRED_TT_NODES 0
#define TT_DEPTH_THRESHOLD 0
#define CHECK_FORCED
#define CHECK_WIN
#define DEEPCOPY
#define USE_NN

#define DEEPENING
#define DEPTH 11

#define DEPLOY
// #define TEST

#define TEST_ALL_DEPTHS
// #define DEBUG

#define TRACEBACK
// #define ADV_TRACEBACK

// #define INT_TRACEBACK
#define SEGV_TRACEBACK


#if defined(DEPLOY) && defined(TEST)
    #error "Can't deploy and test at the same time"
#endif

#if defined(USE_NN)
#include "ai/ai_core.h"
#endif

clock_t begin;
Nodes nodes_visited = 0;
restrict TranspositionTable tt;
Depth _depth;


// ================================== Helpers ==================================
Eval min(register const Eval a, register const Eval b){
    return (a < b) ? a : b;
}
Eval max(register const Eval a, register const Eval b){
    return (a > b) ? a : b;
}


#if defined(CHECK_WIN) || defined(CHECK_FORCED)
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

#if defined(CHECK_FORCED)
Column get_forced_move(register Board* board){
    board_switch_players(board);
    Column column = get_winning_move(board);
    board_switch_players(board);
    return column;
}
#endif

void ttentry_print(register const BoardHash hash){
    TTEntry* ttentry = transpositiontable_get(tt, hash);
    if (ttentry == NULL){
        #if !defined(DEBUG)
        printf("\r\x1b[0K");
        #endif
        puts("[DEBUG]: (null)");
    }else{
        #if !defined(DEBUG)
        printf("\r\x1b[0K");
        #endif
        printf("[DEBUG]: depth=%i  flag=%i  best_move=%i  eval=%i\n", pttentry_get_depth(ttentry), pttentry_get_flag(ttentry), ttentry->best_move, ttentry->eval);
    }
}


// ================================ Driver Code ================================
extern Eval _negamax(register Board* node, register Eval α, register Eval β){
    Nodes tmp_nodes_number;
    TTEntry real_ttentry;
    bool should_add_to_tt = false;
    const BoardHash node_hash = board_hash(node);
    Eval alpha_orig = α;
    nodes_visited++;

    // Transposition Table Lookup; node is the lookup key for ttentry
    TTEntry* ttentry = transpositiontable_get(tt, node_hash);
    if (ttentry == NULL){
        init_ttentry(&real_ttentry, _depth);
        should_add_to_tt = true;
        tmp_nodes_number = nodes_visited;
        ttentry = &real_ttentry;
    }else{
        tmp_nodes_number = nodes_visited;
        #if defined(USE_REPLACE)
        ttentry->nodes = 0xffffffff;
        #endif
    }

    if ((pttentry_get_flag(ttentry) != TT_UNDEFINED) && (pttentry_get_depth(ttentry) >= _depth)){
        assert_false(pttentry_get_depth(ttentry) > _depth, "Should never happen");
        if (pttentry_get_flag(ttentry) == TT_EXACT){
            return ttentry->eval;
        }else if (pttentry_get_flag(ttentry) == TT_LOWERBOUND){
            α = max(α, ttentry->eval);
        }else if (pttentry_get_flag(ttentry) == TT_UPPERBOUND){
            β = min(β, ttentry->eval);
        }
        if (α >= β){
            return ttentry->eval;
        }
    }
    // Check wining/forced moves:
    #if defined(CHECK_WIN)
        if (pttentry_get_flag(ttentry) == TT_UNDEFINED){
            ttentry->best_move = get_winning_move(node);
            #if defined(CHECK_FORCED)
            if (ttentry->best_move == COLUMNS){
                ttentry->best_move = get_forced_move(node);
            }
            #endif
        }
    #elif defined(CHECK_FORCED)
        ttentry->best_move = get_forced_move(node);
    #endif
    // if (ttentry->best_move == COLUMNS){
    //     if (board_is_move_possible(node, 3)){
    //         ttentry->best_move = 3;
    //     }else if (board_is_move_possible(node, 2)){
    //         ttentry->best_move = 2;
    //     }else if (board_is_move_possible(node, 4)){
    //         ttentry->best_move = 4;
    //     }else if (board_is_move_possible(node, 1)){
    //         ttentry->best_move = 1;
    //     }else if (board_is_move_possible(node, 5)){
    //         ttentry->best_move = 5;
    //     }
    // }

    // Actual minimax with alphabeta pruning
    Eval value = NINF;
    _depth--;
    for (Column idx=COLUMNS; idx<=COLUMNS; idx--){
        Column move = idx;
        if (move == COLUMNS){
            move = ttentry->best_move;
        }else if (move == ttentry->best_move){
            continue;
        }
        if (!board_is_move_possible(node, move)){
            continue;
        }
        #if defined(DEEPCOPY)
            Board real_child;
            Board* child = &real_child;
            board_deepcopy(node, child);
            board_move(child, move);
        #else
            Board* child = node;
            Row row = board_move(child, move);
        #endif
        register Eval child_value;
        // Calculate the score for the child
        if (board_is_game_over(child) || (_depth == 0)){
            if (board_is_game_over(child)){
                if (board_is_game_draw(child)){
                    child_value = 0;
                }else{
                    child_value = 50 + (Eval)_depth;
                }
            }else{
                #if defined(USE_NN)
                Vector encoded = (Vector)(Scalar[128]){};
                nn_encode(encoded, node->player1_bb, node->player2_bb,
                          board_get_player(node));
                child_value = (Eval)(nn_network(encoded)*30.0*
                                     ((board_get_player(node) != 0)*2-1.0));
                #else
                child_value = 0;
                #endif
            }
        }else{
            child_value = -_negamax(child, -β, -α);
        }
        #if !defined(DEEPCOPY)
        board_unmove(child, row, move);
        #endif
        if (child_value > value){
            ttentry->eval = child_value;
            ttentry->best_move = move;
            value = child_value;
        }
        α = max(α, value);
        if (α >= β){
            break;
        }
    }
    _depth++;
    // Transposition Table Store; node is the lookup key for ttentry
    #if TT_DEPTH_THRESHOLD == 0
    {
    #else
    if (_depth >= TT_DEPTH_THRESHOLD){
    #endif
        uint8_t flag;
        if (value <= alpha_orig){
            flag = TT_UPPERBOUND;
        }else if (value >= β){
            flag = TT_LOWERBOUND;
        }else{
            flag = TT_EXACT;
        }
        pttentry_set_flag_depth(ttentry, flag, _depth);
        tmp_nodes_number = nodes_visited - tmp_nodes_number;
        if (should_add_to_tt){
            if (tmp_nodes_number > MIN_REQUIRED_TT_NODES){
                #if defined(USE_REPLACE)
                ttentry->nodes = tmp_nodes_number;
                #endif
                transpositiontable_add(tt, node_hash, real_ttentry);
                // ttentry is useless after this point
            }
        }else{
            #if defined(USE_REPLACE)
            ttentry->nodes = tmp_nodes_number;
            #endif
        }
    }

    return value;
}

Column negamax(register Board* node, register const Depth new_depth){
    if (transpositiontable_get(tt, board_hash(node)) == NULL){
        TTEntry real_ttentry;
        init_ttentry(&real_ttentry, 0);
        transpositiontable_add(tt, board_hash(node), real_ttentry);
    }

    assert_false(board_is_game_over(node), "Game already over");
    assert_true(new_depth >= 1, "depth must be +ve");

    _depth = new_depth;
    Eval eval = _negamax(node, NINF, PINF);
    TTEntry* ttentry = transpositiontable_get(tt, board_hash(node));

    if (ttentry == NULL){
        #if !defined(DEBUG)
        printf("\r\x1b[0K");
        #endif
        printf("[OUTPUT]: eval=%i  depth=%i  len_tt=%"PRIu64"\n", eval, new_depth, len_transpositiontable(tt));
        printf("[ERROR]: TT doesn't contain root node depth=%i\n", new_depth);
        fflush(stdout);
        return COLUMNS;
    }else{
        Column move = ttentry->best_move;
        double time_delta = (double)((double)(clock()-begin) / (double)CLOCKS_PER_SEC);
        double kns = (double)(nodes_visited)/time_delta/1000;
        #if !defined(DEBUG)
        printf("\r\x1b[0K");
        #endif
        printf("[OUTPUT]: eval=%i  move=%i  depth=%i  %.2fkn/s  len_tt=%"PRIu64, eval, move, new_depth, kns, len_transpositiontable(tt));
        #if defined(DEBUG)
        printf("\n");
        #endif
        fflush(stdout);
        return move;
    }
}

Column deepening_negamax(register Board* node, register Depth maxdepth){
    const Depth max_possible_depth = board_num_empty_squares(node);
    if (max_possible_depth == 1){
        for (Column move=0; move<COLUMNS; move++){
            if (board_is_move_possible(node, move)){
                return move;
            }
        }
    }
    maxdepth = (max_possible_depth < maxdepth) ? max_possible_depth : maxdepth;
    // assert_true(4 <= maxdepth, "Invalid maxdepth - must be >= 8");
    Column move = COLUMNS;
    for (Depth depth=2; depth<maxdepth+1; depth++){
        move = negamax(node, depth);
    }
    return move;
}


// =================================== Init ====================================
#if defined(TRACEBACK)
void print_trace_simple(){
    void* array[100];
    int size = backtrace(array, 100);
    backtrace_symbols_fd(array, size, STDERR_FILENO);
}
#undef PRINT_TRACE_FUNC
#define PRINT_TRACE_FUNC print_trace_simple
#endif
#if defined(ADV_TRACEBACK)
void print_trace_adv(){
    char pid_buf[30];
    sprintf(pid_buf, "%d", getpid());
    char name_buf[512];
    name_buf[readlink("/proc/self/exe", name_buf, 511)] = 0;
    prctl(PR_SET_PTRACER, PR_SET_PTRACER_ANY, 0, 0, 0);
    int child_pid = fork();
    if (!child_pid){
        dup2(2, 1); // redirect output to stderr - edit: unnecessary?
        execl("/usr/bin/gdb", "gdb", "--batch", "-n", "-ex", "thread", "-ex",
              "bt", name_buf, pid_buf, NULL);
        abort();
    }else{
        waitpid(child_pid, NULL, 0);
    }
}
#undef PRINT_TRACE_FUNC
#define PRINT_TRACE_FUNC print_trace_adv
#endif
#if defined(TRACEBACK) && defined(ADV_TRACEBACK)
void print_trace_both(){
    print_trace_simple();
    fprintf(stderr, "\n========== ADV_TRACEBACK ==========\n");
    print_trace_adv();
}
#undef PRINT_TRACE_FUNC
#define PRINT_TRACE_FUNC print_trace_both
#endif
void cleanup(){
    if (tt == NULL){return;}
    sigset_t block_set;
    sigemptyset(&block_set);
    sigaddset(&block_set, SIGINT);
    sigprocmask(SIG_BLOCK, &block_set, NULL);
    del_transpositiontable(tt);
    tt = NULL;
    sigprocmask(SIG_UNBLOCK, &block_set, NULL);
}
void keyboardinterrupt_handler(int signal){
    printf("\r\n\x1b[0K\x1b[96m[SIGNAL]: KeyboardInterrupt(%i)\x1b[0m\n", signal);
    #if defined(INT_TRACEBACK)
    PRINT_TRACE_FUNC();
    #endif
    cleanup();
    exit(0);
}
void segv_handler(int signal){
    fprintf(stderr, "\r\x1b[0K\x1b[91mError: signal %d:\x1b[0m\n", signal);
    #if defined(SEGV_TRACEBACK)
    PRINT_TRACE_FUNC();
    #endif
    cleanup();
    exit(1);
}
void init(){
    signal(SIGINT, keyboardinterrupt_handler);
    signal(SIGSEGV, segv_handler);

    begin = clock();
    tt = new_transpositiontable();
    printf("\r\x1b[0K[TIME]: Creating TT of size %i ", TTSIZE);
    printf("took %.2fsec\n", (double)(clock()-begin)/CLOCKS_PER_SEC);
    begin = clock();
}


// =================================== Mains ===================================
#if defined(TEST)
typedef struct{
    Column moves[COLUMNS*ROWS];
    int moves_size;
    Column correct[COLUMNS];
    int correct_size;
} Test;

void test(Test test){
    Board board;
    init_board(&board);
    for (int i=0; i<test.moves_size; i++){
        board_move(&board, test.moves[i]);
    }
    #if !defined(DEBUG)
    printf("\r\x1b[0K");
    #endif
    board_print(&board);
    for (Depth depth=4; depth<=DEPTH; depth++){
        #if !defined(TEST_ALL_DEPTHS)
        if (depth != DEPTH){continue;}
        #endif
        #if TT_DEPTH_THRESHOLD != 0
        if (TT_DEPTH_THRESHOLD > depth){continue;}
        #endif
        bool test_passed = false;
        #if defined(DEEPENING)
        Column answer = deepening_negamax(&board, depth);
        #else
        Column answer = negamax(&board, depth);
        #endif
        for (int i=0; i<test.correct_size; i++){
            if (test.correct[i] == answer){
                test_passed = true;
            }
        }
        if (!test_passed){
            #if !defined(DEBUG)
            printf("\r\x1b[0K");
            #endif
            printf("\x1b[91m[TEST]: For that board ^, move=%i\x1b[0m\n", answer);
            assert_true(test_passed, "Test failed");
        }
        transpositiontable_clear(tt);
    }
}

int main(){
    init();

    test((Test) {{0,3,3,4,4,5,5,0,6}, 9, {2}, 1});
    test((Test) {{3,3,4,4,5,5,0,6}, 8, {2}, 1});
    test((Test) {{0,3,3,4}, 4, {2,5}, 2});
    test((Test) {{3,3,4}, 3, {2,5}, 2});
    #if !defined(DEBUG)
    printf("\r\x1b[0K");
    #endif
    puts("\x1b[92m[TEST]: All normal tests passed\x1b[0m");
    printf("[TIME]: Took %.2f sec\n", (double)(clock()-begin)/CLOCKS_PER_SEC);

    // printf("\033c");
    // TTEntry real_ttentry;
    // for (uint64_t i=0; i < 10000000ULL; i++){
    //     init_ttentry(&real_ttentry, i&63);
    //     transpositiontable_add(tt, i*1000, real_ttentry);
    // }
    // transpositiontable_dump_limit(tt, 15);

    printf(">>> ");
    getchar();
    cleanup();
    return 0;
}


#elif defined(DEPLOY)
#if !defined(DEEPENING)
#error "Implement: (DEEPENING == false) && (DEPLOY == true)"
#endif
int main(int argc, char* argv[]){
    init();
    Board board;
    init_board(&board);

    printf("\r\x1b[0K[DEBUG]: sizeof{void*=%zu, ", sizeof(void*));
    printf("TTEntry=%zu, ", sizeof(TTEntry));
    printf("TTBucketEntry=%zu  ", sizeof(TTBucketEntry));
    printf("TTEntry=%zu}\n", sizeof(TTEntry));

    if (argc != 3){
        puts("[ERROR]: Usage: bot <player1_bb> <player2_bb>");
        if (sizeof(void*) == 8){
            printf(">>> ");
            getchar();
        }
        return 1;
    }
    board_load(&board, atoi_u64(argv[1]), atoi_u64(argv[2]));
    // board.player1_bb = 186171037242624;
    // board.player2_bb = 72150681638996609;

    // Print board and start iterative deepening
    board_print(&board);
    printf("[DEBUG]: maxdepth=%i\n", board_num_empty_squares(&board));
    Column move = deepening_negamax(&board, board_num_empty_squares(&board));
    printf("\n[FINAL]: move=%i  len_tt=%"PRIu64, move, len_transpositiontable(tt));
    printf("\n\x1b[92m[END]: Done\x1b[0m\n");
    cleanup();
    return 0;
}


#else
int main(){
    init();

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

    // board_move(&board, 0);
    // board_move(&board, 3);
    // board_move(&board, 3);
    // board_move(&board, 4);
    // board_move(&board, 4);
    // board_move(&board, 5);
    // board_move(&board, 5);
    // board_move(&board, 0);
    // board_move(&board, 6);

    // board_move(&board, 3);
    // board_move(&board, 3);
    // board_move(&board, 3);
    // board_move(&board, 3);
    // board_move(&board, 3);
    // board_move(&board, 3);
    // board_move(&board, 4);
    // board_move(&board, 5);
    // board_move(&board, 1);
    // board_move(&board, 2);
    // board_move(&board, 2);

    // board.player1_bb = 4398587592704;
    // board.player2_bb = 72057628668198915;

    // board.player1_bb = 241906894275585;
    // board.player2_bb = 72092815477394310;

    // board.player1_bb = 4432423059584;
    // board.player2_bb = 72057594321125376;

    // board.player1_bb = 72057594081968128;
    // board.player2_bb = 20971520;

    // board.player1_bb = 30889408987144;
    // board.player2_bb = 72092779217420311;

    // board.player1_bb = 186171037242624;
    // board.player2_bb = 72150681638996609;

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
    printf("%li %li\n", board.player1_bb, board.player2_bb);

    // /*
    #if defined(DEEPENING)
        Column move = deepening_negamax(&board, DEPTH);
        #if !defined(DEBUG)
        printf("\n");
        #endif
    #else
        Column move = negamax(&board, DEPTH);
    #endif
    #if !defined(DEBUG)
    printf("\r\x1b[0K");
    #endif
    printf("[OUTPUT]: move=%i\n", move);
    printf("[TIME]: Took %.2fsec  visited=%"PRIu32"  tt_len=%"PRIu64"\n", (double)(clock()-begin)/CLOCKS_PER_SEC, nodes_visited, len_transpositiontable(tt));

    printf(">>> ");
    getchar();
    // */
    /*
    for (int i=0; i<7; i++){
        board_move(&board, i);
        board_print(&board);
        Vector encoded = (Vector)(Scalar[128]){};
        nn_encode(encoded, board.player1_bb, board.player2_bb,
                  board_get_player(&board));
        // print_vector(encoded, 128);
        Eval child_value = (Eval)(-nn_network(encoded)*(Scalar)(50));
        printf("%i\n", child_value);
        board_unmove(&board, 0, i);
    }
    // */

    cleanup();
    return 0;
}
#endif

// ./bot 4399141240981 72057697932967978
// https://connect4.gamesolver.org/?pos=4444446375535777