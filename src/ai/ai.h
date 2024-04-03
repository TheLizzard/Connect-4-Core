#pragma once
#include <stdint.h>

#define _min(x, y) (((x)<(y))?(x):(y))
#define _max(x, y) (((x)>(y))?(x):(y))
#define _tanh(x) _min(0.997f, _max(-0.997f, \
                                   x*(135550+7785*x*x)/(134700+53200*x*x)))
#define _relu(x) _max(0, x)
#define mat_index2(a,b,i,j) (i*b + j)
#define mat_index3(a,b,c,i,j,k) (i*b*c + j*c + k)
#define mat_index4(a,b,c,d,i,j,k,l) (i*b*c*d + j*c*d + k*d + l)

typedef int32_t Scalar;
typedef Scalar* Vector;
#define Matrix Vector
#define Tensor Vector
#define Kernel Matrix
typedef uint16_t VSize;

void print_vector(register const Vector v, register const VSize a);
void i_tanh(register Vector v, register const VSize a);
void i_relu(register Vector v, register const VSize a);
void i_add(register Vector v1, register const Vector v2, register const VSize a);
Scalar dot(register const Vector v1, register const Vector v2, register const VSize a);
void r_matmul(register const Matrix m, register const Vector v, register Vector r, register const VSize a, register const VSize b);
void print_vector(register const Vector v, register const VSize a);
void print_matrix(register const Matrix m, register const VSize a, register const VSize b);