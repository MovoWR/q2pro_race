#include "strafe_helper.h"
#include <src/client/client.h>
#include "strafe_helper_customization.h"

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

NerdStats ns;
StrafeHelper sh;

bool isOptimal;
bool insideAccelerationZone;
bool speedIncreased;


static float sign(const float value) {
    if (value < 0.0f) {
        return -1.0f;
    } else if (value > 0.0f) {
        return 1.0f;
    }
    return 0.0f;
}

static float crossProduct(const float v[2], const float w[2]) {
    return v[0] * w[1] - v[1] * w[0];
}

static float dotProduct(const float v[2], const float w[2]) {
    float dot_product = 0.0f;
    for (int i = 0; i < 2; i++) {
        dot_product += v[i] * w[i];
    }
    return dot_product;
}

static float angleBetweenVectors(const float v[2], const float w[2]) {
    return atan2f(crossProduct(v, w), dotProduct(v, w));
}

static float vectorAngleSign(const float v[2], const float w[2]) {
    return sign(crossProduct(v, w));
}

static float vectorNorm(const float v[2]) {
    return sqrtf(dotProduct(v, v));
}


// Strafe Calculations
void StrafeHelper_SetAccelerationValues(const float forward[3],
                                        const float velocity[3],
                                        const float wishdir[3],
                                        const float wishspeed,
                                        const float accel,
                                        const float frametime) {
    const float v_z = velocity[2];
    const float w_z = wishdir[2];
    const float wishdir_norm = vectorNorm(wishdir);
    const float forward_velocity_angle = angleBetweenVectors(wishdir, forward);
    const float angle_sign = vectorAngleSign(wishdir, velocity);
    const float two_pi = 2.0f * (float) M_PI;
    sh.velocity_norm = vectorNorm(velocity);
    sh.angle_optimal = (wishspeed * (1.0f - accel * frametime) - v_z * w_z) / (sh.velocity_norm * wishdir_norm);
    sh.angle_optimal = acosf(sh.angle_optimal);
    sh.angle_optimal = angle_sign * sh.angle_optimal - forward_velocity_angle;

    sh.angle_minimum = (wishspeed - v_z * w_z) /
                       (2.0f - wishdir_norm * wishdir_norm) * wishdir_norm / sh.velocity_norm;
    sh.angle_minimum = acosf(sh.angle_minimum < 1.0f ? sh.angle_minimum : 1.0f);
    sh.angle_minimum = angle_sign * sh.angle_minimum - forward_velocity_angle;

    sh.angle_maximum = -0.5f * accel * frametime * wishspeed * wishdir_norm / sh.velocity_norm;
    sh.angle_maximum = acosf(sh.angle_maximum);
    sh.angle_maximum = angle_sign * sh.angle_maximum - forward_velocity_angle;

    sh.angle_current = angleBetweenVectors(forward, velocity);
    sh.angle_current += truncf((sh.angle_minimum - sh.angle_current) / two_pi) * two_pi;
    sh.angle_current += truncf((sh.angle_maximum - sh.angle_current) / two_pi) * two_pi;

    sh.angle_diff = sh.angle_current - sh.angle_optimal;


    NerdStatsUpdate(velocity, wishdir, wishspeed, accel, frametime, forward_velocity_angle);
}

void NerdStatsUpdate(const float velocity[3],
                     const float wishdir[3],
                     const float wishspeed,
                     const float accel,
                     float frametime,
                     float forward_velocity_angle)
{
    float currentspeed = DotProduct(velocity, wishdir);
    float addspeed = wishspeed - currentspeed;
    float accelspeed = accel * frametime * wishspeed;

    ns.currentspeed_nerd = currentspeed;
    ns.addspeed_nerd = addspeed;
    ns.accelspeed_nerd = accelspeed;
    ns.wishspeed_nerd = wishspeed;

    ns.pred_velocity_x = cl.predicted_velocity[0];
    ns.pred_velocity_y = cl.predicted_velocity[1];
    ns.pred_velocity_z = cl.predicted_velocity[2];

    ns.pred_pos_x = cl.predicted_origin[0];
    ns.pred_pos_y = cl.predicted_origin[1];
    ns.pred_pos_z = cl.predicted_origin[2];

    if (cl.frame.ps.pmove.pm_type == PM_FREEZE)
    {
        OriginUpdate();
}
    ns.locmove_x = cl.localmove[0];
    ns.locmove_y = cl.localmove[1];
    ns.locmove_z = cl.localmove[2];

    ns.pitch = cl.refdef.viewangles[0];
    ns.roll = cl.refdef.viewangles[2];
    ns.viewangles = cl.refdef.viewangles[1]; // YAW

    ns.pmove = cl.frame.ps.pmove.pm_type;
    ns.forward_velocity_angle_nerd = forward_velocity_angle;
}

void OriginUpdate(void)
{
    ns.pred_pos_x = cl.playerEntityOrigin[0];
    ns.pred_pos_y = cl.playerEntityOrigin[1];
    ns.pred_pos_z = cl.playerEntityOrigin[2];
    ns.viewangles = cl.playerEntityAngles[1]; // YAW
    ns.pitch = cl.playerEntityAngles[0];
    ns.roll = cl.playerEntityAngles[2];


}





static float angleDiffToPixelDiff(const float angle_difference, const float scale,
                                  const float hud_width) {
    return angle_difference * (hud_width / 2.0f) * scale / (float) M_PI;
}

static float angleToPixel(const float angle, const float scale,
                          const float hud_width) {
    return (hud_width / 2.0f) - 0.5f +
           angleDiffToPixelDiff(angle, scale, hud_width);
}

void StrafeHelper_Draw(const struct StrafeHelperParams *params,
                       const float hud_width, const float hud_height, int indicator_pic, int font_pic) {
    float angle_x, angle_width;
    const float upper_y = (hud_height - params->height) / 2.0f + params->y;
    const float center_width = CLAMP(cl_strafehelper_center_width->value, 0.1f, 5.0f);
    const float optimal_width = CLAMP(cl_strafehelper_optimal_width->value, 0.1f, 5.0f);

    float custom_x, custom_y, custom_width, custom_height;
    sscanf(cl_strafehelper_indicator_pos->string, "%f %f", &custom_x, &custom_y);
    sscanf(cl_strafehelper_indicator_size->string, "%f %f", &custom_width, &custom_height);

    // Adjust angle offsets
    float offset = params->center ? -sh.angle_current : 0.0f;
    if (sh.angle_minimum < sh.angle_maximum) {
        angle_x = sh.angle_minimum + offset;
        angle_width = sh.angle_maximum - sh.angle_minimum;
    } else {
        angle_x = sh.angle_maximum + offset;
        angle_width = sh.angle_minimum - sh.angle_maximum;
    }

    // Draw accel range
    shc_drawFilledRectangle(
        angleToPixel(angle_x, params->scale, hud_width),
        upper_y,
        angleDiffToPixelDiff(angle_width, params->scale, hud_width),
        params->height,
        shc_ElementId_AcceleratingAngles);

    // Draw optimal angle
    shc_drawFilledRectangle(
        angleToPixel(sh.angle_optimal + offset, params->scale, hud_width) - (optimal_width / 2.0f),
        upper_y,
        optimal_width,
        params->height,
        shc_ElementId_OptimalAngle
    );

    if (params->center_marker) {
        shc_drawFilledRectangle(
            angleToPixel(sh.angle_current + offset, params->scale, hud_width) - (center_width / 2.0f),
            upper_y + params->height / 2.0f,
            center_width,
            params->height / 2.0f,
            shc_ElementId_CenterMarker
        );
    }
}

void sh_indicator_draw(const struct StrafeHelperParams *params,
                       float hud_width, float hud_height, int indicator_pic, int font_pic) {

    bool drawIndicator = (cl_strafehelperIndicator->integer == 1 && isOptimal) ||
                         (cl_strafehelperIndicator->integer == 2 && insideAccelerationZone && speedIncreased);


    if (drawIndicator) {
        int ind_x, ind_y;
        int ind_width = hud_width / 10.0f;
        int ind_height = hud_height / 10.0f;

        if (sscanf(cl_strafehelper_indicator_size->string, "%d %d", &ind_width, &ind_height) != 2) {
            ind_width = hud_width / 10.0f;
            ind_height = hud_height / 10.0f;
        }

        int ind_x_center = (hud_width - ind_width) / 2.0f;
        int ind_y_center = (hud_height - ind_height) / 2.0f;

        if (sscanf(cl_strafehelper_indicator_pos->string, "%d %d", &ind_x, &ind_y) != 2) {
            ind_x = ind_x_center;
            ind_y = ind_y_center;
        }


        if (ind_x == 0.0f && ind_y == 0.0f) {
            ind_x = ind_x_center;
            ind_y = ind_y_center;
        } else {
            ind_x = CLAMP(ind_x, 0.0f, hud_width - ind_width);
            ind_y = CLAMP(ind_y, 0.0f, hud_height - ind_height);
        }

        if (cl_strafehelperIndicator->integer == 2) {
            R_DrawStretchPic(
                ind_x, ind_y, ind_width, ind_height, indicator_pic);
        } else {
            shc_drawFilledRectangle(
                ind_x, ind_y, ind_width, ind_height, shc_ElementId_Indicator);
        }
    }
}
