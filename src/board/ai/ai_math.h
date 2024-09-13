#pragma once
#include <stdint.h>
#include <stdio.h>
#include <math.h>

#define _min(x, y) (((x)<(y))?(x):(y))
#define _max(x, y) (((x)>(y))?(x):(y))
#define _tanh(x) _min(0.997f, _max(-0.997f, \
                                   (x)*(135550+7785*(x)*(x))/(134700+53200*(x)*(x))))
#define _relu(x) (_max(0, (x)))
#define mat_index2(a,b,i,j) ((i)*(b) + (j))
#define mat_index3(a,b,c,i,j,k) ((i)*(b)*(c) + (j)*(c) + (k))
#define mat_index4(a,b,c,d,i,j,k,l) ((i)*(b)*(c)*(d) + (j)*(c)*(d) + (k)*(d) + (l))

typedef float Scalar;
typedef uint16_t VSize;
#define Vector Scalar*
#define Matrix Vector
#define Tensor Vector
#define Kernel Matrix

#define i_tanh(v, a) { \
    register Vector _r_matmul_v = (v); \
    register const VSize _r_matmul_a = (a); \
    for (register VSize _r_matmul_i=0; _r_matmul_i<_r_matmul_a; _r_matmul_i++){ \
        _r_matmul_v[_r_matmul_i] = (Scalar) tanh((double)(_r_matmul_v[_r_matmul_i])); \
    } \
}

#define i_relu(v, a) { \
    register Vector _r_matmul_v = (v); \
    register const VSize _r_matmul_a = (a); \
    for (register VSize _r_matmul_i=0; _r_matmul_i<_r_matmul_a; _r_matmul_i++){ \
        register const Scalar _r_matmul_t = _r_matmul_v[_r_matmul_i]; \
        _r_matmul_v[_r_matmul_i] = _relu(_r_matmul_t); \
    } \
}

#define i_add(v1, v2, a) { \
    register Vector _i_add_v1 = (v1); \
    register const Vector const _i_add_v2 = (v2); \
    register const VSize const _i_add_a = (a); \
    for (register VSize _i_add_i=0; _i_add_i<_i_add_a; _i_add_i++){ \
        _i_add_v1[_i_add_i] += _i_add_v2[_i_add_i]; \
    } \
}

#define dot(v1, v2, a) ({ \
    register const Vector const _dot_v1 = (v1); \
    register const Vector const _dot_v2 = (v2); \
    register const VSize _dot_a = (a); \
    register Scalar _dot_output = 0; \
    for (register VSize _dot_i=0; _dot_i<_dot_a; _dot_i++){ \
        _dot_output += (Scalar)(_dot_v1[_dot_i] * _dot_v2[_dot_i]); \
    } \
    _dot_output; \
})

#define r_matmul(r, m, v, a, b) { \
    register Vector _r_matmul_r = (r); \
    register const Matrix const _r_matmul_m = (m); \
    register const Vector const _r_matmul_v = (v); \
    register const VSize _r_matmul_a = (a); \
    register const VSize _r_matmul_b = (b); \
    for (register VSize _r_matmul_i=0; _r_matmul_i<_r_matmul_a; _r_matmul_i++){ \
        _r_matmul_r[_r_matmul_i] = dot(&_r_matmul_m[mat_index2(_r_matmul_a,_r_matmul_b,_r_matmul_i,0)], _r_matmul_v, _r_matmul_b); \
    } \
}

#define sum(v, a) ({ \
    register const Vector const _sum_v = (v); \
    register const VSize _sum_a = (a); \
    register Scalar _sum_result = 0; \
    for (register VSize _sum_i=0; _sum_i<_sum_a; _sum_i++){ \
        _sum_result += _sum_v[_sum_i]; \
    } \
    _sum_result; \
})

#define print_vector(v, a) { \
    register const Vector const _print_vector_v = (v); \
    register const VSize _print_vector_a = (a); \
    printf("["); \
    for (register VSize _print_vector_i=0; _print_vector_i<_print_vector_a; _print_vector_i++){ \
        if (_print_vector_v[_print_vector_i] == 0){ \
            printf("0.0"); \
        }else{ \
            printf("%.8f", _print_vector_v[_print_vector_i]); \
        } \
        if (_print_vector_i != _print_vector_a-1){ \
            printf(", "); \
        } \
    } \
    printf("]\n"); \
}

/*
void i_tanh(register Vector v, register const VSize a){
    for (register VSize i=0; i<a; i++){
        v[i] = (Scalar) tanh((double)(v[i]));
    }
}

void i_relu(register Vector v, register const VSize a){
    for (register VSize i=0; i<a; i++){
        v[i] = _relu(v[i]);
    }
}

void i_add(register Vector v1, register const Vector v2, register const VSize a){
    for (register VSize i=0; i<a; i++){
        v1[i] += v2[i];
    }
}

Scalar dot(register const Vector v1, register const Vector v2, register const VSize a){
    Scalar output = 0;
    for (register VSize i=0; i<a; i++){
        output += (Scalar)(v1[i] * v2[i]);
    }
    return output;
}

void r_matmul(register Vector r, register const Matrix m, register const Vector v, register const VSize a, register const VSize b){
    for (register VSize i=0; i<a; i++){
        r[i] = dot(&m[mat_index2(a,b,i,0)], v, b);
    }
}

Scalar sum(register const Vector v, register const VSize a){
    Scalar result = 0;
    for (register VSize i=0; i<a; i++){
        result += v[i];
    }
    return result;
}

void print_vector(register const Vector v, register const VSize a){
    printf("[");
    for (register VSize i=0; i<a; i++){
        if (v[i] == 0){
            printf("0.0");
        }else{
            printf("%.8f", v[i]);
        }
        if (i != a-1){
            printf(", ");
        }
    }
    printf("]\n");
}

void print_matrix(register const Matrix m, register const VSize a, register const VSize b){
    printf("Matrix[\n");
    for (register VSize i=0; i<a; i++){
        printf("  ");
        print_vector(&m[mat_index2(a,b,i,0)], b);
    }
    printf("]\n");
}
*/
