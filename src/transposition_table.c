#include <inttypes.h>
#include <stdlib.h>
#include <assert.h>
#include <stdio.h>
#include "transposition_table.h"


// ================================== TTEntry ==================================
void init_ttentry(register TTEntry* ttentry, register const Depth depth){
    pttentry_set_flag_depth(ttentry, TT_UNDEFINED, depth);
    ttentry->best_move = COLUMNS;
    ttentry->eval = NINF;
    #if defined(USE_REPLACE)
    ttentry->nodes = 0;
    #endif
}


// =============================== TTBucketEntry ===============================
void init_ttbucketentry(register TTBucketEntry* ttbucketentry, register const BoardHash hash, register const TTEntry ttentry){
    ttbucketentry->hash = hash;
    ttbucketentry->ttentry = ttentry;
}


// ================================== TTBucket =================================
void init_ttbucket(register TTBucket* ttbucket){
    ttbucket->size = 0;
    ttbucket->space = 5;
    ttbucket->entries = (TTBucketEntry**) calloc(ttbucket->space, sizeof(TTBucketEntry*));
    assert_false(ttbucket->entries == NULL, "malloc failed");
}

void del_ttbucket(register TTBucket* ttbucket){
    for (BucketSize i=0; i<ttbucket->size; i++){
        free(ttbucket->entries[i]);
    }
    free(ttbucket->entries);
}

void ttbucket_add(register TTBucket* ttbucket, register const BoardHash hash, register const TTEntry ttentry){
    #if defined(USE_REPLACE)
    #error "Implement this"
    // https://www.researchgate.net/publication/2737817_Replacement_Schemes_for_Transposition_Tables
    if (ttbucket->size >= OPTIMAL_TT_BUCKET_SIZE){
        BucketSize orig_loc = (BucketSize)(ttbucket->entries[i]->hash % (BoardHash)(ttbucket->space));
        BucketSize loc = orig_loc;
        while (ttbucket->entries[loc] != NULL){
            if (ttbucket->entries[loc]->ttentry.nodes < ttentry.nodes){
                ttbucket->entries[loc]->ttentry = ttentry;
                ttbucket->entries[loc]->hash = hash;
                return;
            }
            loc = ((BucketSize)(loc+1)) % ttbucket->space;
            if (loc == orig_loc){
                break;
            }
        }
    }
    #endif
    if (ttbucket->size == ttbucket->space){
        // Grow ttbucket->entries by 1.25x
        BucketSize new_space = ttbucket->space + (ttbucket->space >> 1);
        if (new_space <= ttbucket->space){
            puts("[ERROR]: TTBucket overflow error");
            assert_true(0, "Not adding due to ^ error");
            return;
        }
        // Copy ttbucket->entries and free the old
        TTBucketEntry** old = ttbucket->entries;
        ttbucket->entries = (TTBucketEntry**) calloc(new_space, sizeof(TTBucketEntry*));
        assert_false(ttbucket->entries == NULL, "malloc failed");
        for (BucketSize i=0; i<ttbucket->space; i++){
            if (old[i] == NULL){continue;}
            BucketSize loc = (BucketSize)(old[i]->hash % (BoardHash)(new_space));
            while (ttbucket->entries[loc] != NULL){
                loc = ((BucketSize)(loc+1)) % new_space;
            }
            ttbucket->entries[loc] = old[i];
        }
        free(old);
        ttbucket->space = new_space;
    }
    // Create a TTBucketEntry
    TTBucketEntry* ttbucketentry = (TTBucketEntry*) malloc(sizeof(TTBucketEntry));
    assert_false(ttbucketentry == NULL, "malloc failed");
    init_ttbucketentry(ttbucketentry, hash, ttentry);
    BucketSize loc = (BucketSize)(hash % (BoardHash)(ttbucket->space));
    while (ttbucket->entries[loc] != NULL){
        loc = ((BucketSize)(loc+1)) % ttbucket->space;
    }
    // Add the ttbucketentry
    ttbucket->entries[loc] = ttbucketentry;
    ttbucket->size++;
}

TTEntry* ttbucket_get(register const TTBucket* ttbucket, register const BoardHash hash){
    BucketSize orig_loc = (BucketSize)(hash % (BoardHash)(ttbucket->space));
    BucketSize loc = orig_loc;
    while (ttbucket->entries[loc] != NULL){
        if (ttbucket->entries[loc]->hash == hash){
            return &ttbucket->entries[loc]->ttentry;
        }
        loc = ((BucketSize)(loc+1)) % ttbucket->space;
        if (loc == orig_loc){
            break;
        }
    }
    return NULL;
}


// ============================= TranspositionTable ============================
TranspositionTable new_transpositiontable(){
    TTBucket* tt = (TTBucket*) malloc(TTSIZE*sizeof(TTBucket));
    assert_false(tt == NULL, "malloc failed");
    for (TTSize i=0; i<TTSIZE; i++){
        init_ttbucket(&tt[i]);
    }
    return tt;
}

void del_transpositiontable(register TranspositionTable tt){
    for (TTSize i=0; i<TTSIZE; i++){
        del_ttbucket(&tt[i]);
    }
    free(tt);
}

void transpositiontable_clear(register TranspositionTable tt){
    for (TTSize i=0; i<TTSIZE; i++){
        del_ttbucket(&tt[i]);
        init_ttbucket(&tt[i]);
    }
}

void transpositiontable_add(register TranspositionTable tt, register const BoardHash hash, register const TTEntry ttentry){
    ttbucket_add(&tt[hash%TTSIZE], hash, ttentry);
}

TTEntry* transpositiontable_get(register TranspositionTable tt, register const BoardHash hash){
    return ttbucket_get(&tt[hash%TTSIZE], hash);
}

uint64_t len_transpositiontable(register const TranspositionTable tt){
    uint64_t output = 0;
    for (TTSize i=0; i<TTSIZE; i++){
        output += tt[i].size;
    }
    return output;
}

void transpositiontable_dump(register const TranspositionTable tt){
    for (TTSize i=0; i<TTSIZE; i++){
        const BucketSize bucket_size = tt[i].size;
        if (tt[i].size != 0){
            printf("Switching to bucket %i:\n", i);
        }
        for (BucketSize j=0; j<bucket_size; j++){
            printf("\tflag=%i  hash=%" PRIu64 "  depth=%i\n", ttentry_get_flag(tt[i].entries[j]->ttentry), tt[i].entries[j]->hash, ttentry_get_depth(tt[i].entries[j]->ttentry));
        }
    }
}

void transpositiontable_dump_limit(register const TranspositionTable tt, register const BucketSize limit){
    for (TTSize i=0; i<TTSIZE; i++){
        const BucketSize bucket_size = tt[i].size;
        if (tt[i].size >= limit){
            printf("Switching to bucket %i:\n", i);
            for (BucketSize j=0; j<bucket_size; j++){
                printf("\tflag=%i  hash=%" PRIu64 "  depth=%i\n", ttentry_get_flag(tt[i].entries[j]->ttentry), tt[i].entries[j]->hash, ttentry_get_depth(tt[i].entries[j]->ttentry));
            }
        }
    }
}


/*
int main(){
    TTEntry ttentry1, ttentry2;
    TTEntry* entry_result;
    Board board;

    init_board(&board);
    board_move(&board, 1);
    board_print(&board);
    init_ttentry(&ttentry1, 6);
    init_ttentry(&ttentry2, 7);

    TranspositionTable tt = new_transpositiontable();

    entry_result = transpositiontable_add(tt, board_hash(&board), ttentry1);
    entry_result = transpositiontable_get(tt, board_hash(&board));
    assert_false(entry_result == NULL, ".get shouldn't return NULL just after .add");
    printf("%i\n", entry_result->depth);

    // board_move(&board, 1);
    entry_result = transpositiontable_add(tt, board_hash(&board), ttentry2);
    // entry_result = transpositiontable_get(tt, board_hash(&board));
    assert_false(entry_result == NULL, ".get shouldn't return NULL just after .add");
    printf("%i\n", entry_result->depth);

    del_transpositiontable(tt);
    return 0;
}
// */