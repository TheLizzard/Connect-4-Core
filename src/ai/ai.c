#include <stdlib.h>
#include <stdio.h>
#include "ai.h"

void i_tanh(Vector v, VSize a){
    for (register VSize i=0; i<a; i++){
        v[i] = (Scalar) _tanh((float)(v[i]));
    }
}

void i_relu(Vector v, VSize a){
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

void r_matmul(register const Matrix m, register const Vector v, register Vector r, register const VSize a, register const VSize b){
    for (register VSize i=0; i<a; i++){
        r[i] = dot(&m[mat_index2(a,b,i,0)], v, b);
    }
}

// void print_vector(register const Vector v, register const VSize a){
//     printf("[");
//     for (register VSize i=0; i<a; i++){
//         printf("%lu", v[i]);
//         if (i != a-1){
//             printf(", ");
//         }
//     }
//     printf("]\n");
// }

// void print_matrix(register const Matrix m, register const VSize a, register const VSize b){
//     printf("Matrix[\n");
//     for (register VSize i=0; i<a; i++){
//         printf("  ");
//         print_vector(&m[mat_index2(a,b,i,0)], b);
//     }
//     printf("]\n");
// }