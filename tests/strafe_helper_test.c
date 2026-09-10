/* Exercise production angle math and prediction capture with HUD boundaries stubbed. */
#include "../src/jump/strafe_helper.c"
#undef NDEBUG
#include <assert.h>
#include "cvar_clamp_stub.h"

client_state_t cl;
client_static_t cls;
static cvar_t smoothing_var, smoothing_mode_var, nerdstats_var;
static struct {
    float x, width;
    enum shc_ElementId id;
} drawn_rectangles[16];
static int drawn_count;
static bool test_gradient, saw_clipped_gradient;
static int last_ups_x, last_ups_y;
static bool efficiency_update_valid;
static float efficiency_update_value;
static unsigned efficiency_updates;

static void CheckNear(float actual, float expected, float tolerance)
{
    if (fabsf(actual - expected) > tolerance) {
        fprintf(stderr, "expected %.9f, got %.9f\n", expected, actual);
        abort();
    }
}

static void Setup(float smoothing)
{
    StrafeHelper_Clear();
    sh_predicting = false;
    cls.realtime = 1000;
    smoothing_var.value = smoothing;
    smoothing_mode_var.integer = 1;
    nerdstats_var.integer = 0;
    cl_strafehelperSmoothing = &smoothing_var;
    cl_strafehelperSmoothingMode = &smoothing_mode_var;
    cl_strafehelperNerdStats = &nerdstats_var;
}

static void SetSample(float speed, float theta, float horizontal, float vertical_velocity,
                      float target, float budget)
{
    const vec3_t forward = { 1, 0, 0 };
    const vec3_t wishdir = { 0, horizontal, sqrtf(1.0f - horizontal * horizontal) };
    const vec3_t velocity = { speed * sinf(theta), speed * cosf(theta), vertical_velocity };
    StrafeHelper_SetAccelerationValues(forward, velocity, wishdir, 300.0f,
                                       target, budget / (300.0f * .008f), .008f);
}

static void Sample(float speed, float theta, float horizontal, float vertical_velocity,
                   float target, float budget)
{
    StrafeHelper_BeginPrediction();
    SetSample(speed, theta, horizontal, vertical_velocity, target, budget);
    StrafeHelper_EndPrediction();
}

/* Directly evaluate PMove's acceleration and resulting horizontal squared speed. */
static double Gain(double speed, double theta, double horizontal, double vertical_velocity,
                   double target, double budget)
{
    const double projection = speed * horizontal * cos(theta);
    const double addspeed = target - projection - vertical_velocity * sqrt(1.0 - horizontal * horizontal);
    const double acceleration = fmin(budget, fmax(addspeed, 0.0));
    return 2.0 * acceleration * projection + acceleration * acceleration * horizontal * horizontal;
}

static void CheckMath(float speed, float horizontal, float vertical_velocity,
                      float target, float budget)
{
    Setup(0);
    Sample(speed, .5f, horizontal, vertical_velocity, target, budget);
    assert(StrafeHelper_HasData());
    const float half_pi = (float)M_PI * .5f;
    const float minimum = half_pi - sh.angle_minimum;
    const float optimal = half_pi - sh.angle_optimal;
    const float maximum = half_pi - sh.angle_maximum;
    assert(minimum <= optimal + .00001f && optimal <= maximum + .00001f);
    const double best = Gain(speed, optimal, horizontal, vertical_velocity, target, budget);
    for (int i = 0; i <= 10000; i++) {
        const double theta = i * M_PI / 10000;
        const double gain = Gain(speed, theta, horizontal, vertical_velocity, target, budget);
        assert(gain <= best + .2);
        if (theta > minimum + .0001 && theta < maximum - .0001)
            assert(gain > 0.0);
        if (theta < minimum - .0001 || theta > maximum + .0001)
            assert(gain <= .2);
    }
    if (minimum > .0001f)
        assert(fabs(Gain(speed, minimum, horizontal, vertical_velocity, target, budget)) < .2);
    if (maximum < M_PI - .0001)
        assert(fabs(Gain(speed, maximum, horizontal, vertical_velocity, target, budget)) < .2);
}

static void CheckInterval(void)
{
    const float low = min(sh.angle_minimum, sh.angle_maximum);
    const float high = max(sh.angle_minimum, sh.angle_maximum);
    assert(sh.angle_optimal >= low - .00001f && sh.angle_optimal <= high + .00001f);
    assert(high - low <= M_PI + .00001f);
}

static float SmoothRun(unsigned step, int count)
{
    Setup(2);
    Sample(400, .5f, 1, 0, 300, 2.4f);
    for (int i = 0; i < count; i++) {
        cls.realtime += step;
        Sample(800, .5f, 1, 0, 300, 2.4f);
    }
    return sh.angle_diff;
}

/* Compare the actual HUD rectangles with acceleration at each candidate heading. */
static void CheckDrawing(float yaw, float wish_yaw, float velocity_yaw, int center, float smoothing)
{
    static cvar_t width = { .value = 2 }, style = { .string = "solid" };
    const float pi = (float)M_PI;
    const vec3_t forward = { cosf(yaw), sinf(yaw), 0 };
    const vec3_t wish = { cosf(yaw + wish_yaw), sinf(yaw + wish_yaw), 0 };
    const vec3_t velocity = { 400 * cosf(yaw + velocity_yaw), 400 * sinf(yaw + velocity_yaw), 0 };
    const struct StrafeHelperParams params = {
        .center = center, .center_marker = 1, .scale = 1, .height = 12, .hud_scale = 1
    };
    Setup(smoothing);
    cl_strafehelper_center_width = &width;
    cl_strafehelper_optimal_width = &width;
    style.string = test_gradient ? "gradient" : "solid";
    cl_strafehelperBarStyle = &style;
    StrafeHelper_BeginPrediction();
    StrafeHelper_SetAccelerationValues(forward, velocity, wish, 300, 300, 1, .008f);
    StrafeHelper_EndPrediction();
    CheckInterval();
    drawn_count = 0;
    StrafeHelper_Draw(&params, 1920, 1080, 0);

    const float side = sinf(velocity_yaw - wish_yaw) < 0 ? -1 : 1;
    const float optimal = wish_yaw + side * acosf(297.6f / 400) - (center ? velocity_yaw : 0);
    const float optimal_x = 959.5f + atan2f(sinf(optimal), cosf(optimal)) * 960 / pi;
    const float current_x = 959.5f + (center ? 0 : velocity_yaw) * 960 / pi;
    int optimal_count = 0, current_count = 0;
    float acceleration_width = 0;
    for (int i = 0; i < drawn_count; i++) {
        if (drawn_rectangles[i].id == shc_ElementId_OptimalAngle) {
            CheckNear(drawn_rectangles[i].x + drawn_rectangles[i].width * .5f, optimal_x, .001f);
            optimal_count++;
        } else if (drawn_rectangles[i].id == shc_ElementId_CenterMarker) {
            CheckNear(drawn_rectangles[i].x + drawn_rectangles[i].width * .5f, current_x, .001f);
            current_count++;
        } else {
            acceleration_width += drawn_rectangles[i].width;
        }
    }
    assert(optimal_count == 1 && current_count == 1);
    CheckNear(acceleration_width, (acosf(-2.4f / 800) - acosf(.75f)) * 960 / pi - width.value, .002f);
    for (int i = 0; i < 720; i++) {
        const float angle = (i + .5f) * pi / 360 - pi;
        const float x = 959.5f + angle * 960 / pi;
        if (fabsf(x - optimal_x) <= 2) continue;
        const float relative = angle + (center ? velocity_yaw : 0) - wish_yaw;
        const float projection = 400 * cosf(relative);
        const float acceleration = min(2.4f, max(300 - projection, 0));
        const bool expected = side * sinf(relative) > 0 &&
                              2 * acceleration * projection + acceleration * acceleration > .001f;
        bool actual = false;
        for (int j = 0; j < drawn_count; j++) {
            if (drawn_rectangles[j].id == shc_ElementId_AcceleratingAngles &&
                x >= drawn_rectangles[j].x && x <= drawn_rectangles[j].x + drawn_rectangles[j].width)
                actual = true;
        }
        assert(actual == expected);
    }
}

static void CheckUps(void)
{
    static cvar_t predict = { .integer = 1 };
    static cvar_t enabled = { .integer = 1 };
    static cvar_t three_d, mode = { .string = "gradient" };
    static cvar_t color = { .string = "255 255 255 255" };
    cl_predict = &predict;
    cl_strafehelperUps = &enabled;
    cl_strafehelperUps3D = &three_d;
    cl_strafehelperUpsColorMode = &mode;
    cl_strafehelperUpsColorNeutral = &color;
    cl_strafehelperUpsColorGain = &color;
    cl_strafehelperUpsColorLoss = &color;
    cl.clientNum = cl.frame.clientNum = 1;
    cl.frame.ps.pmove.pm_flags = 0;
    cls.demo.playback = false;
    VectorSet(cl.predicted_velocity, 1000, 0, 0);
    VectorSet(cl.frame.ps.pmove.velocity, 480, 640, 960);
    float speed;
    assert(SH_Ups_GetSpeed(&speed));
    CheckNear(speed, 1000, .0001f);
    cl.frame.ps.pmove.pm_flags = PMF_NO_PREDICTION;
    assert(SH_Ups_GetSpeed(&speed));
    CheckNear(speed, 100, .0001f);
    three_d.integer = 1;
    assert(SH_Ups_GetSpeed(&speed));
    CheckNear(speed, sqrtf(24400), .0001f);
    three_d.integer = 0;
    cl.frame.ps.pmove.pm_flags = 0;
    predict.integer = 0;
    assert(SH_Ups_GetSpeed(&speed));
    CheckNear(speed, 100, .0001f);
    predict.integer = 1;
    cls.demo.playback = true;
    assert(SH_Ups_GetSpeed(&speed));
    CheckNear(speed, 100, .0001f);
    cls.demo.playback = false;
    cl.frame.clientNum = 2;
    assert(SH_Ups_GetSpeed(&speed));
    CheckNear(speed, 100, .0001f);
    cl.frame.clientNum = CLIENTNUM_NONE;
    assert(!SH_Ups_GetSpeed(&speed));
    cl.frame.clientNum = 1;

    for (int tick = 1; tick <= 10; tick += 9) {
        sh_ups_history.valid = false;
        cls.realtime = 1000;
        cls.frametime = tick * .001f;
        SH_Ups_ColorForSpeed(100);
        cls.realtime += 20;
        SH_Ups_ColorForSpeed(120);
        CheckNear(sh_ups_history.acceleration, 150, .001f);
        SH_Ups_ColorForSpeed(999); // Same time cannot consume a new baseline.
        CheckNear(sh_ups_history.acceleration, 150, .001f);
        CheckNear(sh_ups_history.speed, 120, .001f);
        cls.realtime += 20;
        SH_Ups_ColorForSpeed(140);
        CheckNear(sh_ups_history.acceleration, 277.5f, .001f);
    }
    cl.frame.ps.pmove.pm_flags = PMF_NO_PREDICTION;
    cls.realtime += 20;
    SH_Ups_ColorForSpeed(10);
    CheckNear(sh_ups_history.acceleration, 0, .0001f);
    cl.frame.ps.pmove.pm_flags = 0;
    cls.realtime += 20;
    SH_Ups_ColorForSpeed(1000);
    CheckNear(sh_ups_history.acceleration, 0, .0001f);
    three_d.integer = 1;
    SH_Ups_ColorForSpeed(1500);
    CheckNear(sh_ups_history.acceleration, 0, .0001f);
    three_d.integer = 0;

    cl.frame.clientNum = CLIENTNUM_NONE;
    SH_Ups_Draw(1920, 1080, 1, 0);
    assert(!sh_ups_history.valid);
    cl.frame.clientNum = 1;
    SH_Ups_ColorForSpeed(100);
    cls.realtime += 20;
    SH_Ups_ColorForSpeed(120);
    enabled.integer = 0;
    SH_Ups_Draw(1920, 1080, 1, 0);
    assert(!sh_ups_history.valid);
    enabled.integer = 1;

    // The unsigned display clock remains valid across its wrap.
    cls.realtime = UINT_MAX - 9;
    SH_Ups_ColorForSpeed(100);
    cls.realtime = 10;
    SH_Ups_ColorForSpeed(120);
    CheckNear(sh_ups_history.acceleration, 150, .001f);
}

static void CheckEfficiencyBridge(void)
{
    Setup(0);
    efficiency_updates = 0;
    sh_efficiency = (StrafeEfficiency) { .valid = true, .efficiency = .75f };
    sh_efficiency_realtime = cls.realtime;
    StrafeHelper_UpdateEfficiency();
    assert(efficiency_updates == 1 && efficiency_update_valid);
    CheckNear(efficiency_update_value, .75f, .00001f);
    cls.realtime++;
    StrafeHelper_UpdateEfficiency();
    assert(efficiency_updates == 2 && !efficiency_update_valid);
    StrafeHelper_ClearEfficiency();
    StrafeHelper_UpdateEfficiency();
    assert(efficiency_updates == 3 && !efficiency_update_valid);
}

static void CheckUpsViewport(void)
{
    cvar_t x = { .value = 800, .integer = 800, .string = "800" };
    cvar_t y = { .value = 600, .integer = 600, .string = "600" };
    cvar_t scale = { .value = 1 }, color = { .string = "static" };
    cvar_t *old_x = cl_strafehelperUpsX, *old_y = cl_strafehelperUpsY;
    cvar_t *old_scale = cl_strafehelperUpsScale, *old_mode = cl_strafehelperUpsColorMode;
    cl_strafehelperUpsX = &x;
    cl_strafehelperUpsY = &y;
    cl_strafehelperUpsScale = &scale;
    cl_strafehelperUpsColorMode = &color;
    test_cvar_writes = 0;
    SH_Ups_Draw(640, 480, 1, 0);
    assert(last_ups_x == 960 && last_ups_y == (480 - CHAR_HEIGHT) / 2 + 480);
    SH_Ups_Draw(1280, 960, 1, 0);
    assert(last_ups_x == 1440 && last_ups_y == (960 - CHAR_HEIGHT) / 2 + 600);
    assert(x.value == 800 && x.integer == 800 && !strcmp(x.string, "800"));
    assert(y.value == 600 && y.integer == 600 && !strcmp(y.string, "600"));
    assert(!x.modified && !y.modified && test_cvar_writes == 0);
    cl_strafehelperUpsX = old_x;
    cl_strafehelperUpsY = old_y;
    cl_strafehelperUpsScale = old_scale;
    cl_strafehelperUpsColorMode = old_mode;
}

int main(void)
{
    CheckEfficiencyBridge();
    CheckUps();
    CheckUpsViewport();
    for (int style = 0; style < 2; style++) {
        test_gradient = style != 0;
        for (int center = 0; center <= 1; center++) {
            for (int side = -1; side <= 1; side += 2) {
                for (int rotation = 0; rotation < 4; rotation++) {
                    const float yaw = rotation * (float)M_PI * .5f;
                    const float velocity_yaw = side * (float)M_PI * (2.0f / 3.0f);
                    CheckDrawing(yaw, (float)M_PI - .0001f, velocity_yaw, center, 0);
                    CheckDrawing(yaw, (float)M_PI, velocity_yaw, center, 2);
                    CheckDrawing(yaw, (float)M_PI + .0001f, velocity_yaw, center, 2);
                    CheckDrawing(yaw, side * 150 * (float)M_PI / 180,
                                 side * 170 * (float)M_PI / 180, center, 2);
                    CheckDrawing(yaw, side * 124 * (float)M_PI / 180,
                                 side * 150 * (float)M_PI / 180, center, 2);
                }
            }
        }
    }
    assert(saw_clipped_gradient);

    /* A marker crossing +/-pi has one fragment at each end of the angular strip. */
    for (int marker = shc_ElementId_OptimalAngle; marker <= shc_ElementId_CenterMarker; marker++) {
        const struct StrafeHelperParams params = { .scale = .5f, .height = 12 };
        drawn_count = 0;
        drawAngleMarker((float)M_PI, 4, 0, 12, &params, 1920, marker, false);
        assert(drawn_count == 2);
        CheckNear(drawn_rectangles[0].x, 479.5f, .001f);
        CheckNear(drawn_rectangles[0].width, 2, .001f);
        CheckNear(drawn_rectangles[1].x, 1437.5f, .001f);
        CheckNear(drawn_rectangles[1].width, 2, .001f);
    }
    CheckMath(400, 1, 0, 300, 2.4f); /* Normal 8 ms air acceleration. */
    CheckMath(400, 1, 0, 300, 4.8f); /* Normal 16 ms. */
    CheckMath(400, 1, 0, 30, 24);    /* Capped air 8 ms. */
    CheckMath(400, 1, 0, 30, 48);    /* Capped air 16 ms: budget exceeds target. */
    CheckMath(400, 1, 0, 30, 600);   /* Capped air 200 ms: capped loss boundary. */
    CheckMath(400, 1, 0, 300, 600);  /* Ground 200 ms. */
    CheckMath(400, 1, 0, 300, 750);  /* Ground 250 ms: budget exceeds twice target. */
    CheckMath(10, 1, 0, 300, 2.4f);  /* Acceleration directly along slow velocity. */
    CheckMath(400, .8f, 50, 300, 48); /* Shallow vertical current. */
    CheckMath(400, .8f, 50, 300, 480);
    CheckMath(400, .8f, 50, 300, 750);

    Setup(0);
    Sample(400, 0, 1, 0, 300, 2.4f);
    CheckNear(fabsf(sh.angle_diff), acosf(297.6f / 400), .00001f);
    assert(fabsf(sh.angle_maximum - sh.angle_minimum) > .8f);
    Sample(400, -.01f, 1, 0, 300, 2.4f);
    const float side = sh_previous_side;
    Sample(400, 0, 1, 0, 300, 2.4f);
    assert(sh_previous_side == side); /* Exact alignment keeps the last selected side. */

    Setup(2);
    Sample(400, -.00001f, 1, 0, 300, 2.4f);
    for (int i = 0; i < 16; i++) {
        cls.realtime += 8;
        Sample(400, .00001f, 1, 0, 300, 2.4f);
        CheckInterval();
        CheckNear(fabsf(sh.angle_maximum - sh.angle_minimum),
                  acosf(-2.4f / 800) - acosf(.75f), .00001f);
    }
    CheckNear(SmoothRun(16, 1), SmoothRun(8, 2), .00001f);

    /* Rotating the wish vector across atan2's wrap keeps the same interval. */
    Setup(2);
    float previous_diff = 0.0f;
    for (int i = 0; i < 2; i++) {
        const float yaw = (i ? -179.99f : 179.99f) * ((float)M_PI / 180.0f);
        const vec3_t forward = { 1, 0, 0 };
        const vec3_t wish = { cosf(yaw), sinf(yaw), 0 };
        const vec3_t vel = { 400 * cosf(yaw - 1), 400 * sinf(yaw - 1), 0 };
        cls.realtime += 16;
        StrafeHelper_BeginPrediction();
        StrafeHelper_SetAccelerationValues(forward, vel, wish, 300, 300, 1, .008f);
        StrafeHelper_EndPrediction();
        CheckInterval();
        if (i) CheckNear(sh.angle_diff, previous_diff, .00001f);
        previous_diff = sh.angle_diff;
    }

    Setup(2);
    Sample(400, .5f, 1, 0, 300, 2.4f);
    const StrafeHelper previous = sh;
    SetSample(900, -1, 1, 0, 30, 48); /* History/entity calls cannot change the HUD. */
    assert(!memcmp(&sh, &previous, sizeof(sh)));
    const vec3_t velocity = { 400, 0, 0 }, wishdir = { 0, 1, 0 };
    StrafeHelper_SetEfficiency(velocity, wishdir, 300, 2.4f);
    assert(!sh_efficiency.valid);
    StrafeHelper_BeginPrediction();
    SetSample(600, .5f, 1, 0, 300, 2.4f);
    SetSample(800, .5f, 1, 0, 300, 2.4f);
    assert(!memcmp(&sh, &previous, sizeof(sh))); /* No smoothing during PMove. */
    cls.realtime += 16;
    StrafeHelper_EndPrediction();
    const float expected = previous.angle_diff + (1 - expf(-.016f / .1f)) *
        (sh_raw.angle_diff - previous.angle_diff);
    CheckNear(sh.angle_diff, expected, .00001f);
    const float once = sh.angle_diff;
    Sample(1000, .5f, 1, 0, 300, 2.4f); /* Zero elapsed time does not advance the filter. */
    CheckNear(sh.angle_diff, once, .00001f);
    StrafeHelper_EndPrediction();
    CheckNear(sh.angle_diff, once, .00001f);

    StrafeHelper_BeginPrediction();
    StrafeHelper_Clear();
    assert(StrafeHelper_IsPredicting());
    StrafeHelper_SetEfficiency(velocity, wishdir, 300, 2.4f);
    StrafeHelper_EndPrediction();
    assert(!StrafeHelper_HasData() && sh_efficiency.valid);
    StrafeHelper_BeginPrediction();
    StrafeHelper_EndPrediction();
    assert(!StrafeHelper_HasData() && !sh_efficiency.valid && !sh_smoothing_initialized);
    Sample(400, .5f, .6f, 400, 300, 48); /* Every horizontal direction loses speed. */
    assert(!StrafeHelper_HasData());

    Setup(0);
    nerdstats_var.integer = 1;
    Sample(400, .5f, 1, 0, 30, 48);
    CheckNear(ns.addspeed_nerd, 30 - 400 * cosf(.5f), .0001f);
    puts("strafe-helper math/prediction tests passed");
    return 0;
}

/* Rendering and configuration are boundary stubs; production math remains real. */
float Cvar_ClampValue(cvar_t *var, float low, float high) { return Test_ClampCvarValue(var, low, high); }
#if USE_UI
bool HUD_EditorPreview(void) { return false; }
bool HUD_EditorShow(int id) { return true; }
float HUD_EditorClamp(cvar_t *var, float low, float high) { return Cvar_ClampValue(var, low, high); }
void HUD_EditorBounds(hud_edit_id_t id, float x, float y, float w, float h) {}
#endif
void R_SetScale(float scale) {}
void R_SetColor(uint32_t color) {}
void R_ClearColor(void) {}
int SCR_DrawStringEx(int x, int y, int flags, size_t maxlen, const char *s, qhandle_t font)
{
    last_ups_x = x;
    last_ups_y = y;
    return x;
}
int Q_strcasecmp(const char *s1, const char *s2) { return strcmp(s1, s2); }
size_t Q_scnprintf(char *dest, size_t size, const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    const int length = vsnprintf(dest, size, fmt, args);
    va_end(args);
    return length < 0 ? 0 : (size_t)length;
}
uint32_t shc_ParseColorString(const char *text, uint8_t *r, uint8_t *g, uint8_t *b, uint8_t *a)
{
    if (r) *r = 255;
    if (g) *g = 255;
    if (b) *b = 255;
    if (a) *a = 255;
    return U32_WHITE;
}
void shc_drawFilledRectangle(float x, float y, float w, float h, enum shc_ElementId id)
{
    assert(drawn_count < q_countof(drawn_rectangles));
    drawn_rectangles[drawn_count].x = x;
    drawn_rectangles[drawn_count].width = w;
    drawn_rectangles[drawn_count].id = id;
    drawn_count++;
}
void shc_drawGradientRectangle(float x, float y, float w, float h, float peak,
                               float gradient_start, float gradient_end,
                               enum shc_ElementId edge, enum shc_ElementId center)
{
    assert(gradient_start <= x + .001f && gradient_end >= x + w - .001f);
    if (gradient_start < x - .001f || gradient_end > x + w + .001f)
        saw_clipped_gradient = true;
    shc_drawFilledRectangle(x, y, w, h, edge);
}
void SH_Efficiency_Update(bool valid, float value)
{
    efficiency_updates++;
    efficiency_update_valid = valid;
    efficiency_update_value = value;
}
void SH_Efficiency_Draw(float y, float h, float width, float scale, int font) {}
void SH_Efficiency_DrawPreview(float y, float h, float width, float scale, int font) {}
cvar_t *cl_predict;
cvar_t *cl_strafehelper_center_width;
cvar_t *cl_strafehelper_optimal_outline;
cvar_t *cl_strafehelper_optimal_width;
cvar_t *cl_strafehelperBarStyle;
cvar_t *cl_strafehelperNerdStats;
cvar_t *cl_strafehelperSmoothing;
cvar_t *cl_strafehelperSmoothingMode;
cvar_t *cl_strafehelperUps;
cvar_t *cl_strafehelperUps3D;
cvar_t *cl_strafehelperUpsColorGain;
cvar_t *cl_strafehelperUpsColorLoss;
cvar_t *cl_strafehelperUpsColorMode;
cvar_t *cl_strafehelperUpsColorNeutral;
cvar_t *cl_strafehelperUpsFormat;
cvar_t *cl_strafehelperUpsHideZero;
cvar_t *cl_strafehelperUpsScale;
cvar_t *cl_strafehelperUpsShadow;
cvar_t *cl_strafehelperUpsX;
cvar_t *cl_strafehelperUpsY;
