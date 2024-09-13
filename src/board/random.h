#include <stdlib.h>
#include <time.h>

#define random_double() ((double)rand() / RAND_MAX)

#define random_float() ((float)((double)rand() / RAND_MAX))

#define init_random() srand((unsigned int)time(NULL))

#define randint(lower, upper) ({ \
    register const int _randint_lower = (lower); \
    register const int _randint_upper = (upper); \
    (rand()%(_randint_upper-_randint_lower+1))-_randint_lower; \
})

#define random_from_dist(dist, size) ({ \
    register const double* _random_from_dist_dist = (dist); \
    register const int _random_from_dist_size = (size); \
    register double _random_from_dist_sum = random_double(); \
    register int _random_from_dist_i=0; \
    for (;_random_from_dist_i<=_random_from_dist_size-1; _random_from_dist_i++){ \
        _random_from_dist_sum -= _random_from_dist_dist[_random_from_dist_i]; \
        if ((_random_from_dist_i == _random_from_dist_size-1) || (_random_from_dist_sum < 0)){ \
            break; \
        } \
    } \
    _random_from_dist_i; \
})

/*
float random_float(){
    return (float)((double)rand() / RAND_MAX);
}

double random_double(){
    return (double)rand() / RAND_MAX;
}

void init_random(){
    srand((unsigned int)time(NULL));
}

int randint(register const int lower, register const int upper){
    return (rand()%(upper-lower+1))-lower;
}

int random_from_dist(register const double* dist, register const int size){
    register double sum = random_double();
    for (register int i=0; i<size; i++){
        sum -= dist[i];
        if (sum < 0){
            return i;
        }
    }
    return size-1;
}
*/