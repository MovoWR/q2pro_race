/* Strict integer input shared by FPS commands and reminder inspection. */
#pragma once

#include <limits.h>

#define NUM_FPS_SLOTS 12

/* Returns zero for anything outside the positive decimal int range. */
static inline int CL_ParseFpsInteger(const char *text)
{
    int value = 0;

    if (!*text)
        return 0;
    do {
        int digit = *text++ - '0';
        if (digit < 0 || digit > 9 || value > (INT_MAX - digit) / 10)
            return 0;
        value = value * 10 + digit;
    } while (*text);
    return value;
}

void CL_InitFpsSlots(void);
void CL_FpsDown_f(void);
void CL_FpsUp_f(void);
void CL_FpsHoldDown_f(void);
void CL_FpsHoldUp_f(void);
void CL_FpsShortcut_f(void);
