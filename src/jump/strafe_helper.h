#pragma once
#ifdef __cplusplus
extern "C" {
#endif
#include "inc/shared/shared.h"
#include <math.h>

#define DEG_TO_RAD(deg) ((deg) * ((float)M_PI / 180.0f))
#define OPTIMAL_ANGLE_TOLERANCE DEG_TO_RAD(cl_strafehelper_tolerance->value) // Angle tolerance in degrees




struct StrafeHelperParams {
    int center;
    int center_marker;
    float scale;
    float height;
    float y;
    float speed_scale;
    float speed_x;
    float speed_y;
};



void StrafeHelper_SetAccelerationValues(const float forward[3],
                                        const float velocity[3],
                                        const float wishdir[3],
                                        const float wishspeed,
                                        const float accel,
                                        const float frametime);
void StrafeHelper_Draw(const struct StrafeHelperParams *params,
                       const float hud_width, const float hud_height);


#ifndef STRAFE_HELPER_H
#define STRAFE_HELPER_H



#endif // STRAFE_HELPER_H


#ifdef __cplusplus
} /* extern "C" */
#endif


