// math/math.h

#ifndef _MATH_H_
#define _MATH_H_

#include <stdint.h>
#include <kernel.h>

#define M_PI 3.1415926535

double sqrt(double x) {
    if (x < 0) return -1; // 错误输入
    if (x == 0) return 0;

    double guess = x / 2.0;
    double epsilon = 1e-10; // 精度

    while (1) {
        double next_guess = (guess + x / guess) / 2.0;
        if (next_guess - guess < epsilon && next_guess - guess > -epsilon) {
            break; // 已经足够接近了
        }
        guess = next_guess;
    }

    return guess;
}
double abs(double x) {
    return x < 0 ? -x : x;
}
double mod(double a, double b) {
    if (b == 0) return 0; // 避免除以零
    double result = a - b * (int)(a / b);
    if (result < 0) result += b; // 确保结果为正数
    return result;
}
static void __sincos_internal(double a, double *s, double *c) {
    a = mod(a, 2.0 * M_PI);
    if (a < 0) a += 2.0 * M_PI;

    double sign_s = 1.0;
    double sign_c = 1.0;

    // 象限映射逻辑
    if (a > 0.5 * M_PI && a <= 1.5 * M_PI) {
        // 第二、三象限
        a -= M_PI;
        sign_c = -1.0;
        sign_s = -1.0; // 关键：sin 在这里也要翻转
    } else if (a > 1.5 * M_PI) {
        // 第四象限
        a -= 2.0 * M_PI;
        // cos 在第四象限是正的，sin 是负的
        // 因为 a 减去了 2PI，迭代出的 rs 本身就会带负号，所以不需要额外 sign_s
    }

    // --- 迭代逻辑保持不变 ---
    double ta = 0.7853981633974483; 
    double tk = 1.0;
    double ca = 0.0;
    double rc = 1.0, rs = 0.0;

    for (int i = 0; i < 30; i++) {
        double dir = (a > ca) ? 1.0 : -1.0;
        double inv_sec = 1.0 / sqrt(1.0 + tk * tk);
        double c_s = inv_sec;
        double s_s = tk * inv_sec * dir;

        double next_rc = rc * c_s - rs * s_s;
        double next_rs = rs * c_s + rc * s_s;
        
        rc = next_rc;
        rs = next_rs;
        ca += ta * dir;

        tk = s_s * dir / (1.0 + c_s); 
        ta /= 2.0;
    }

    // 最终符号补偿
    if (s) *s = rs * sign_s; 
    if (c) *c = rc * sign_c;
}

// 现在 API 函数可以极其简单
double sin(double a) {
    double s;
    __sincos_internal(a, &s, 0);
    return s;
}

double cos(double a) {
    double c;
    __sincos_internal(a, 0, &c);
    return c;
}

double tan(double a) {
    double s, c;
    __sincos_internal(mod(a, 2 * M_PI), &s, &c);
    if (abs(c) < 1e-9) return 0; // 避免除以 0
    return s / c;
}

// random seed
static unsigned long _next = 1;

int rand(void) {
    _next = _next * 1103515245 + 12345;
    return (unsigned int)(_next / 65536) % 32768;
}
void srand(unsigned int seed) {
    _next = seed;
}
int rand_range(int min, int max) {
    if (max <= min) return min;
    return rand() % (max - min + 1) + min;
}

#endif