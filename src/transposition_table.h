#pragma once
#include <stdint.h>
#include "utils.h"
#include "board.h"

//59369,65521,118739,237487,312677,787331,1000003,1048573,1409237,1500007 (type=TTSize)
#define TTSIZE 1048573
// #define USE_REPLACE
#define OPTIMAL_TT_BUCKET_SIZE 5

#define PINF (0b01111111)
#define NINF (-PINF)
#define TT_UNDEFINED 0
#define TT_EXACT 1
#define TT_LOWERBOUND 2
#define TT_UPPERBOUND 3 // Note must fit in 2 bits
#define TT_FLAG_MASK 3

typedef uint8_t BucketSize;
typedef uint32_t TTSize;
typedef uint32_t Nodes;
typedef uint8_t Depth;
typedef int8_t Eval;

// TTEntry
typedef struct{
    uint8_t flag_depth;
    Column best_move;
    #if defined(USE_REPLACE)
    Nodes nodes;
    #endif
    Eval eval;
} __attribute__((packed)) TTEntry;

#define ttentry_get_flag(ttentry) ((ttentry).flag_depth & TT_FLAG_MASK)
#define pttentry_get_flag(ttentry) ((ttentry)->flag_depth & TT_FLAG_MASK)
#define ttentry_get_depth(ttentry) ((ttentry).flag_depth >> 2)
#define pttentry_get_depth(ttentry) ((ttentry)->flag_depth >> 2)

#define pttentry_set_flag_depth(ttentry, flag, depth) {\
    assert_true(depth < 64, "flag_depth overflowed"); \
    assert_true(flag <= TT_FLAG_MASK, "flag too big"); \
    ttentry->flag_depth = (uint8_t)(((uint8_t)flag) | (((uint8_t)depth) << 2));}

// TTBucketEntry
typedef struct{
    BoardHash hash;
    TTEntry ttentry;
} __attribute__((packed)) TTBucketEntry;

// TTBucket
typedef struct{
    BucketSize size;
    BucketSize space;
    TTBucketEntry** entries;
} __attribute__((packed)) TTBucket;

// TT
typedef TTBucket* TranspositionTable;


void init_ttentry(register TTEntry* ttentry, register const Depth depth);

// Only use these functions:
TranspositionTable new_transpositiontable();
void del_transpositiontable(register TranspositionTable tt);
void transpositiontable_clear(register TranspositionTable tt);
void transpositiontable_add(register TranspositionTable tt, register const BoardHash hash, register const TTEntry ttentry);
TTEntry* transpositiontable_get(register const TranspositionTable tt, register const BoardHash hash);
bool transpositiontable_contains(register const TranspositionTable tt, register const BoardHash hash);
uint64_t len_transpositiontable(register const TranspositionTable tt);
void transpositiontable_dump(register const TranspositionTable tt);
void transpositiontable_dump_limit(register const TranspositionTable tt, register const BucketSize limit);


#if PINF <= 0
#error "Invalid (PINF <= 0)"
#endif
#if PINF != -NINF
#error "Invalid (PINF != -NINF)"
#endif