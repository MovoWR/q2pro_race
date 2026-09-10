#include "strafe_helper.h"
#include "client/hud_editor.h"
#include <src/client/client.h>
#include "strafe_helper_customization.h"
#include "sh_efficiency_draw.h"
#include "sh_draw_math.h"

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#define SH_EPSILON 0.0001f

NerdStats ns;
StrafeHelper sh;
static StrafeHelper sh_raw;
static StrafeHelper sh_previous_smooth;
static bool sh_smoothing_initialized;
static bool sh_predicting;
static float sh_raw_side;
static float sh_previous_side;
static unsigned sh_visual_realtime;
static StrafeEfficiency sh_efficiency;
static unsigned sh_efficiency_realtime;

bool sh_drawing_preview = false;


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

static bool isForwardOnlyWishdir(const float forward[3], const float wishdir[3]) {
    const float wishdir_norm = vectorNorm(wishdir);
    const float forward_norm = vectorNorm(forward);

    if (wishdir_norm <= SH_EPSILON || forward_norm <= SH_EPSILON) {
        return false;
    }

    return fabsf(crossProduct(wishdir, forward)) <= SH_EPSILON * wishdir_norm * forward_norm &&
           dotProduct(wishdir, forward) > 0.0f;
}

static float safeAcosf(const float value) {
    return acosf(CLAMP(value, -1.0f, 1.0f));
}

static void clearStrafeAngles(void) {
    sh_raw = (StrafeHelper) { 0 };
    sh = (StrafeHelper) { 0 };

    sh_smoothing_initialized = false;
    sh_raw_side = 0.0f;
    sh_previous_side = 0.0f;
}

void StrafeHelper_SetEfficiency(const float velocity[3],
                                const float wishdir[3],
                                const float target, const float budget) {
    if (!sh_predicting) {
        return;
    }

    sh_efficiency = StrafeEfficiency_Calculate(velocity, wishdir,
                                               target, budget);
    sh_efficiency_realtime = cls.realtime;
}

void StrafeHelper_UpdateEfficiency(void)
{
    const bool valid = sh_efficiency.valid && sh_efficiency_realtime == cls.realtime;
    SH_Efficiency_Update(valid, sh_efficiency.efficiency);
}

void StrafeHelper_ClearEfficiency(void) {
    sh_efficiency = (StrafeEfficiency) { 0 };
    sh_efficiency_realtime = 0;
}

void StrafeHelper_Clear(void) {
    clearStrafeAngles();
    StrafeHelper_ClearEfficiency();
}

void StrafeHelper_BeginPrediction(void) {
    sh_predicting = true;
    sh_raw = (StrafeHelper) { 0 };
    sh_raw_side = 0.0f;
    StrafeHelper_ClearEfficiency();
}

bool StrafeHelper_IsPredicting(void) {
    return sh_predicting;
}

static float SH_SmoothingAmount(void) {
    if (!cl_strafehelperSmoothing) {
        return 0.0f;
    }

    return Cvar_ClampValue(cl_strafehelperSmoothing, 0.0f, 10.0f);
}

static float SH_SmoothingFactor(const float frametime, const float smoothing) {
    const float tau = 0.05f * smoothing;

    if (tau <= 0.0f) {
        return 1.0f;
    }

    return 1.0f - expf(-max(frametime, 0.0f) / tau);
}

static float SH_EaseOut(float t) {
    const int mode = cl_strafehelperSmoothingMode
                     ? cl_strafehelperSmoothingMode->integer
                     : 1;

    t = CLAMP(t, 0.0f, 1.0f);
    switch (mode) {
        case 2:
            return 1.0f - (1.0f - t) * (1.0f - t);
        case 3:
            return 1.0f - powf(1.0f - t, 3.0f);
        case 4:
            return sinf(t * (float) M_PI * 0.5f);
        case 5:
            return (t >= 1.0f) ? 1.0f : 1.0f - powf(2.0f, -10.0f * t);
        default:
            return t;
    }
}

static float SH_UnwrapAngle(float angle, const float reference) {
    const float two_pi = 2.0f * (float) M_PI;

    while (angle - reference > (float) M_PI) {
        angle -= two_pi;
    }
    while (angle - reference < -(float) M_PI) {
        angle += two_pi;
    }

    return angle;
}

static float SH_SmoothAngle(const float previous, const float target, const float factor) {
    return previous + factor * (target - previous);
}

static void SH_ApplyVisualSmoothing(const float frametime) {
    const float smoothing = SH_SmoothingAmount();

    if (smoothing <= 0.0f) {
        sh = sh_raw;
        sh_smoothing_initialized = false;
        return;
    }

    if (!sh_smoothing_initialized || sh_raw_side != sh_previous_side) {
        sh_previous_smooth = sh_raw;
        sh_smoothing_initialized = true;
    }

    const float factor = SH_EaseOut(SH_SmoothingFactor(frametime, smoothing));
    const float previous_origin = sh_previous_smooth.angle_current;
    const float target_origin = sh_raw.angle_current;
    const float previous_optimal = sh_previous_smooth.angle_optimal - previous_origin;
    const float target_optimal = sh_raw.angle_optimal - target_origin;
    // Keep the whole interval on one branch so its endpoints cannot cross.
    const float branch = SH_UnwrapAngle(target_optimal, previous_optimal) - target_optimal;
    sh = sh_raw;
    sh.angle_optimal = target_origin + SH_SmoothAngle(previous_optimal, target_optimal + branch, factor);
    sh.angle_minimum = target_origin + SH_SmoothAngle(sh_previous_smooth.angle_minimum - previous_origin,
                                                      sh_raw.angle_minimum - target_origin + branch, factor);
    sh.angle_maximum = target_origin + SH_SmoothAngle(sh_previous_smooth.angle_maximum - previous_origin,
                                                      sh_raw.angle_maximum - target_origin + branch, factor);
    sh.angle_diff = sh.angle_current - sh.angle_optimal;
    sh_previous_smooth = sh;
}

void StrafeHelper_EndPrediction(void) {
    if (!sh_predicting) {
        return;
    }
    sh_predicting = false;

    if (sh_raw.velocity_norm <= SH_EPSILON) {
        clearStrafeAngles();
        return;
    }

    SH_ApplyVisualSmoothing((cls.realtime - sh_visual_realtime) * 0.001f);
    sh_visual_realtime = cls.realtime;
    sh_previous_side = sh_raw_side;
}

// Strafe Calculations
void StrafeHelper_SetAccelerationValues(const float forward[3],
                                        const float velocity[3],
                                        const float wishdir[3],
                                        const float wishspeed,
                                        const float acceleration_target,
                                        const float accel,
                                        const float frametime) {
    if (!sh_predicting) {
        return;
    }

    const float v_z = velocity[2];
    const float w_z = wishdir[2];
    const float wishdir_norm = vectorNorm(wishdir);
    const float forward_velocity_angle = angleBetweenVectors(wishdir, forward);
    float angle_sign = vectorAngleSign(wishdir, velocity);
    const float budget = accel * frametime * wishspeed;
    const float target = acceleration_target - v_z * w_z;
    NerdStatsUpdate(velocity, wishdir, wishspeed, acceleration_target,
                    accel, frametime, forward_velocity_angle);

    sh_raw.velocity_norm = vectorNorm(velocity);
    if (sh_raw.velocity_norm <= SH_EPSILON || wishdir_norm <= SH_EPSILON ||
        target <= 0.0f || budget <= 0.0f ||
        isForwardOnlyWishdir(forward, wishdir)) {
        clearStrafeAngles();
        return;
    }

    if (angle_sign == 0.0f) {
        angle_sign = sh_previous_side != 0.0f ? sh_previous_side
                                             : vectorAngleSign(wishdir, forward);
        if (angle_sign == 0.0f) {
            angle_sign = 1.0f;
        }
    }
    sh_raw_side = angle_sign;

    const float horizontal_squared = min(wishdir_norm * wishdir_norm, 1.0f);
    const float projection_scale = sh_raw.velocity_norm * wishdir_norm;
    // PMove applies k = min(budget, target - horizontal projection).
    // Maximize 2*k*projection + k*k*horizontal_squared on both branches.
    const float capped_optimal = target * (1.0f - horizontal_squared) / (2.0f - horizontal_squared);
    const float optimal_projection = max(target - budget, capped_optimal);
    sh_raw.angle_optimal = angle_sign * safeAcosf(optimal_projection / projection_scale) - forward_velocity_angle;
    sh_raw.angle_minimum = angle_sign * safeAcosf(target / projection_scale) - forward_velocity_angle;

    const float loss_boundary = min(budget * 0.5f, target / (2.0f - horizontal_squared));
    sh_raw.angle_maximum = angle_sign * safeAcosf(-loss_boundary * wishdir_norm / sh_raw.velocity_norm) - forward_velocity_angle;
    sh_raw.angle_current = SH_UnwrapAngle(angleBetweenVectors(forward, velocity), sh_raw.angle_optimal);
    sh_raw.angle_diff = sh_raw.angle_current - sh_raw.angle_optimal;
}

void NerdStatsUpdate(const float velocity[3],
                     const float wishdir[3],
                     const float wishspeed,
                     const float acceleration_target,
                     const float accel,
                     float frametime,
                     float forward_velocity_angle) {
    if (cl_strafehelperNerdStats->integer) {
        float currentspeed = DotProduct(velocity, wishdir);
        float addspeed = acceleration_target - currentspeed;
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


        ns.locmove_x = cl.localmove[0];
        ns.locmove_y = cl.localmove[1];
        ns.locmove_z = cl.localmove[2];

        ns.pitch = cl.refdef.viewangles[0];
        ns.roll = cl.refdef.viewangles[2];
        ns.viewangles = cl.refdef.viewangles[1]; // YAW

        ns.pmove = cl.frame.ps.pmove.pm_type;
        ns.forward_velocity_angle_nerd = forward_velocity_angle;
    }
}

void OriginUpdate(void) {
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

#define SH_UPS_THRESHOLD_LOW 400.0f
#define SH_UPS_THRESHOLD_HIGH 600.0f

typedef enum {
    SH_BarStyle_Solid,
    SH_BarStyle_Gradient,
    SH_BarStyle_Outline,
    SH_BarStyle_Minimal
} SH_BarStyle;

static bool stringEquals(const char *value, const char *expected) {
    return value && !Q_stricmp(value, expected);
}

static struct {
    float speed, acceleration;
    unsigned time;
    int client_num;
    bool valid, predicted, three_dimensional;
} sh_ups_history;

static bool SH_Ups_UsesPrediction(void) {
    return !cls.demo.playback && cl.frame.clientNum == cl.clientNum &&
           cl_predict->integer &&
           !(cl.frame.ps.pmove.pm_flags & PMF_NO_PREDICTION);
}

static bool SH_Ups_GetSpeed(float *out_speed) {
    vec3_t vel;

    if (cl.frame.clientNum == CLIENTNUM_NONE) {
        return false;
    }

    if (SH_Ups_UsesPrediction()) {
        VectorCopy(cl.predicted_velocity, vel);
    } else {
        VectorScale(cl.frame.ps.pmove.velocity, 0.125f, vel);
    }

    if (!cl_strafehelperUps3D || !cl_strafehelperUps3D->integer) {
        vel[2] = 0.0f;
    }
    *out_speed = VectorLength(vel);
    return true;
}

static uint32_t SH_Ups_ParseColor(cvar_t *color_cvar, const uint32_t fallback) {
    if (!color_cvar) {
        return fallback;
    }
    return shc_ParseColorString(color_cvar->string, NULL, NULL, NULL, NULL);
}

static uint32_t SH_Ups_RainbowColor(void) {
    const float time = (float) cls.realtime * 0.006f;
    const int r = Q_rint(127.5f + sinf(time) * 127.5f);
    const int g = Q_rint(127.5f + sinf(time + 2.094395f) * 127.5f);
    const int b = Q_rint(127.5f + sinf(time + 4.188790f) * 127.5f);
    return MakeColor(r, g, b, 255);
}

static uint32_t SH_Ups_LerpColor(uint8_t r1, uint8_t g1, uint8_t b1, uint8_t a1,
                                 uint8_t r2, uint8_t g2, uint8_t b2, uint8_t a2,
                                 float t)
{
    uint8_t r = Q_rint(r1 + t * (r2 - r1));
    uint8_t g = Q_rint(g1 + t * (g2 - g1));
    uint8_t b = Q_rint(b1 + t * (b2 - b1));
    uint8_t a = Q_rint(a1 + t * (a2 - a1));
    return MakeColor(r, g, b, a);
}

static uint32_t SH_Ups_ColorForSpeed(const float speed) {
    const bool predicted = SH_Ups_UsesPrediction();
    const bool three_dimensional = cl_strafehelperUps3D && cl_strafehelperUps3D->integer;
    if (!sh_ups_history.valid || sh_ups_history.predicted != predicted ||
        sh_ups_history.client_num != cl.frame.clientNum ||
        sh_ups_history.three_dimensional != three_dimensional) {
        sh_ups_history.speed = speed;
        sh_ups_history.acceleration = 0.0f;
        sh_ups_history.time = cls.realtime;
        sh_ups_history.client_num = cl.frame.clientNum;
        sh_ups_history.predicted = predicted;
        sh_ups_history.three_dimensional = three_dimensional;
        sh_ups_history.valid = true;
    }
    const float previous_speed = sh_ups_history.speed;
    const unsigned elapsed_ms = cls.realtime - sh_ups_history.time;
    const char *mode = cl_strafehelperUpsColorMode ? cl_strafehelperUpsColorMode->string : "dynamic";
    const bool gradient = stringEquals(mode, "gradient");
    uint32_t color;

    if (stringEquals(mode, "rainbow")) {
        color = SH_Ups_RainbowColor();
    } else if (stringEquals(mode, "static")) {
        color = SH_Ups_ParseColor(cl_strafehelperUpsColorNeutral, U32_WHITE);
    } else if (stringEquals(mode, "threshold")) {
        if (speed >= SH_UPS_THRESHOLD_HIGH) {
            color = SH_Ups_ParseColor(cl_strafehelperUpsColorGain, U32_GREEN);
        } else if (speed >= SH_UPS_THRESHOLD_LOW) {
            color = SH_Ups_ParseColor(cl_strafehelperUpsColorNeutral, U32_WHITE);
        } else {
            color = SH_Ups_ParseColor(cl_strafehelperUpsColorLoss, U32_RED);
        }
    } else if (gradient) {
        // Speed is sampled during drawing, independently of main-loop ticks.
        if (elapsed_ms) {
            const float acceleration = (speed - previous_speed) / (elapsed_ms * 0.001f);
            sh_ups_history.acceleration += 0.15f * (acceleration - sh_ups_history.acceleration);
        }

        float factor = sh_ups_history.acceleration / 1000.0f;
        if (factor < -1.0f) factor = -1.0f;
        else if (factor > 1.0f) factor = 1.0f;

        uint8_t nr, ng, nb, na;
        uint8_t gr, gg, gb, ga;
        uint8_t lr, lg, lb, la;
        shc_ParseColorString(cl_strafehelperUpsColorNeutral->string, &nr, &ng, &nb, &na);
        shc_ParseColorString(cl_strafehelperUpsColorGain->string, &gr, &gg, &gb, &ga);
        shc_ParseColorString(cl_strafehelperUpsColorLoss->string, &lr, &lg, &lb, &la);

        if (factor > 0.0f) {
            color = SH_Ups_LerpColor(nr, ng, nb, na, gr, gg, gb, ga, factor);
        } else {
            color = SH_Ups_LerpColor(nr, ng, nb, na, lr, lg, lb, la, -factor);
        }
    } else if (stringEquals(mode, "strafing")) {
        if (sh.velocity_norm < 10.0f) {
            color = SH_Ups_ParseColor(cl_strafehelperUpsColorNeutral, U32_WHITE);
        } else {
            float diff = fabsf(sh.angle_diff);
            uint8_t nr, ng, nb, na;
            uint8_t gr, gg, gb, ga;
            uint8_t lr, lg, lb, la;
            shc_ParseColorString(cl_strafehelperUpsColorNeutral->string, &nr, &ng, &nb, &na);
            shc_ParseColorString(cl_strafehelperUpsColorGain->string, &gr, &gg, &gb, &ga);
            shc_ParseColorString(cl_strafehelperUpsColorLoss->string, &lr, &lg, &lb, &la);

            if (diff <= 0.01f) {
                color = MakeColor(gr, gg, gb, ga);
            } else if (diff <= 0.08f) {
                float t = (diff - 0.01f) / 0.07f;
                color = SH_Ups_LerpColor(gr, gg, gb, ga, nr, ng, nb, na, t);
            } else if (diff <= 0.20f) {
                float t = (diff - 0.08f) / 0.12f;
                color = SH_Ups_LerpColor(nr, ng, nb, na, lr, lg, lb, la, t);
            } else {
                color = MakeColor(lr, lg, lb, la);
            }
        }
    } else {
        if (speed > previous_speed) {
            color = SH_Ups_ParseColor(cl_strafehelperUpsColorGain, U32_GREEN);
        } else if (speed < previous_speed) {
            color = SH_Ups_ParseColor(cl_strafehelperUpsColorLoss, U32_RED);
        } else {
            color = SH_Ups_ParseColor(cl_strafehelperUpsColorNeutral, U32_WHITE);
        }
    }

    if (elapsed_ms || !gradient) {
        sh_ups_history.speed = speed;
        sh_ups_history.time = cls.realtime;
    }
    return color;
}

static bool SH_Ups_FormatText(const float speed, char *buffer, const size_t size) {
    const int rounded_speed = Q_rint(speed);
    const char *format = cl_strafehelperUpsFormat ? cl_strafehelperUpsFormat->string : "plain";

    if (cl_strafehelperUpsHideZero && cl_strafehelperUpsHideZero->integer && rounded_speed == 0) {
        if (size) {
            *buffer = 0;
        }
        return false;
    }

    if (stringEquals(format, "suffix") || stringEquals(format, "ups") || stringEquals(format, "1")) {
        Q_scnprintf(buffer, size, "%d ups", rounded_speed);
    } else if (stringEquals(format, "prefix") || stringEquals(format, "label") || stringEquals(format, "2")) {
        Q_scnprintf(buffer, size, "UPS: %d", rounded_speed);
    } else {
        Q_scnprintf(buffer, size, "%d", rounded_speed);
    }

    return buffer[0] != 0;
}

void SH_Ups_Draw(const float hud_width, const float hud_height,
                 const float hud_scale, const int font_pic) {
    if (!HUD_EditorPreview() && (!cl_strafehelperUps || !cl_strafehelperUps->integer)) {
        sh_ups_history.valid = false;
        return;
    }

    char buffer[MAX_STRING_CHARS];
    float speed;
    const float text_scale = cl_strafehelperUpsScale
                             ? HUD_EditorClamp(cl_strafehelperUpsScale,
                                               SH_UPS_SCALE_MIN, SH_UPS_SCALE_MAX)
                             : 1.0f;
    const float draw_scale = (hud_scale > 0.0f ? hud_scale : 1.0f) / text_scale;
    const float draw_hud_width = hud_width / text_scale;
    const float draw_hud_height = hud_height / text_scale;
    const float y_offset = cl_strafehelperUpsY
                           ? SH_ClampDrawValue(HUD_EditorClamp(cl_strafehelperUpsY,
                                                               SH_UPS_Y_MIN, SH_UPS_Y_MAX),
                                               -hud_height, hud_height) / text_scale
                           : 0.0f;
    const float x_offset = cl_strafehelperUpsX
                           ? SH_ClampDrawValue(HUD_EditorClamp(cl_strafehelperUpsX,
                                                               SH_UPS_X_MIN, SH_UPS_X_MAX),
                                               -hud_width, hud_width) / text_scale
                           : 0.0f;
    int flags = UI_CENTER;

    speed = 742;
    if (!HUD_EditorPreview() && !SH_Ups_GetSpeed(&speed)) {
        sh_ups_history.valid = false;
        return;
    }

    if (!SH_Ups_FormatText(speed, buffer, sizeof(buffer))) {
        if (!HUD_EditorPreview()) {
            sh_ups_history.valid = false;
        }
        return;
    }

    if (cl_strafehelperUpsShadow && cl_strafehelperUpsShadow->integer) {
        flags |= UI_DROPSHADOW;
    } else {
        flags |= UI_NOSHADOW;
    }

    const float x = Q_rint(draw_hud_width / 2.0f + x_offset);
    const float y = Q_rint((draw_hud_height - CHAR_HEIGHT) / 2.0f + y_offset);
    HUD_EditorBounds(HUD_EDIT_UPS, (x - strlen(buffer) * CHAR_WIDTH * 0.5f) * text_scale,
                     y * text_scale, strlen(buffer) * CHAR_WIDTH * text_scale + 1,
                     CHAR_HEIGHT * text_scale + 1);
    if (!HUD_EditorShow(HUD_EDIT_UPS)) {
        return;
    }
    R_SetColor(HUD_EditorPreview() ? U32_WHITE : SH_Ups_ColorForSpeed(speed));
    R_SetScale(draw_scale);
    SCR_DrawStringEx(x, y,
                     flags, MAX_STRING_CHARS, buffer, font_pic);
    R_SetScale(hud_scale);
    R_ClearColor();
}

static SH_BarStyle SH_GetBarStyle(void) {
    const char *style = cl_strafehelperBarStyle ? cl_strafehelperBarStyle->string : "gradient";

    if (stringEquals(style, "solid") || stringEquals(style, "0")) {
        return SH_BarStyle_Solid;
    }
    if (stringEquals(style, "outline") || stringEquals(style, "2")) {
        return SH_BarStyle_Outline;
    }
    if (stringEquals(style, "minimal") || stringEquals(style, "3")) {
        return SH_BarStyle_Minimal;
    }
    return SH_BarStyle_Gradient;
}

static void drawRectangleOutline(float x, const float y, float w, const float h,
                                 const float thickness,
                                 const enum shc_ElementId element_id) {
    if (w < 0.0f) {
        x += w;
        w = -w;
    }
    if (w <= 0.0f || h <= 0.0f || thickness <= 0.0f) {
        return;
    }

    const float t = CLAMP(thickness, 1.0f, h * 0.5f);
    shc_drawFilledRectangle(x, y, w, t, element_id);
    shc_drawFilledRectangle(x, y + h - t, w, t, element_id);
    shc_drawFilledRectangle(x, y, t, h, element_id);
    shc_drawFilledRectangle(x + w - t, y, t, h, element_id);
}

static void drawAngleMarker(const float angle, const float width, const float y, const float height,
                            const struct StrafeHelperParams *params, const float hud_width,
                            const enum shc_ElementId element_id, const bool outline) {
    const float x = angleToPixel(angle, params->scale, hud_width) - width * 0.5f;
    const float edge_a = angleToPixel(-(float) M_PI, params->scale, hud_width);
    const float edge_b = angleToPixel((float) M_PI, params->scale, hud_width);
    const float clip_start = min(edge_a, edge_b);
    const float clip_end = max(edge_a, edge_b);
    const float period = clip_end - clip_start;
    const int first_branch = params->center ? 0 : -1;
    const int last_branch = params->center ? 0 : 1;
    for (int branch = first_branch; branch <= last_branch; branch++) {
        float start = x + branch * period;
        float end = start + width;
        if (!params->center) {
            start = max(start, clip_start);
            end = min(end, clip_end);
        }
        if (end <= start) {
            continue;
        }
        if (outline) {
            drawRectangleOutline(start, y, end - start, height, 1.0f, element_id);
        } else {
            shc_drawFilledRectangle(start, y, end - start, height, element_id);
        }
    }
}

// Visible bounds are clipped at the view seam; full bounds retain the gradient domain.
static void drawAccelerationZone(const float accel_start, const float accel_end,
                                 const float full_start, const float full_end,
                                 const float upper_y, const float height,
                                 const float optimal_x,
                                 const float optimal_width) {
    const float marker_gap_half_width = (cl_strafehelper_optimal_outline && cl_strafehelper_optimal_outline->integer)
                                        ? (optimal_width * 0.5f + 1.0f)
                                        : (optimal_width * 0.5f);
    const float gap_start = CLAMP(optimal_x - marker_gap_half_width, accel_start, accel_end);
    const float gap_end = CLAMP(optimal_x + marker_gap_half_width, accel_start, accel_end);
    const float full_gap_start = CLAMP(optimal_x - marker_gap_half_width, full_start, full_end);
    const float full_gap_end = CLAMP(optimal_x + marker_gap_half_width, full_start, full_end);
    const SH_BarStyle style = SH_GetBarStyle();

    if (style == SH_BarStyle_Gradient) {
        if (gap_start > accel_start) {
            shc_drawGradientRectangle(
                accel_start,
                upper_y,
                gap_start - accel_start,
                height,
                optimal_x, full_start, full_gap_start,
                shc_ElementId_AcceleratingAngles,
                shc_ElementId_OptimalAngle);
        }
        if (gap_end < accel_end) {
            shc_drawGradientRectangle(
                gap_end,
                upper_y,
                accel_end - gap_end,
                height,
                optimal_x, full_gap_end, full_end,
                shc_ElementId_AcceleratingAngles,
                shc_ElementId_OptimalAngle);
        }
        return;
    }

    if (style == SH_BarStyle_Solid) {
        if (gap_start > accel_start) {
            shc_drawFilledRectangle(
                accel_start,
                upper_y,
                gap_start - accel_start,
                height,
                shc_ElementId_AcceleratingAngles);
        }
        if (gap_end < accel_end) {
            shc_drawFilledRectangle(
                gap_end,
                upper_y,
                accel_end - gap_end,
                height,
                shc_ElementId_AcceleratingAngles);
        }
        return;
    }

    if (style == SH_BarStyle_Outline) {
        if (gap_start > accel_start) {
            drawRectangleOutline(
                accel_start,
                upper_y,
                gap_start - accel_start,
                height,
                1.0f,
                shc_ElementId_AcceleratingAngles);
        }
        if (gap_end < accel_end) {
            drawRectangleOutline(
                gap_end,
                upper_y,
                accel_end - gap_end,
                height,
                1.0f,
                shc_ElementId_AcceleratingAngles);
        }
        return;
    }
}

bool StrafeHelper_HasData(void) {
    return sh.velocity_norm > SH_EPSILON;
}

void StrafeHelper_DrawPreview(const struct StrafeHelperParams *params,
                              const float hud_width, const float hud_height,
                              const int font_pic) {
    if (params->height <= 0.0f) {
        return;
    }

    sh_drawing_preview = true;

    const float upper_y = (hud_height - params->height) / 2.0f + params->y;
    const float center_width = CLAMP(cl_strafehelper_center_width->value, 0.1f, 5.0f);
    const float optimal_width = CLAMP(cl_strafehelper_optimal_width->value, 0.1f, 5.0f);
    const float center_x = hud_width * 0.5f;
    const float scale = CLAMP(params->scale, 0.25f, 8.0f);
    const float accel_width = CLAMP(hud_width * 0.28f * scale, 96.0f, hud_width * 0.82f);
    const float accel_start = center_x - accel_width * 0.5f;
    const float accel_end = center_x + accel_width * 0.5f;
    const float optimal_offset = CLAMP(36.0f * scale, 12.0f, accel_width * 0.35f);
    const float optimal_x = center_x + optimal_offset;
    HUD_EditorBounds(HUD_EDIT_STRAFE, accel_start, upper_y, accel_width, params->height);

    SH_Efficiency_DrawPreview(upper_y, params->height, hud_width,
                              params->hud_scale, font_pic);

    if (!HUD_EditorShow(HUD_EDIT_STRAFE)) {
        sh_drawing_preview = false;
        return;
    }

    drawAccelerationZone(accel_start, accel_end, accel_start, accel_end,
                         upper_y, params->height, optimal_x, optimal_width);

    if (cl_strafehelper_optimal_outline && cl_strafehelper_optimal_outline->integer) {
        drawRectangleOutline(
            optimal_x - optimal_width / 2.0f,
            upper_y,
            optimal_width,
            params->height,
            1.0f,
            shc_ElementId_OptimalAngle);
    } else {
        shc_drawFilledRectangle(
            optimal_x - optimal_width / 2.0f,
            upper_y,
            optimal_width,
            params->height,
            shc_ElementId_OptimalAngle);
    }

    if (params->center_marker) {
        shc_drawFilledRectangle(
            center_x - center_width / 2.0f,
            upper_y + params->height / 2.0f,
            center_width,
            params->height / 2.0f,
            shc_ElementId_CenterMarker);
    }

    sh_drawing_preview = false;
}

void StrafeHelper_Draw(const struct StrafeHelperParams *params,
                       const float hud_width, const float hud_height, int font_pic) {
    if (params->height <= 0.0f) {
        return;
    }

    const float upper_y = (hud_height - params->height) / 2.0f + params->y;

    if (cl.frame.ps.pmove.pm_type == PM_NORMAL) {
        SH_Efficiency_Draw(upper_y, params->height, hud_width,
                           params->hud_scale, font_pic);
    }

    if (!StrafeHelper_HasData()) {
        return;
    }

    float angle_x, angle_width;
    const float center_width = CLAMP(cl_strafehelper_center_width->value, 0.1f, 5.0f);
    const float optimal_width = CLAMP(cl_strafehelper_optimal_width->value, 0.1f, 5.0f);

    // Shift the interval together; only noncentered drawing wraps at the view seam.
    const float offset = params->center ? -sh.angle_current
                         : SH_UnwrapAngle(sh.angle_optimal, 0.0f) - sh.angle_optimal;
    if (sh.angle_minimum < sh.angle_maximum) {
        angle_x = sh.angle_minimum + offset;
        angle_width = sh.angle_maximum - sh.angle_minimum;
    } else {
        angle_x = sh.angle_maximum + offset;
        angle_width = sh.angle_minimum - sh.angle_maximum;
    }

    const float two_pi = 2.0f * (float) M_PI;
    const int first_branch = params->center ? 0 : -1;
    const int last_branch = params->center ? 0 : 1;
    for (int branch = first_branch; branch <= last_branch; branch++) {
        float start_angle = angle_x + branch * two_pi;
        float end_angle = start_angle + angle_width;
        if (!params->center) {
            // Split a wrapped interval at +/-pi without stretching its endpoints.
            start_angle = max(start_angle, -(float) M_PI);
            end_angle = min(end_angle, (float) M_PI);
        }
        if (end_angle <= start_angle) {
            continue;
        }
        float accel_start = angleToPixel(start_angle, params->scale, hud_width);
        float accel_end = angleToPixel(end_angle, params->scale, hud_width);
        if (accel_start > accel_end) {
            const float tmp = accel_start;
            accel_start = accel_end;
            accel_end = tmp;
        }
        const float branch_optimal_x = angleToPixel(sh.angle_optimal + offset + branch * two_pi,
                                                    params->scale, hud_width);
        const float full_start = angleToPixel(angle_x + branch * two_pi, params->scale, hud_width);
        const float full_end = angleToPixel(angle_x + angle_width + branch * two_pi, params->scale, hud_width);
        drawAccelerationZone(accel_start, accel_end, min(full_start, full_end), max(full_start, full_end),
                             upper_y, params->height, branch_optimal_x, optimal_width);
    }

    const bool outline = cl_strafehelper_optimal_outline && cl_strafehelper_optimal_outline->integer;
    drawAngleMarker(sh.angle_optimal + offset, optimal_width, upper_y, params->height,
                    params, hud_width, shc_ElementId_OptimalAngle, outline);
    if (params->center_marker) {
        const float current_angle = params->center ? 0.0f : SH_UnwrapAngle(sh.angle_current, 0.0f);
        drawAngleMarker(current_angle, center_width, upper_y + params->height / 2.0f, params->height / 2.0f,
                        params, hud_width, shc_ElementId_CenterMarker, false);
    }
}