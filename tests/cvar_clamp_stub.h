/* Model cvar clamp writes so rendering tests detect configuration damage. */
#pragma once
#include <assert.h>
#include <stdio.h>

static unsigned test_cvar_writes;

static void Test_SetCvarValue(cvar_t *var, float value)
{
    static struct { cvar_t *var; char text[32]; } values[128];
    size_t slot = 0;
    while (slot < q_countof(values) && values[slot].var && values[slot].var != var)
        slot++;
    assert(slot < q_countof(values));
    values[slot].var = var;
    snprintf(values[slot].text, sizeof(values[slot].text), "%g", value);
    var->string = values[slot].text;
    var->value = value;
    var->integer = (int)value;
    var->modified = true;
    test_cvar_writes++;
}

static float Test_ClampCvarValue(cvar_t *var, float low, float high)
{
    const float value = var->value < low ? low : var->value > high ? high : var->value;
    if (value != var->value)
        Test_SetCvarValue(var, value);
    return value;
}

static inline int Test_ClampCvarInteger(cvar_t *var, int low, int high)
{
    const int value = Q_clip(var->integer, low, high);
    if (value != var->integer)
        Test_SetCvarValue(var, value);
    return value;
}
