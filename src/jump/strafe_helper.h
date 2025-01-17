#pragma once
#ifdef __cplusplus
extern "C" {
#endif

#include <math.h>
#include "src/client/client.h"

#define CLAMP(value, min, max) ((value) < (min) ? (min) : ((value) > (max) ? (max) : (value)))
#define DEG_TO_RAD(deg) ((deg) * ((float)M_PI / 180.0f))
#define OPTIMAL_ANGLE_TOLERANCE DEG_TO_RAD(CLAMP(cl_strafehelper_tolerance->value, 0.0f, 10.0f))


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


struct StrafeHelper
{
    // [Angles]
    float angle_optimal;
    float angle_minimum;
    float angle_maximum;
    float angle_current;
    float angle_diff;

    // [Velocity]
    float velocity_norm;
};

extern struct StrafeHelper sh;


void StrafeHelper_SetAccelerationValues(const float forward[3],
                                        const float velocity[3],
                                        const float wishdir[3],
                                        const float wishspeed,
                                        const float accel,
                                        const float frametime);
    // StrafeHud
    void StrafeHelper_Draw(const struct StrafeHelperParams* params,
                           float hud_width, float hud_height, int indicator_pic, int font_pic);

// Indicator
void sh_indicator_draw(const struct StrafeHelperParams* params,
                       float hud_width, float hud_height, int indicator_pic, int font_pic);

#ifndef STRAFE_HELPER_H
#define STRAFE_HELPER_H



#endif // STRAFE_HELPER_H


#ifdef __cplusplus
} /* extern "C" */
#endif


