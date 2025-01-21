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

typedef struct
{
    // [Velocity]
    float speed;
    float pred_velocity;
    float pred_velocity_x;
    float pred_velocity_y;
    float pred_velocity_z;
    // [Position]
    float pred_pos_x;
    float pred_pos_y;
    float pred_pos_z;
    float originX;
    float originY;
    float originZ;
    vec3_t origin;
    // [Network]
    float ref_msec;
    float phys_msec;
    float main_msec;
    // [Local Move]
    float locmove_x;
    float locmove_y;
    float locmove_z;
    // [pitch, yaw, roll]
    float pitch;
    float roll;
    float viewangles;
    // [PMove Debug]
    float pmove;
    // [PM_Accelerate]
    float accelspeed_nerd;
    float addspeed_nerd;
    float currentspeed_nerd;
    float wishdir_nerd;
    float wishspeed_nerd;
    float forward_velocity_angle_nerd;
} NerdStats;

extern NerdStats ns;

typedef struct
{
    // [Angles]
    float angle_optimal;
    float angle_minimum;
    float angle_maximum;
    float angle_current;
    float angle_diff;
    float velocity_norm;
} StrafeHelper;

extern StrafeHelper sh;
// StrafeHud
void StrafeHelper_SetAccelerationValues(const float forward[3],
                                        const float velocity[3],
                                        const float wishdir[3],
                                        const float wishspeed,
                                        const float accel,
                                        const float frametime);
// StrafeHud
void StrafeHelper_Draw(const struct StrafeHelperParams* params,
                           float hud_width, float hud_height, int indicator_pic, int font_pic);

// NerdStats
void NerdStatsUpdate(const float velocity[3],
                     const float wishdir[3],
                     float wishspeed,
                     float accel,
                     float frametime,
                     float forward_velocity_angle);
void OriginUpdate(void);
char* SH_NerdStats_Draw(float hud_width, float hud_height, int font_pic);


// Indicator
void SH_Indicator_Draw(const struct StrafeHelperParams* params,
                       float hud_width, float hud_height, int indicator_pic, int font_pic);


// Debug
void SH_DebugNow_f(void);
void DebugNow(void);
void StrafeHelper_DebugNow(void);
void printKeyValueFloatPrecise(const char* label, float value);
void printKeyValueFloat(const char* label, float value);
void printSectionHeader(const char* title);
void printKeyValueGeneric(const char* label, const char* value);