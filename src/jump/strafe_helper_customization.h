#ifndef STRAFE_HELPER_CUSTOMIZATION_H
#define STRAFE_HELPER_CUSTOMIZATION_H
#ifdef __cplusplus
extern "C" {
#endif
#pragma once
#include "inc/shared/shared.h"
#include "strafe_helper_customization.h"
#include <stdint.h>

enum shc_ElementId {
    shc_ElementId_AcceleratingAngles,
    shc_ElementId_OptimalAngle,
    shc_ElementId_CenterMarker,
    shc_ElementId_HighlightOptimal
};


uint32_t ParseColorCvar(cvar_t *cvar);
uint32_t getColorForElement(enum shc_ElementId element_id);

void shc_drawFilledRectangle(float x, float y, float w, float h, enum shc_ElementId element_id);
void shc_drawString(float x, float y, const char *string, float scale, enum shc_ElementId element_id);

#endif // STRAFE_HELPER_CUSTOMIZATION_H

#ifdef __cplusplus
} /* extern "C" */
#endif


