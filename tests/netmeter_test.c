/* Exercise production network sampling and drawing with platform boundaries stubbed. */
#include "../src/jump/sh_netmeter.c"
#undef NDEBUG
#include <assert.h>
#include "cvar_clamp_stub.h"

client_state_t cl;
client_static_t cls;
scr_t scr;
uiStatic_t uis;

#define TEST_CVAR(name) static cvar_t name##_storage; cvar_t *name = &name##_storage
TEST_CVAR(scr_alpha);
TEST_CVAR(sh_netmeter);
TEST_CVAR(sh_netwarn_ping_adaptive);
TEST_CVAR(sh_netwarn_spike_ms);
TEST_CVAR(sh_netwarn_spike_pct);
TEST_CVAR(sh_netwarn_jitter_ms);
TEST_CVAR(sh_netalert);
TEST_CVAR(sh_netalert_x);
TEST_CVAR(sh_netalert_y);
TEST_CVAR(sh_netalert_color);
TEST_CVAR(sh_netalert_alpha);
TEST_CVAR(sh_netalert_duration_ms);
TEST_CVAR(sh_netalert_loss);
TEST_CVAR(sh_netalert_jitter);
TEST_CVAR(sh_netalert_spike);
TEST_CVAR(sh_lagometer_x);
TEST_CVAR(sh_lagometer_y);
TEST_CVAR(sh_netmeter_min_ms);
TEST_CVAR(sh_netmeter_max_ms);
TEST_CVAR(sh_netmeter_adaptive);
TEST_CVAR(sh_lagometer_color_normal);
TEST_CVAR(sh_lagometer_color_spike);
TEST_CVAR(sh_lagometer_color_jitter);
TEST_CVAR(sh_lagometer_color_loss_s2c);
TEST_CVAR(sh_lagometer_color_loss_c2s);
TEST_CVAR(sh_lagometer_alpha);
TEST_CVAR(sh_lagometer_bad_alpha);
TEST_CVAR(sh_netgraph_y);
TEST_CVAR(sh_netgraph_height);
TEST_CVAR(sh_netgraph_alpha);
TEST_CVAR(sh_netgraph_color_normal);
TEST_CVAR(sh_netgraph_color_spike);
TEST_CVAR(sh_netgraph_color_jitter);
TEST_CVAR(sh_netgraph_color_loss_s2c);
TEST_CVAR(sh_netgraph_color_loss_c2s);
TEST_CVAR(sh_histogram_x);
TEST_CVAR(sh_histogram_y);
TEST_CVAR(sh_histogram_width_mode);
TEST_CVAR(sh_histogram_width);
TEST_CVAR(sh_histogram_height);
TEST_CVAR(sh_histogram_fill_mode);
TEST_CVAR(sh_histogram_spacing_mode);
TEST_CVAR(sh_histogram_bg_alpha);
TEST_CVAR(sh_histogram_color_bg);
TEST_CVAR(sh_histogram_color_normal);
TEST_CVAR(sh_histogram_color_spike);
TEST_CVAR(sh_histogram_color_jitter);
TEST_CVAR(sh_histogram_color_loss_s2c);
TEST_CVAR(sh_histogram_color_loss_c2s);
TEST_CVAR(sh_histogram_alpha);
TEST_CVAR(sh_histogram_bad_alpha);
TEST_CVAR(sh_histogram_history);
TEST_CVAR(sh_histogram_ping);
#undef TEST_CVAR

static cvar_t *test_cvars[] = { &scr_alpha_storage,
    &sh_netmeter_storage,
    &sh_netwarn_ping_adaptive_storage,
    &sh_netwarn_spike_ms_storage,
    &sh_netwarn_spike_pct_storage,
    &sh_netwarn_jitter_ms_storage,
    &sh_netalert_storage,
    &sh_netalert_x_storage,
    &sh_netalert_y_storage,
    &sh_netalert_color_storage,
    &sh_netalert_alpha_storage,
    &sh_netalert_duration_ms_storage,
    &sh_netalert_loss_storage,
    &sh_netalert_jitter_storage,
    &sh_netalert_spike_storage,
    &sh_lagometer_x_storage,
    &sh_lagometer_y_storage,
    &sh_netmeter_min_ms_storage,
    &sh_netmeter_max_ms_storage,
    &sh_netmeter_adaptive_storage,
    &sh_lagometer_color_normal_storage,
    &sh_lagometer_color_spike_storage,
    &sh_lagometer_color_jitter_storage,
    &sh_lagometer_color_loss_s2c_storage,
    &sh_lagometer_color_loss_c2s_storage,
    &sh_lagometer_alpha_storage,
    &sh_lagometer_bad_alpha_storage,
    &sh_netgraph_y_storage,
    &sh_netgraph_height_storage,
    &sh_netgraph_alpha_storage,
    &sh_netgraph_color_normal_storage,
    &sh_netgraph_color_spike_storage,
    &sh_netgraph_color_jitter_storage,
    &sh_netgraph_color_loss_s2c_storage,
    &sh_netgraph_color_loss_c2s_storage,
    &sh_histogram_x_storage,
    &sh_histogram_y_storage,
    &sh_histogram_width_mode_storage,
    &sh_histogram_width_storage,
    &sh_histogram_height_storage,
    &sh_histogram_fill_mode_storage,
    &sh_histogram_spacing_mode_storage,
    &sh_histogram_bg_alpha_storage,
    &sh_histogram_color_bg_storage,
    &sh_histogram_color_normal_storage,
    &sh_histogram_color_spike_storage,
    &sh_histogram_color_jitter_storage,
    &sh_histogram_color_loss_s2c_storage,
    &sh_histogram_color_loss_c2s_storage,
    &sh_histogram_alpha_storage,
    &sh_histogram_bad_alpha_storage,
    &sh_histogram_history_storage,
    &sh_histogram_ping_storage,
};

enum { COLOR_NORMAL = 201, COLOR_SPIKE, COLOR_JITTER, COLOR_S2C, COLOR_C2S };
static struct { int x, y, w, h, color; float alpha; } rectangles[10000];
static int rectangle_count, text_count, preview_count, checks, failures;
static char last_text[64];
static float draw_alpha;
static bool editor_preview;

#define CHECK(expr) do { checks++; if (!(expr)) { \
    fprintf(stderr, "%s:%d: %s\n", __func__, __LINE__, #expr); failures++; \
} } while (0)

int Cvar_ClampInteger(cvar_t *var, int low, int high) { return Test_ClampCvarInteger(var, low, high); }
float Cvar_ClampValue(cvar_t *var, float low, float high) { return Test_ClampCvarValue(var, low, high); }
bool HUD_EditorPreview(void) { return editor_preview; }
bool HUD_EditorShow(int id) { return true; }
int HUD_EditorNetworkMode(void) { return 3; }
float HUD_EditorValue(const cvar_t *var) { return var->value; }
float HUD_EditorClamp(cvar_t *var, float low, float high)
{
    if (editor_preview)
        return SH_ClampDrawValue(var->value, low, high);
    return Cvar_ClampValue(var, low, high);
}
void HUD_EditorBounds(hud_edit_id_t id, float x, float y, float w, float h) {}
void HUD_LayoutBegin(int id) {}
void HUD_LayoutEnd(void) {}
void R_SetAlpha(float alpha) { draw_alpha = alpha; }
void UI_SetColor_Wrapper(uint32_t color) { draw_alpha = ((color >> 24) & 255) / 255.0f; }
void UI_ClearColor_Wrapper(void) { draw_alpha = 1; }
void UI_DrawFill32_Wrapper(int x, int y, int w, int h, uint32_t color) { preview_count++; }
void R_DrawStretchPic(int x, int y, int w, int h, qhandle_t pic) {}
void R_DrawFill8(int x, int y, int w, int h, int color)
{
    assert(rectangle_count < q_countof(rectangles));
    rectangles[rectangle_count].x = x;
    rectangles[rectangle_count].y = y;
    rectangles[rectangle_count].w = w;
    rectangles[rectangle_count].h = h;
    rectangles[rectangle_count].color = color;
    rectangles[rectangle_count++].alpha = draw_alpha;
}
int SCR_DrawStringEx(int x, int y, int flags, size_t maxlen, const char *text, qhandle_t font)
{
    text_count++;
    snprintf(last_text, sizeof(last_text), "%s", text);
    return x;
}
size_t Q_strlcpy(char *dest, const char *src, size_t size)
{
    size_t len = strlen(src);
    if (size) {
        size_t count = min(len, size - 1);
        memcpy(dest, src, count);
        dest[count] = 0;
    }
    return len;
}
size_t Q_scnprintf(char *dest, size_t size, const char *format, ...)
{
    va_list args;
    va_start(args, format);
    int count = vsnprintf(dest, size, format, args);
    va_end(args);
    return count < 0 || !size ? 0 : min((size_t)count, size - 1);
}

static void Set(cvar_t *var, float value)
{
    var->integer = (int)value;
    var->value = value;
}
static void ResetDrawing(void)
{
    rectangle_count = text_count = preview_count = 0;
    last_text[0] = 0;
    draw_alpha = scr_alpha->value;
}
static void Setup(void)
{
    SH_NetMeter_Clear();
    memset(&cl, 0, sizeof(cl));
    memset(&cls, 0, sizeof(cls));
    memset(&uis, 0, sizeof(uis));
    for (size_t i = 0; i < q_countof(test_cvars); i++)
        memset(test_cvars[i], 0, sizeof(*test_cvars[i]));
    editor_preview = false;
    cls.netchan.protocol = PROTOCOL_VERSION_Q2PRO;
    cls.realtime = 1000;
    scr.hud_width = 640;
    scr.hud_height = 480;
    Set(scr_alpha, 1);
    Set(sh_netmeter, 3);
    Set(sh_netwarn_ping_adaptive, 1);
    Set(sh_netwarn_spike_ms, 300);
    Set(sh_netwarn_spike_pct, 100);
    Set(sh_netwarn_jitter_ms, 100);
    Set(sh_netalert, 1);
    Set(sh_netalert_loss, 1);
    Set(sh_netalert_jitter, 1);
    Set(sh_netalert_spike, 1);
    Set(sh_netalert_duration_ms, 2000);
    Set(sh_netalert_alpha, 1);
    Set(sh_netalert_y, 20);
    Set(sh_netgraph_height, 15);
    Set(sh_netgraph_alpha, 1);
    Set(sh_lagometer_alpha, 1);
    Set(sh_lagometer_bad_alpha, 1);
    Set(sh_histogram_width, 100);
    Set(sh_histogram_height, 15);
    Set(sh_histogram_history, 52000);
    Set(sh_histogram_alpha, 1);
    Set(sh_histogram_bad_alpha, 1);
    Set(sh_netmeter_max_ms, 50);
    Set(sh_lagometer_color_normal, COLOR_NORMAL);
    Set(sh_lagometer_color_spike, COLOR_SPIKE);
    Set(sh_lagometer_color_jitter, COLOR_JITTER);
    Set(sh_lagometer_color_loss_s2c, COLOR_S2C);
    Set(sh_lagometer_color_loss_c2s, COLOR_C2S);
    Set(sh_netgraph_color_normal, COLOR_NORMAL);
    Set(sh_netgraph_color_spike, COLOR_SPIKE);
    Set(sh_netgraph_color_jitter, COLOR_JITTER);
    Set(sh_netgraph_color_loss_s2c, COLOR_S2C);
    Set(sh_netgraph_color_loss_c2s, COLOR_C2S);
    Set(sh_histogram_color_normal, COLOR_NORMAL);
    Set(sh_histogram_color_spike, COLOR_SPIKE);
    Set(sh_histogram_color_jitter, COLOR_JITTER);
    Set(sh_histogram_color_loss_s2c, COLOR_S2C);
    Set(sh_histogram_color_loss_c2s, COLOR_C2S);
    ResetDrawing();
}
static unsigned LatestFlags(void)
{
    return netmeter.samples[(netmeter.head - 1) & NETMETER_MASK].flags;
}
static int ColorCount(int color)
{
    int count = 0;
    for (int i = 0; i < rectangle_count; i++)
        count += rectangles[i].color == color && rectangles[i].w > 0 && rectangles[i].h > 0;
    return count;
}

static void CheckLoss(void)
{
    const unsigned flags[] = { FF_CLIENTDROP, FF_CLIENTPRED, FF_CLIENTDROP | FF_CLIENTPRED };
    for (size_t i = 0; i < q_countof(flags); i++) {
        Setup();
        SH_NetMeter_Sample(30);
        SH_NetMeter_Sample(31);
        unsigned clean = netmeter.ping_samples, avg = netmeter.avg_ping, last = netmeter.last_ping;
        double jitter = netmeter.jitter, baseline = netmeter.jitter_base;
        unsigned head = netmeter.head;
        cl.frameflags = flags[i];
        SH_NetMeter_Sample(80);
        CHECK(LatestFlags() == NETEVENT_PRED);
        CHECK(netmeter.head == head + 1 && netmeter.ping_samples == clean);
        CHECK(netmeter.avg_ping == avg && netmeter.last_ping == last);
        CHECK(netmeter.jitter == jitter && netmeter.jitter_base == baseline);
        CHECK(netmeter.display_ping > 31);
        for (int mode = 1; mode <= 3; mode++) {
            Set(sh_netmeter, mode);
            ResetDrawing();
            SH_NetMeter_Draw();
            CHECK(ColorCount(COLOR_C2S) == 1);
            CHECK(text_count == 1 && !strcmp(last_text, "CLIENT DROP"));
        }
    }
    Setup();
    cl.frameflags = FF_CLIENTPRED | FF_SERVERDROP;
    SH_NetMeter_Sample(40);
    SH_NetMeter_Draw();
    CHECK(LatestFlags() == (NETEVENT_LOSS | NETEVENT_PRED));
    CHECK(ColorCount(COLOR_S2C) == 1 && !strcmp(last_text, "PACKET LOSS"));
    Setup();
    SH_NetMeter_Sample(40);
    CHECK(LatestFlags() == 0 && netmeter.ping_samples == 1);
    cls.demo.playback = true;
    SH_NetMeter_Sample(400);
    CHECK(netmeter.head == 1);
    cls.demo.playback = false;
    cls.netchan.protocol = 0;
    SH_NetMeter_Sample(400);
    CHECK(netmeter.head == 1);
}

static void CheckJitter(void)
{
    for (int adaptive = 0; adaptive <= 1; adaptive++) {
        Setup();
        Set(sh_netwarn_ping_adaptive, adaptive);
        for (int i = 0; i < 64; i++) SH_NetMeter_Sample(100);
        double reference = 0;
        unsigned warnings = 0;
        for (int i = 0; i < 1000; i++) {
            bool expected = reference >= 100;
            SH_NetMeter_Sample(i % 2 ? 100 : 206);
            CHECK(!!(LatestFlags() & NETEVENT_JITTER) == expected);
            reference += (106 - reference) * .125;
            CHECK(fabs(netmeter.jitter - reference) < 1e-10);
            warnings += !!(LatestFlags() & NETEVENT_JITTER);
        }
        CHECK(warnings == 978 && fabs(netmeter.jitter - 106) < 1e-10);
        for (int i = 0; i < 200; i++) {
            SH_NetMeter_Sample(100);
            reference *= .875;
            CHECK(fabs(netmeter.jitter - reference) < 1e-10);
        }
        CHECK(netmeter.jitter < .00001 && !(LatestFlags() & NETEVENT_JITTER));
    }
    Setup();
    SH_NetMeter_Sample(100);
    SH_NetMeter_Sample(107);
    CHECK(netmeter.jitter == .875 && netmeter.jitter_base == .875);
    SH_NetMeter_Sample(107);
    CHECK(netmeter.jitter == .765625 && netmeter.jitter_base == .8681640625);
    Setup();
    for (int i = 0; i < 32; i++) {
        SH_NetMeter_Sample(i % 2 ? 100 : 206);
        CHECK(!(LatestFlags() & NETEVENT_JITTER));
    }
    Setup();
    for (int i = 0; i < 1000; i++) {
        SH_NetMeter_Sample(i % 2 ? 100 : 198);
        CHECK(!(LatestFlags() & NETEVENT_JITTER));
    }
    /* Adaptive comparison must retain the fractional baseline as well. */
    for (int above = 0; above <= 1; above++) {
        Setup();
        netmeter.ping_samples = 32;
        netmeter.avg_ping = netmeter.last_ping = 100;
        netmeter.jitter_base = 25.6;
        netmeter.jitter = above ? 100.7 : 100.4;
        SH_NetMeter_Sample(100);
        CHECK(!!(LatestFlags() & NETEVENT_JITTER) == above);
    }
    SH_NetMeter_Clear();
    CHECK(netmeter.jitter == 0 && netmeter.jitter_base == 0 && netmeter.head == 0);
    SH_NetMeter_PredictionError(1000);
    CHECK(netmeter.head == 0);
}

static void CheckNotices(void)
{
    for (int mode = 0; mode <= 3; mode++) {
        Setup();
        Set(sh_netmeter, mode);
        Set(scr_alpha, .4f);
        SH_NetMeter_PushSample(cls.realtime, 50, NETEVENT_LOSS);
        SH_NetMeter_Draw();
        CHECK(text_count == 1 && !strcmp(last_text, "PACKET LOSS"));
        CHECK(mode || rectangle_count == 0);
        CHECK(fabsf(draw_alpha - .4f) < .0001f);
        Set(sh_netalert, 0);
        ResetDrawing(); SH_NetMeter_Draw();
        CHECK(text_count == 0);
        Set(sh_netalert, 1);
        Set(sh_netalert_loss, 0);
        ResetDrawing(); SH_NetMeter_Draw();
        CHECK(text_count == 0);
        Set(sh_netalert_loss, 1);
        cls.realtime += 2001;
        ResetDrawing(); SH_NetMeter_Draw();
        CHECK(text_count == 0);
    }
    const unsigned flags[] = { NETEVENT_SPIKE, NETEVENT_JITTER, NETEVENT_PRED };
    for (size_t i = 0; i < q_countof(flags); i++) {
        Setup(); Set(sh_netmeter, 0);
        SH_NetMeter_PushSample(cls.realtime, 50, flags[i]);
        SH_NetMeter_Draw(); CHECK(text_count == 1);
        Set(i == 0 ? sh_netalert_spike : i == 1 ? sh_netalert_jitter : sh_netalert_loss, 0);
        ResetDrawing(); SH_NetMeter_Draw(); CHECK(text_count == 0);
    }
    Setup(); Set(sh_netmeter, 0);
    SH_NetMeter_PushSample(cls.realtime, 50, NETEVENT_LOSS);
    cls.demo.playback = true;
    SH_NetMeter_Draw(); CHECK(text_count == 0 && rectangle_count == 0);
    cls.demo.playback = false; cls.netchan.protocol = 0;
    SH_NetMeter_Draw(); CHECK(text_count == 0);
    static menuFrameWork_t menu;
    menu.name = "jumpnetalerts"; uis.activeMenu = &menu;
    SH_NetMeter_Draw(); CHECK(text_count == 1 && !strcmp(last_text, "TEST ALERT"));
    uis.activeMenu = NULL; ResetDrawing(); editor_preview = true;
    unsigned head = netmeter.head;
    SH_NetMeter_Draw();
    CHECK(preview_count > 0 && netmeter.head == head);
}

static void DrawHistogram(void)
{
    ResetDrawing();
    SCR_DrawNetMeterHistogram(1, cls.realtime);
}
static void CheckHistogram(void)
{
    for (int fill = 0; fill <= 1; fill++) {
        for (int order = 0; order <= 1; order++) {
            Setup(); Set(sh_histogram_fill_mode, fill);
            SH_NetMeter_PushSample(cls.realtime, 50, order ? 0 : NETEVENT_LOSS);
            SH_NetMeter_PushSample(cls.realtime, 50, order ? NETEVENT_LOSS : 0);
            DrawHistogram();
            CHECK(rectangle_count == 2 && ColorCount(COLOR_S2C) == 1);
            CHECK(rectangles[1].x == 99 && rectangles[1].h == 15);
        }
        Setup(); Set(sh_histogram_fill_mode, fill);
        const unsigned flags[] = { NETEVENT_JITTER, NETEVENT_SPIKE, NETEVENT_PRED, NETEVENT_LOSS };
        const int colors[] = { COLOR_JITTER, COLOR_SPIKE, COLOR_C2S, COLOR_S2C };
        SH_NetMeter_PushSample(cls.realtime - 100, 50, NETEVENT_CHOKE);
        for (size_t i = 0; i < q_countof(flags); i++) {
            SH_NetMeter_PushSample(cls.realtime - 90 + (unsigned)i * 2, 10, flags[i]);
            SH_NetMeter_PushSample(cls.realtime - 89 + (unsigned)i * 2, 50, 0);
            DrawHistogram();
            CHECK(rectangle_count == 2 && rectangles[1].color == colors[i]);
            CHECK(rectangles[1].h == 15);
        }
        /* Different timestamps still map to one column; newest equal priority supplies alpha. */
        Setup(); Set(sh_histogram_fill_mode, fill); Set(sh_histogram_bad_alpha, .6f);
        SH_NetMeter_PushSample(cls.realtime - 100, 10, NETEVENT_JITTER);
        SH_NetMeter_PushSample(cls.realtime - 50, 40, 0);
        SH_NetMeter_PushSample(cls.realtime - 25, 20, NETEVENT_JITTER);
        DrawHistogram();
        CHECK(rectangle_count == 2 && rectangles[1].color == COLOR_JITTER);
        CHECK(rectangles[1].h == 12);
        CHECK(fabsf(rectangles[1].alpha - .6f * (.35f + .65f * (52000 - 25) / 52000)) < .00001f);
        /* Dense even spacing covers a loss in the first of several aliased samples. */
        Setup(); Set(sh_histogram_fill_mode, fill); Set(sh_histogram_spacing_mode, 1); Set(sh_histogram_width, 10);
        for (int i = 0; i < 30; i++)
            SH_NetMeter_PushSample(cls.realtime - 30 + i, 50, i == 0 ? NETEVENT_LOSS : 0);
        DrawHistogram();
        CHECK(rectangle_count == 11 && rectangles[1].color == COLOR_S2C);
        CHECK(rectangles[1].x == 0 && rectangles[1].h == 15);
        for (int i = 1; i < rectangle_count; i++) {
            CHECK(rectangles[i].w == 1 && rectangles[i].x == i - 1);
        }
        /* Unique columns and all bar-width branches retain their original geometry. */
        const int gaps[] = { 1, 2, 3, 20 };
        for (int spacing = 0; spacing <= 1; spacing++) {
            for (size_t g = 0; g < q_countof(gaps); g++) {
                Setup(); Set(sh_histogram_fill_mode, fill); Set(sh_histogram_spacing_mode, spacing);
                Set(sh_histogram_width, gaps[g] * 10 + 1); Set(sh_histogram_history, 1000);
                for (int i = 0; i <= 10; i++)
                    SH_NetMeter_PushSample(cls.realtime - 1000 + i * 100 + 1, 25, i == 5 ? NETEVENT_LOSS : 0);
                cls.realtime++;
                DrawHistogram();
                CHECK(rectangle_count == 12);
                for (int i = 1; i < rectangle_count; i++) {
                    CHECK(rectangles[i].x == (i - 1) * gaps[g]);
                    CHECK(rectangles[i].h == (i == 6 ? 15 : 7));
                    int expected_w = i == 11 ? 1 : fill ? gaps[g] : min(max(gaps[g] / 2, 1), 8);
                    CHECK(rectangles[i].w == expected_w);
                    if (i > 1) CHECK(rectangles[i - 1].x + rectangles[i - 1].w <= rectangles[i].x);
                }
            }
        }
    }
    Setup(); DrawHistogram(); CHECK(rectangle_count == 1);
    SH_NetMeter_PushSample(cls.realtime, 25, 0);
    DrawHistogram(); CHECK(rectangle_count == 2 && rectangles[1].h == 7);
    cls.realtime += 52001;
    DrawHistogram(); CHECK(rectangle_count == 1);
    Setup(); Set(sh_histogram_history, 120000); cls.realtime = 10000;
    for (unsigned i = 0; i < NETMETER_SAMPLES + 100; i++)
        SH_NetMeter_PushSample(1000 + i, 50, i == 0 ? NETEVENT_LOSS : i == 101 ? NETEVENT_PRED : 0);
    unsigned head = netmeter.head;
    DrawHistogram();
    CHECK(ColorCount(COLOR_S2C) == 0 && ColorCount(COLOR_C2S) == 1);
    CHECK(netmeter.head == head);
}

static void CheckViewportPreservesSettings(void)
{
    Setup();
    Set(sh_histogram_width, 800); sh_histogram_width->string = "800";
    Set(sh_histogram_height, 600); sh_histogram_height->string = "600";
    Set(sh_netgraph_height, 600); sh_netgraph_height->string = "600";
    cl.frameflags = FF_SERVERDROP;
    SH_NetMeter_Sample(40); // A loss column occupies the full configured graph height.
    test_cvar_writes = 0;
    for (int large = 0; large <= 1; large++) {
        scr.hud_width = large ? 1280 : 640;
        scr.hud_height = large ? 960 : 480;
        ResetDrawing();
        SCR_DrawNetMeterHistogram(1, cls.realtime);
        CHECK(rectangles[0].w == (large ? 800 : 640));
        CHECK(rectangles[0].h == (large ? 600 : 480));
        ResetDrawing();
        SCR_DrawNetMeterNetgraph(1);
        CHECK(rectangles[0].h == (large ? 600 : 480));
        CHECK(sh_histogram_width->value == 800 && sh_histogram_width->integer == 800);
        CHECK(sh_histogram_height->value == 600 && sh_histogram_height->integer == 600);
        CHECK(sh_netgraph_height->value == 600 && sh_netgraph_height->integer == 600);
        CHECK(!strcmp(sh_histogram_width->string, "800"));
        CHECK(!strcmp(sh_histogram_height->string, "600"));
        CHECK(!strcmp(sh_netgraph_height->string, "600"));
        CHECK(!sh_histogram_width->modified && !sh_histogram_height->modified && !sh_netgraph_height->modified);
        CHECK(test_cvar_writes == 0);
    }
}

int main(int argc, char **argv)
{
    if (argc == 1 || !strcmp(argv[1], "viewport")) CheckViewportPreservesSettings();
    if (argc == 1 || !strcmp(argv[1], "loss")) CheckLoss();
    if (argc == 1 || !strcmp(argv[1], "jitter")) CheckJitter();
    if (argc == 1 || !strcmp(argv[1], "notices")) CheckNotices();
    if (argc == 1 || !strcmp(argv[1], "histogram")) CheckHistogram();
    printf("netmeter: %d checks, %d failures\n", checks, failures);
    return failures != 0;
}