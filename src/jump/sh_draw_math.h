/* Effective drawing bounds never change saved cvars or editor drafts. */
#pragma once
#include <math.h>

static inline float SH_ClampDrawValue(float value, float low, float high)
{
    if (high < low)
        high = low;
    if (!isfinite(value))
        return low;
    return value < low ? low : value > high ? high : value;
}
