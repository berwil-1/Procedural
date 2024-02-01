#pragma once
#include <corecrt_math.h>

inline float clamp(float f, float a, float b) { return fmaxf(a, fminf(f, b)); }