/* Exercise production efficiency drawing, tint, timing, and preview isolation. */
#include "../src/jump/sh_efficiency_draw.c"
#include "../src/jump/strafe_helper_customization.c"
#undef NDEBUG
#include <assert.h>
#include <limits.h>
#include "cvar_clamp_stub.h"

client_state_t cl;
client_static_t cls;
bool sh_drawing_preview;

cvar_t *cl_strafehelperEfficiency = &(cvar_t) { .integer = 1 };
cvar_t *cl_strafehelperEffStyle = &(cvar_t) { .string = "bar" };
cvar_t *cl_strafehelperEffWidth = &(cvar_t) { .value = 80 };
cvar_t *cl_strafehelperEffHeight = &(cvar_t) { .value = 4 };
cvar_t *cl_strafehelperEffX;
cvar_t *cl_strafehelperEffY;
cvar_t *cl_strafehelperEffBorder = &(cvar_t) { 0 };
cvar_t *cl_strafehelperEffMarker = &(cvar_t) { .value = .9f };
cvar_t *cl_strafehelperEffColorMode = &(cvar_t) { .string = "dynamic" };
cvar_t *cl_strafehelperEffColorGood = &(cvar_t) { .string = "80 220 90 255" };
cvar_t *cl_strafehelperEffColorMid = &(cvar_t) { .string = "235 200 60 255" };
cvar_t *cl_strafehelperEffColorBad = &(cvar_t) { .string = "235 70 60 255" };
cvar_t *cl_strafehelperEffColorBg = &(cvar_t) { .string = "40 40 40 200" };
cvar_t *cl_strafehelperEffColorMidpoint = &(cvar_t) { .value = .5f };
cvar_t *cl_strafehelperEffSmoothing = &(cvar_t) { 0 };
cvar_t *cl_strafehelperEffHoldMs = &(cvar_t) { .value = 150 };
cvar_t *cl_strafehelperEffTextScale;
cvar_t *cl_strafehelperEffTint = &(cvar_t) { .string = "both" };
cvar_t *cl_strafehelperEffTintStrength = &(cvar_t) { .value = 1 };
cvar_t *cl_strafehelperAlpha = &(cvar_t) { .value = 1 };
cvar_t *cl_strafehelper_color_accelerating = &(cvar_t) { .string = "10 20 30 255" };
cvar_t *cl_strafehelper_color_optimal = &(cvar_t) { .string = "40 50 60 255" };
cvar_t *cl_strafehelper_color_centermarker = &(cvar_t) { .string = "70 80 90 255" };
cvar_t *cl_strafehelper_color_nerdstats = &(cvar_t) { .string = "50 0 50 100" };

static struct {
    int x, y, width, height;
    uint32_t color;
} rectangles[64];
static int rectangle_count, text_count;
static char last_text[8];
static bool editor_preview;
static const cvar_t *draft_var;
static float draft_value;

typedef struct {
    float displayed, held;
    bool displayed_valid, have_held;
    unsigned last_valid_ms, last_update_ms;
} LiveState;

static LiveState SaveState(void)
{
    return (LiveState) { eff_displayed, eff_held_value, eff_displayed_valid,
                         eff_have_held, eff_last_valid_ms, eff_last_update_ms };
}

static void CheckState(const LiveState state)
{
    assert(eff_displayed == state.displayed && eff_held_value == state.held);
    assert(eff_displayed_valid == state.displayed_valid && eff_have_held == state.have_held);
    assert(eff_last_valid_ms == state.last_valid_ms && eff_last_update_ms == state.last_update_ms);
}

static void CheckNear(float actual, float expected)
{
    if (fabsf(actual - expected) > .00001f) {
        fprintf(stderr, "expected %.9f, got %.9f\n", expected, actual);
        abort();
    }
}

static void ClearDrawing(void)
{
    rectangle_count = text_count = 0;
    last_text[0] = 0;
}

static void Reset(float smoothing)
{
    memset(&cl, 0, sizeof(cl));
    memset(&cls, 0, sizeof(cls));
    cls.realtime = 1000;
    cls.frametime = .010f;
    cl.localmove[0] = 300;
    cl.frame.ps.pmove.pm_type = PM_NORMAL;
    eff_displayed = eff_held_value = 0;
    eff_displayed_valid = eff_have_held = false;
    eff_last_valid_ms = eff_last_update_ms = 0;
    sh_drawing_preview = editor_preview = false;
    cl_strafehelperEfficiency->integer = 1;
    cl_strafehelperEffStyle->string = "bar";
    cl_strafehelperEffSmoothing->value = smoothing;
    cl_strafehelperEffHoldMs->value = 150;
    cl_strafehelperEffMarker->value = .9f;
    cl_strafehelperEffBorder->integer = 0;
    cl_strafehelperEffTint->string = "both";
    ClearDrawing();
}

static void Draw(bool valid, float value)
{
    ClearDrawing();
    SH_Efficiency_Update(valid, value);
    SH_Efficiency_Draw(100, 12, 1920, 1, 0);
}

static float RunCadence(unsigned step, float main_step)
{
    Reset(1);
    cls.frametime = main_step;
    Draw(true, 0);
    for (unsigned elapsed = 0; elapsed < 100; elapsed += step) {
        cls.realtime += step;
        Draw(true, 1);
    }
    return eff_displayed;
}

static void CheckClock(void)
{
    const float expected = 1 - expf(-.100f / .05f);
    CheckNear(RunCadence(20, .010f), expected);
    CheckNear(RunCadence(20, .003f), expected);
    CheckNear(RunCadence(1, .001f), expected);
    CheckNear(RunCadence(10, .010f), expected);

    Reset(1);
    Draw(true, 0);
    const unsigned intervals[] = { 1, 7, 13, 29, 50 };
    for (unsigned i = 0; i < q_countof(intervals); i++) {
        cls.realtime += intervals[i];
        Draw(true, 1);
    }
    CheckNear(eff_displayed, expected);
    const float once = eff_displayed;
    Draw(true, .2f);
    CheckNear(eff_displayed, once); /* A duplicate draw cannot advance the filter. */
    assert(eff_last_update_ms == cls.realtime);

    cl_strafehelperEffSmoothing->value = 0;
    Draw(true, .25f);
    CheckNear(eff_displayed, .25f); /* Disabled smoothing still snaps at zero dt. */

    Reset(1);
    cls.realtime = UINT_MAX - 4;
    Draw(true, 0);
    cls.realtime += 10;
    Draw(true, 1);
    CheckNear(eff_displayed, 1 - expf(-.010f / .05f));
}

static void CheckHold(void)
{
    Reset(2);
    Draw(true, .75f);
    const unsigned valid_time = eff_last_valid_ms;
    cls.realtime += 100;
    Draw(false, 0);
    CheckNear(eff_displayed, .75f);
    assert(eff_last_valid_ms == valid_time && eff_last_update_ms == cls.realtime);
    const LiveState held = SaveState();
    (void)getColorForElement(shc_ElementId_OptimalAngle);
    CheckState(held); /* Tint queries cannot extend either timestamp. */

    cls.realtime += 50;
    Draw(false, 0);
    assert(eff_displayed_valid && eff_last_valid_ms == valid_time);
    cls.realtime++;
    Draw(false, 0);
    assert(!eff_displayed_valid && !eff_have_held && eff_last_update_ms == 0);
    assert(rectangle_count == 2); /* Background and marker remain; value has expired. */
    const uint32_t base = shc_ParseColorString(cl_strafehelper_color_optimal->string, NULL, NULL, NULL, NULL);
    assert(getColorForElement(shc_ElementId_OptimalAngle) == base);

    cls.realtime += 1000;
    Draw(true, .3f);
    CheckNear(eff_displayed, .3f); /* Expiry reseeds instead of filtering stale history. */
    assert(eff_last_update_ms == cls.realtime && eff_last_valid_ms == cls.realtime);

    Reset(2);
    cl_strafehelperEffHoldMs->value = 0;
    Draw(true, .8f);
    Draw(false, 0);
    assert(eff_displayed_valid);
    cls.realtime++;
    Draw(false, 0);
    assert(!eff_displayed_valid);
}

static void CheckMarker(void)
{
    const float values[] = { .5f, .9f, 1 };
    char *const styles[] = { "bar", "both" };
    for (unsigned style = 0; style < q_countof(styles); style++) {
        for (unsigned i = 0; i < q_countof(values); i++) {
            Reset(0);
            cl_strafehelperEffStyle->string = styles[style];
            Draw(true, values[i]);
            assert(rectangle_count == 3);
            assert(rectangles[1].width == Q_rint(80 * values[i]));
            assert(rectangles[2].x == 992 && rectangles[2].width == 1);
            assert(rectangles[2].color == MakeColor(200, 200, 200, 160));
            assert(text_count == (int)style);
        }
    }
    cl_strafehelperEffBorder->integer = 1;
    Draw(true, 1);
    assert(rectangle_count == 4 && rectangles[3].width == 1);
    cl_strafehelperEffMarker->value = 0;
    Draw(true, 1);
    assert(rectangle_count == 3 && rectangles[2].width == 80);
    cl_strafehelperEffStyle->string = "text";
    Draw(true, 1);
    assert(rectangle_count == 0 && text_count == 1 && !strcmp(last_text, "100%"));
    cl_strafehelperEffStyle->string = "none";
    Draw(true, 1);
    assert(rectangle_count == 0 && text_count == 0);
}

static void CheckPreview(bool editor)
{
    Reset(1);
    Draw(true, .2f);
    const LiveState live = SaveState();
    const unsigned times[] = { 1050, 1300, 1800 };
    for (unsigned i = 0; i < q_countof(times); i++) {
        cls.realtime = times[i];
        editor_preview = editor;
        sh_drawing_preview = !editor;
        ClearDrawing();
        SH_Efficiency_DrawPreview(100, 12, 1920, 1, 0);
        assert(sh_drawing_preview == !editor);
        const float value = editor ? .78f : .65f + .35f * sinf(cls.realtime * .0015f);
        assert(rectangles[1].width == Q_rint(80 * value));
        assert(getColorForElement(shc_ElementId_OptimalAngle) == rectangles[1].color);
        assert(getColorForElement(shc_ElementId_AcceleratingAngles) == rectangles[1].color);
        CheckState(live);
        cl_strafehelperEffStyle->string = "none";
        Draw(false, 0);
        assert(rectangle_count == 0 && text_count == 0);
        assert(SH_Efficiency_ApplyTint(shc_ElementId_OptimalAngle, U32_WHITE) == SH_Efficiency_FillColor(value));
        CheckState(live);
        cl_strafehelperEffStyle->string = "bar";
    }
    sh_drawing_preview = editor_preview = false;
    Draw(false, 0);
    assert(!eff_have_held && !eff_displayed_valid); /* Old hold expired during preview. */

    Reset(1);
    Draw(true, .2f);
    cls.realtime += 50;
    SH_Efficiency_DrawPreview(100, 12, 1920, 1, 0); /* Public preview is safe without an outer flag. */
    assert(!sh_drawing_preview);
    Draw(false, 0);
    CheckNear(eff_displayed, .2f);
    assert(eff_last_valid_ms == 1000);

    Reset(1);
    Draw(true, 0);
    cls.realtime += 1000;
    SH_Efficiency_DrawPreview(100, 12, 1920, 1, 0);
    Draw(true, 1);
    CheckNear(eff_displayed, 1 - expf(-1.0f / .05f)); /* No artificial menu-gap cap. */
}

static void CheckPreviewWithoutLiveData(void)
{
    Reset(0);
    const LiveState empty = SaveState();
    sh_drawing_preview = true;
    Draw(false, 0);
    assert(rectangle_count == 3);
    assert(getColorForElement(shc_ElementId_OptimalAngle) == rectangles[1].color);
    CheckState(empty);
    cl_strafehelperEfficiency->integer = 0;
    Draw(false, 0);
    assert(rectangle_count == 0);
    CheckState(empty);
#if USE_UI
    editor_preview = true;
    Draw(false, 0);
    assert(rectangle_count == 3); /* Editor still previews a disabled widget. */
    CheckState(empty);
#endif
}

static void CheckReadOnlyDrawing(void)
{
    Reset(1);
    SH_Efficiency_Update(true, .2f);
    cls.realtime += 50;
    SH_Efficiency_Update(true, .8f);
    const LiveState live = SaveState();
    const uint32_t before = getColorForElement(shc_ElementId_OptimalAngle);
    ClearDrawing();
    SH_Efficiency_Draw(100, 12, 1920, 1, 0);
    assert(rectangles[1].color == before);
    assert(getColorForElement(shc_ElementId_OptimalAngle) == before);
    SH_Efficiency_Draw(100, 12, 1920, 1, 0);
    CheckState(live);

    cl_strafehelperEffStyle->string = "none";
    cls.realtime += 50;
    SH_Efficiency_Update(true, .9f);
    const LiveState tint_only = SaveState();
    const uint32_t tint = getColorForElement(shc_ElementId_OptimalAngle);
    ClearDrawing();
    SH_Efficiency_Draw(100, 12, 1920, 1, 0);
    assert(rectangle_count == 0 && text_count == 0);
    assert(getColorForElement(shc_ElementId_OptimalAngle) == tint);
    CheckState(tint_only);

    SH_Efficiency_Update(true, .4f);
    CheckNear(eff_displayed, tint_only.displayed);
    CheckNear(eff_held_value, .4f); // A newer sample at the same time updates the target.
    cl_strafehelperEffSmoothing->value = 0;
    SH_Efficiency_Update(true, .6f);
    CheckNear(eff_displayed, .6f);

    const unsigned valid_time = eff_last_valid_ms;
    cls.realtime += 100;
    SH_Efficiency_Update(true, NAN);
    assert(eff_last_valid_ms == valid_time);
    CheckNear(eff_displayed, .6f);
    cls.realtime += 51;
    const LiveState expired = SaveState();
    SH_Efficiency_Draw(100, 12, 1920, 1, 0);
    assert(SH_Efficiency_ApplyTint(shc_ElementId_OptimalAngle, U32_WHITE) == U32_WHITE);
    CheckState(expired); // Read-only queries hide expired values without clearing state.
    SH_Efficiency_Update(true, INFINITY);
    assert(!eff_have_held && !eff_displayed_valid);

    Reset(0);
    SH_Efficiency_Update(true, NAN);
    assert(!eff_have_held && !eff_displayed_valid);
}

static void CheckViewportPreservesSettings(void)
{
    Reset(0);
    cvar_t width = { .value = 800, .integer = 800, .string = "800" };
    cvar_t x = { .value = 500, .integer = 500, .string = "500" };
    cvar_t *old_width = cl_strafehelperEffWidth, *old_x = cl_strafehelperEffX;
    cl_strafehelperEffWidth = &width;
    cl_strafehelperEffX = &x;
    SH_Efficiency_Update(true, .5f);
    test_cvar_writes = 0;
    const float viewports[] = { 640, 1280 };
    const int expected_widths[] = { 640, 800 };
    const int expected_x[] = { 320, 740 };
    for (unsigned i = 0; i < q_countof(viewports); i++) {
        ClearDrawing();
        SH_Efficiency_Draw(100, 12, viewports[i], 1, 0);
        assert(rectangles[0].width == expected_widths[i]);
        assert(rectangles[0].x == expected_x[i]);
        assert(width.value == 800 && width.integer == 800 && !strcmp(width.string, "800"));
        assert(x.value == 500 && x.integer == 500 && !strcmp(x.string, "500"));
        assert(!width.modified && !x.modified && test_cvar_writes == 0);
    }
#if USE_UI
    editor_preview = true;
    draft_var = &width;
    draft_value = 1000;
    ClearDrawing();
    SH_Efficiency_Draw(100, 12, 640, 1, 0);
    assert(rectangles[0].width == 640 && draft_value == 1000);
    ClearDrawing();
    SH_Efficiency_Draw(100, 12, 1280, 1, 0);
    assert(rectangles[0].width == 1000 && draft_value == 1000);
    assert(width.value == 800 && !strcmp(width.string, "800") && test_cvar_writes == 0);
    draft_var = NULL;
    editor_preview = false;
#endif
    cl_strafehelperEffWidth = old_width;
    cl_strafehelperEffX = old_x;

    // The boundary stub must mutate state for a real fixed-range correction.
    cvar_t invalid = { .value = 4, .integer = 4, .string = "4" };
    assert(Cvar_ClampValue(&invalid, 8, 4000) == 8);
    assert(invalid.value == 8 && invalid.integer == 8 && !strcmp(invalid.string, "8"));
    assert(invalid.modified && test_cvar_writes == 1);
}

static void CheckPreviewPreservesInvalidSettings(void)
{
    Reset(1);
    Draw(true, .4f);
    const LiveState live = SaveState();
    cvar_t x = { 0 }, y = { 0 }, text_scale = { 0 };
    cvar_t *old_x = cl_strafehelperEffX, *old_y = cl_strafehelperEffY;
    cvar_t *old_text_scale = cl_strafehelperEffTextScale;
    cl_strafehelperEffX = &x;
    cl_strafehelperEffY = &y;
    cl_strafehelperEffTextScale = &text_scale;
    cvar_t *vars[] = { cl_strafehelperEffWidth, cl_strafehelperEffHeight,
        &x, &y, &text_scale, cl_strafehelperEffMarker, cl_strafehelperEffHoldMs,
        cl_strafehelperEffTintStrength, cl_strafehelperAlpha };
    cvar_t saved[q_countof(vars)];
    for (size_t i = 0; i < q_countof(vars); i++) {
        saved[i] = *vars[i];
        vars[i]->value = 99999;
        vars[i]->integer = 99999;
        vars[i]->string = "99999";
        vars[i]->modified = false;
    }
    cl_strafehelperEffStyle->string = "both";
    test_cvar_writes = 0;
    for (int editor = 0; editor <= USE_UI; editor++) {
        editor_preview = editor != 0;
        sh_drawing_preview = !editor;
        ClearDrawing();
        SH_Efficiency_DrawPreview(100, 12, 640, 1, 0);
        (void)getColorForElement(shc_ElementId_OptimalAngle);
        SH_Efficiency_Update(true, .9f);
        CheckState(live);
        for (size_t i = 0; i < q_countof(vars); i++) {
            assert(vars[i]->value == 99999 && vars[i]->integer == 99999);
            assert(!strcmp(vars[i]->string, "99999") && !vars[i]->modified);
        }
        assert(test_cvar_writes == 0);
    }
    sh_drawing_preview = editor_preview = false;
    for (size_t i = 0; i < q_countof(vars); i++)
        *vars[i] = saved[i];
    cl_strafehelperEffX = old_x;
    cl_strafehelperEffY = old_y;
    cl_strafehelperEffTextScale = old_text_scale;
}

int main(void)
{
    CheckPreviewPreservesInvalidSettings();
    CheckReadOnlyDrawing();
    CheckViewportPreservesSettings();
    CheckClock();
    CheckHold();
    CheckMarker();
    CheckPreview(false);
#if USE_UI
    CheckPreview(true);
#endif
    CheckPreviewWithoutLiveData();
    puts("strafe-efficiency drawing, timing, and preview tests passed");
    return 0;
}

/* Renderer, client configuration, and editor boundaries only. */
float Cvar_ClampValue(cvar_t *var, float low, float high) { return Test_ClampCvarValue(var, low, high); }
#if USE_UI
bool HUD_EditorPreview(void) { return editor_preview; }
bool HUD_EditorShow(int id) { return true; }
float HUD_EditorValue(const cvar_t *var) { return var == draft_var ? draft_value : var->value; }
float HUD_EditorClamp(cvar_t *var, float low, float high)
{
    if (editor_preview)
        return SH_ClampDrawValue(var == draft_var ? draft_value : var->value, low, high);
    return Cvar_ClampValue(var, low, high);
}
void HUD_EditorBounds(hud_edit_id_t id, float x, float y, float width, float height) {}
#endif
void R_DrawFill32(int x, int y, int width, int height, uint32_t color)
{
    assert(rectangle_count < q_countof(rectangles));
    rectangles[rectangle_count].x = x;
    rectangles[rectangle_count].y = y;
    rectangles[rectangle_count].width = width;
    rectangles[rectangle_count].height = height;
    rectangles[rectangle_count].color = color;
    rectangle_count++;
}
void R_SetScale(float scale) {}
void R_SetColor(uint32_t color) {}
void R_ClearColor(void) {}
int SCR_DrawStringEx(int x, int y, int flags, size_t maxlen, const char *text, qhandle_t font)
{
    text_count++;
    snprintf(last_text, sizeof(last_text), "%s", text);
    return x;
}
int Q_strcasecmp(const char *left, const char *right) { return strcmp(left, right); }
size_t Q_scnprintf(char *dest, size_t size, const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    const int length = vsnprintf(dest, size, fmt, args);
    va_end(args);
    return length < 0 || !size ? 0 : min((size_t)length, size - 1);
}
