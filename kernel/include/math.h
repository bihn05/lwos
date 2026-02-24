#ifndef _MATH_H_
#define _MATH_H_

#include <stdint.h>

#define M_PI 3.1415926535

double sqrt(double x);
double abs(double x);
double mod(double a, double b);
double sin(double a);
double cos(double a);
double tan(double a);
int rand(void);
void srand(unsigned int seed);
int rand_range(int min, int max);

#endif