/* Production layout registry/capture with renderer and cvar boundaries stubbed. */
#undef NDEBUG
#include "../src/client/hud_layout.c"
#include "../src/refresh/draw_group.h"
#include <assert.h>
#include <stdarg.h>
#include "fixtures/server_statusbar.h"

client_state_t cl;
scr_t scr;
refcfg_t r_config;
static draw_group_t group;
static cvar_t vars[HUD_LAYOUT_MAX * 4];
static char var_names[HUD_LAYOUT_MAX * 4][MAX_QPATH];
static char var_text[HUD_LAYOUT_MAX * 4][MAX_QPATH];
static char var_defaults[HUD_LAYOUT_MAX * 4][MAX_QPATH];
static int var_count;
static bool editor_active, preview, focus;
static int selected = -1;
static hud_preview_scenario_t scenario;
static unsigned sample_lines;
static const cvar_t *draft_var;
static float draft_value;
static vrect_t preview_bounds[HUD_EDIT_COUNT];

size_t Q_strlcpy(char *dst, const char *src, size_t size)
{
    size_t n = strlen(src);
    if (size) { size_t copy = min(n, size - 1); memcpy(dst, src, copy); dst[copy] = 0; }
    return n;
}

size_t Q_snprintf(char *dst, size_t size, const char *fmt, ...)
{
    va_list args; va_start(args, fmt); int n = vsnprintf(dst, size, fmt, args); va_end(args);
    return n < 0 ? 0 : (size_t)n;
}

cvar_t *Cvar_FindVar(const char *name)
{
    for (int i = 0; i < var_count; i++) if (!strcmp(vars[i].name, name)) return &vars[i];
    return NULL;
}

cvar_t *Cvar_Get(const char *name, const char *value, int flags)
{
    cvar_t *v = Cvar_FindVar(name);
    if (v) return v;
    assert(var_count < q_countof(vars));
    v = &vars[var_count];
    Q_strlcpy(var_names[var_count], name, MAX_QPATH);
    v->name = var_names[var_count];
    Q_strlcpy(var_text[var_count], value, MAX_QPATH);
    v->string = var_text[var_count];
    Q_strlcpy(var_defaults[var_count], value, MAX_QPATH);
    v->default_string = var_defaults[var_count++];
    v->value = strtof(value, NULL); v->integer = v->value; v->flags = flags;
    return v;
}

void Cvar_SetByVar(cvar_t *var, const char *value, from_t from)
{
    assert(from == FROM_CODE);
    Q_strlcpy(var->string, value, MAX_QPATH);
    var->value = strtof(value, NULL);
    var->integer = var->value;
}

bool HUD_EditorActive(void) { return editor_active; }
bool HUD_EditorPreview(void) { return preview; }
hud_preview_scenario_t HUD_EditorScenario(void)
{
    return scenario;
}

bool HUD_EditorSelected(int id)
{
    return id == selected;
}

bool HUD_EditorShow(int id) { return !preview || !focus || id == selected; }
float HUD_EditorValue(const cvar_t *v)
{
    return preview && v == draft_var ? draft_value : v->value;
}
float HUD_EditorClamp(cvar_t *v, float low, float high) { return Q_clip(v->value, low, high); }
void HUD_EditorBounds(hud_edit_id_t id, float x, float y, float w, float h)
{
    preview_bounds[id] = (vrect_t) { x, y, w, h };
}

void R_BeginDrawGroup(float x, float y, bool hidden)
{
    assert(!group.active);
    group = (draw_group_t) { .active = true, .hidden = hidden, .x = x, .y = y };
}

void R_SetDrawGroupScale(float sx, float sy, float x, float y)
{
    DrawGroup_SetScale(&group, sx, sy, x, y);
}

static vrect_t draw_bounds(float left, float top, float right, float bottom)
{
    int x = (int)floorf(left), y = (int)floorf(top);
    return (vrect_t) { x, y, (int)ceilf(right) - x, (int)ceilf(bottom) - y };
}

bool R_EndDrawGroupRaw(vrect_t *r, vrect_t *raw)
{
    bool have = group.have_bounds;
    if (have && r)
        *r = draw_bounds(group.left, group.top, group.right, group.bottom);
    if (have && raw)
        *raw = draw_bounds(group.raw_left, group.raw_top, group.raw_right, group.raw_bottom);
    group = (draw_group_t) { 0 };
    return have;
}

bool R_EndDrawGroup(vrect_t *r)
{
    return R_EndDrawGroupRaw(r, NULL);
}

void R_DrawFill32(int x, int y, int w, int h, uint32_t color)
{
    float left = x, top = y, width = w, height = h;
    DrawGroup_Rect(&group, scr.hud_scale, &left, &top, &width, &height);
}

void R_SetColor(uint32_t color) {}
void R_ClearColor(void) {}
int R_DrawString(int x, int y, int flags, size_t n, const char *text, qhandle_t font)
{
    if (!strncmp(text, "Time left: 12", 13))
        sample_lines |= 1;
    if (!strncmp(text, "Runner Three", 12))
        sample_lines |= 2;
    if (!strcmp(text, "SamplePlayer"))
        sample_lines |= 4;
    if (!strcmp(text, "PACKET LOSS"))
        sample_lines |= 8;
    if (!strcmp(text, "Run in progress"))
        sample_lines |= 16;
    for (; *text && n; text++, n--, x += CHAR_WIDTH) {
        if ((*text & 127) != ' ') {
            float left = x, top = y, width = CHAR_WIDTH, height = CHAR_HEIGHT;
            DrawGroup_Rect(&group, scr.hud_scale, &left, &top, &width, &height);
        }
    }
    return x;
}
/* The reminder fixture covers the shared panel renderer; this boundary checks
 * editor routing and placement of its fixed sample. */
static int reminder_previews;
void SCR_PreviewBindReminders(void)
{
    reminder_previews++;
    HUD_LayoutBegin(HL_BIND_REMINDERS);
    int width = Q_rint(r_config.width * scr.hud_scale);
    int height = Q_rint(r_config.height * scr.hud_scale);
    int x = max(0, width - 110 - 8);
    int y = min(Q_rint(height * .525f), max(0, height - 90 - 8));
    R_DrawFill32(x, y, 110, 90, 0);
    HUD_LayoutEnd();
}

static void rect(float x, float y, float w, float h)
{
    DrawGroup_Rect(&group, scr.hud_scale, &x, &y, &w, &h);
}
/* Preview bounds keep the full group extent so alignment cannot push it off-screen. */
static void test_preview_height(void)
{
    static const struct {
        int id, height;
    } cases[] = {
        { HL_TIMER, 32 },
        { HL_VOTE, 40 },
        { HL_MAPCOUNT, 16 },
        { HL_CHAT, 4 * CHAR_HEIGHT },
        { HL_NOTIFY, 32 },
        { HL_RENDER_FPS, CHAR_HEIGHT },
        { HL_STATUS1, CHAR_HEIGHT },
        { HL_TARGET, CHAR_HEIGHT },
        { HL_ITEM, 24 },
        { HL_INPUTS, 42 },
        { HL_INVENTORY, 240 },
        { HL_SCOREBOARD, 240 },
        { HL_BIND_REMINDERS, 90 },
    };
    const float scales[] = { 1, .5f, .25f };
    HUD_LayoutInit();
    int custom = HUD_LayoutObject("custom_text");
    editor_active = preview = true;
    for (size_t s = 0; s < q_countof(scales); s++) {
        scr.hud_scale = scales[s];
        for (int cached = 0; cached < 2; cached++) {
            for (size_t i = 0; i <= q_countof(cases); i++) {
                int id = i == q_countof(cases) ? custom : cases[i].id;
                int expected = i == q_countof(cases) ? CHAR_HEIGHT : cases[i].height;
                selected = HUD_EDIT_LAYOUT_FIRST + id;
                memset(last_bounds, 0, sizeof(last_bounds));
                memset(preview_bounds, 0, sizeof(preview_bounds));
                items[id].x->value = 10;
                items[id].y->value = 20;
                /* A live capture taller than the sample text still defines the bounds. */
                if (cached && id != HL_BIND_REMINDERS) {
                    expected = 34;
                    HUD_LayoutSetBounds(id, (vrect_t) { 40, 50, 160, expected });
                }
                HUD_LayoutFrame();
                HUD_LayoutPreview();
                assert(preview_bounds[selected].height == expected);
                if (cached && id != HL_BIND_REMINDERS) {
                    assert(preview_bounds[selected].x == 50 && preview_bounds[selected].y == 70);
                    assert(last_bounds[id].height == expected);
                }
            }
        }
    }
    editor_active = preview = false;
}


static void test_workbench_bounds(void)
{
    HUD_LayoutInit();
    memset(last_bounds, 0, sizeof(last_bounds));
    memset(preview_bounds, 0, sizeof(preview_bounds));
    selected = HUD_EDIT_UPS;
    editor_active = preview = focus = true;
    items[HL_CHAT].visible->value = 0;
    HUD_LayoutPreview();
    /* Hidden, unselected and uncached elements remain alignment references. */
    assert(preview_bounds[HUD_EDIT_LAYOUT_FIRST + HL_CHAT].width > 0);
    assert(preview_bounds[HUD_EDIT_LAYOUT_FIRST + HL_TIMER].width > 0);
    assert(preview_bounds[HUD_EDIT_LAYOUT_FIRST + HL_CROSSHAIR].width == 0);
    assert(items[HL_CHAT].visible->value == 0 && !last_bounds[HL_CHAT].width);
    editor_active = preview = focus = false;
}

static void test_transient_preview(void)
{
    HUD_LayoutInit();
    HUD_LayoutFrame();
    assert(HUD_LayoutPreviewAvailable(HL_TIMER));
    assert(!HUD_LayoutPreviewAvailable(HL_INVENTORY));
    assert(!HUD_LayoutPreviewAvailable(HL_SCOREBOARD));
    assert(!HUD_LayoutPreviewAvailable(HL_NETALERT));
    assert(!HUD_LayoutPreviewAvailable(-1));
    assert(!HUD_LayoutPreviewAvailable(HUD_LAYOUT_MAX));
    editor_active = true;
    preview = false;
    HUD_LayoutBegin(HL_INVENTORY);
    rect(100, 100, 240, 200);
    HUD_LayoutEnd();
    assert(HUD_LayoutPreviewAvailable(HL_INVENTORY));
    HUD_LayoutFrame();
    assert(!HUD_LayoutPreviewAvailable(HL_INVENTORY));
    /* Measuring an editor sample must not mark its live group active. */
    preview = true;
    HUD_LayoutPreview();
    assert(!HUD_LayoutPreviewAvailable(HL_INVENTORY));
    assert(preview_bounds[HUD_EDIT_LAYOUT_FIRST + HL_INVENTORY].width > 0);
    editor_active = preview = false;
}

static void test_server_scope(void)
{
    HUD_LayoutInit();
    scr.hud_scale = 1;
    r_config.width = 1280;
    r_config.height = 720;
    editor_active = preview = focus = false;
    int custom = HUD_LayoutObject("scope_custom");
    assert(custom >= HL_BUILTIN_COUNT);

    /* The generic server group still renders with its existing saved offsets. */
    items[HL_OTHER_STATUS].x->value = 17;
    items[HL_OTHER_STATUS].y->value = 29;
    items[HL_OTHER_STATUS].visible->value = 1;
    HUD_LayoutFrame();
    HUD_LayoutBegin(HL_OTHER_STATUS);
    assert(!group.hidden && group.x == 17 && group.y == 29);
    rect(100, 120, 80, 16);
    HUD_LayoutEnd();
    assert(last_bounds[HL_OTHER_STATUS].width > 0);

    /* Even captured generic content cannot enter the editor preview. */
    memset(preview_bounds, 0, sizeof(preview_bounds));
    editor_active = preview = true;
    selected = HUD_EDIT_UPS;
    HUD_LayoutPreview();
    assert(!preview_bounds[HUD_EDIT_LAYOUT_FIRST + HL_OTHER_STATUS].width);
    for (int id = 0; id < HL_SERVER_COUNT; id++)
        assert(preview_bounds[HUD_EDIT_LAYOUT_FIRST + id].width > 0);
    assert(preview_bounds[HUD_EDIT_LAYOUT_FIRST + HL_SCOREBOARD].width > 0);
    assert(preview_bounds[HUD_EDIT_LAYOUT_FIRST + HL_CHAT].width > 0);
    assert(preview_bounds[HUD_EDIT_LAYOUT_FIRST + custom].width > 0);
    assert(items[HL_OTHER_STATUS].x->value == 17 && items[HL_OTHER_STATUS].y->value == 29);
    assert(items[HL_OTHER_STATUS].visible->value == 1);
    editor_active = preview = false;
}

static void test_ping_preview_geometry(void)
{
    HUD_LayoutInit();
    cvar_t *x = Cvar_Get("sh_lagometer_x", "200", 0);
    cvar_t *y = Cvar_Get("sh_lagometer_y", "100", 0);
    items[HL_NETICON].x->value = 7;
    items[HL_NETICON].y->value = -3;
    const float scales[] = { 1, .5f };
    for (int i = 0; i < q_countof(scales); i++) {
        scr.hud_scale = scales[i];
        r_config.width = 1280;
        r_config.height = 720;
        int width = Q_rint(r_config.width * scr.hud_scale), height = Q_rint(r_config.height * scr.hud_scale);
        editor_active = preview = false;
        HUD_LayoutFrame();
        assert(!HUD_LayoutCaptured(HL_NETICON));
        /* A cached graph at a different origin must not override changed settings. */
        HUD_LayoutBegin(HL_NETICON);
        rect(50, 60, 20, 10);
        HUD_LayoutEnd();
        assert(HUD_LayoutCaptured(HL_NETICON));
        editor_active = preview = true;
        for (int negative = 0; negative < 2; negative++) {
            x->value = negative ? -1 : 200;
            y->value = negative ? -11 : 100;
            memset(preview_bounds, 0, sizeof(preview_bounds));
            HUD_LayoutPreview();
            vrect_t b = preview_bounds[HUD_EDIT_LAYOUT_FIRST + HL_NETICON];
            assert(b.x == (negative ? width - 48 : 200) + 7);
            assert(b.y == (negative ? height - 58 : 100) - 3);
            assert(b.width == 48 && b.height == 48);
        }
        /* The legacy ping graph preserves off-screen positions, unlike the netmeter. */
        x->value = 4000;
        y->value = -4000;
        memset(preview_bounds, 0, sizeof(preview_bounds));
        HUD_LayoutPreview();
        vrect_t offscreen = preview_bounds[HUD_EDIT_LAYOUT_FIRST + HL_NETICON];
        assert(offscreen.x == 4000 + 7 && offscreen.y == height - 4000 - 48 + 1 - 3);
        x->value = -4000;
        y->value = 4000;
        memset(preview_bounds, 0, sizeof(preview_bounds));
        HUD_LayoutPreview();
        offscreen = preview_bounds[HUD_EDIT_LAYOUT_FIRST + HL_NETICON];
        assert(offscreen.x == width - 4000 - 48 + 1 + 7 && offscreen.y == 4000 - 3);
        HUD_LayoutFrame();
        assert(!HUD_LayoutCaptured(HL_NETICON));
    }
    editor_active = preview = false;
}

/* Sample dimensions follow the draft, never the previous frame's graph bars. */
static void test_debuggraph_draft_geometry(void)
{
    HUD_LayoutInit();
    cvar_t *height_var = Cvar_Get("sh_netgraph_height", "15", 0);
    cvar_t *mode_var = Cvar_Get("scr_netgraph", "0", 0);
    items[HL_DEBUGGRAPH].x->value = 7;
    items[HL_DEBUGGRAPH].y->value = -3;
    const float scales[] = { 1, .5f, .25f };
    const int heights[] = { 1, 40, 200, 800 };
    draft_var = height_var;
    selected = HUD_EDIT_LAYOUT_FIRST + HL_DEBUGGRAPH;
    editor_active = preview = true;
    for (size_t i = 0; i < q_countof(scales); i++) {
        scr.hud_scale = scales[i];
        r_config.width = 1280;
        r_config.height = 720;
        int width = Q_rint(r_config.width * scr.hud_scale);
        int height = Q_rint(r_config.height * scr.hud_scale);
        for (int cached = 0; cached < 2; cached++) {
            HUD_LayoutSetBounds(HL_DEBUGGRAPH, cached ?
                (vrect_t) { 40, 50, 160, 8 } : (vrect_t) { 0 });
            vrect_t original_bounds = last_bounds[HL_DEBUGGRAPH];
            for (int detailed = 0; detailed < 2; detailed++) {
                mode_var->value = detailed ? 2 : 0;
                for (size_t h = 0; h < q_countof(heights); h++) {
                    draft_value = heights[h];
                    int expected = detailed ? Q_clip(heights[h], 10, 200) : heights[h];
                    memset(preview_bounds, 0, sizeof(preview_bounds));
                    HUD_LayoutFrame();
                    HUD_LayoutPreview();
                    vrect_t bounds = preview_bounds[selected];
                    assert(bounds.x == 7 && bounds.y == height - expected - 3);
                    assert(bounds.width == width && bounds.height == expected);
                    assert(height_var->value == 15 && height_var->integer == 15);
                    assert(!memcmp(&original_bounds, &last_bounds[HL_DEBUGGRAPH], sizeof(original_bounds)));
                    assert(!HUD_LayoutCaptured(HL_DEBUGGRAPH));
                }
            }
        }
    }
    mode_var->value = 0;
    draft_var = NULL;
    editor_active = preview = false;
}

static void test_scenario_samples(void)
{
    static client_state_t saved_cl;
    memcpy(&saved_cl, &cl, sizeof(cl));
    bool saved_captured[HUD_LAYOUT_MAX];
    vrect_t saved_bounds[HUD_LAYOUT_MAX];
    float saved_values[q_countof(vars)];
    memcpy(saved_captured, captured, sizeof(captured));
    memcpy(saved_bounds, last_bounds, sizeof(last_bounds));
    for (int i = 0; i < var_count; i++)
        saved_values[i] = vars[i].value;
    editor_active = preview = true;
    focus = false;
    selected = -1;
    sample_lines = 0;
    for (scenario = HUD_PREVIEW_RUNNING; scenario < HUD_PREVIEW_COUNT; scenario++) {
        memset(preview_bounds, 0, sizeof(preview_bounds));
        HUD_LayoutPreview();
        for (int i = 0; i < HL_BUILTIN_COUNT; i++)
            if (!HUD_LayoutEditable(i))
                assert(preview_bounds[HUD_EDIT_LAYOUT_FIRST + i].width == 0);
        if (scenario == HUD_PREVIEW_VOTING)
            assert(preview_bounds[HUD_EDIT_LAYOUT_FIRST + HL_VOTE].height >= 4 * CHAR_HEIGHT);
        if (scenario == HUD_PREVIEW_SCOREBOARD)
            assert(preview_bounds[HUD_EDIT_LAYOUT_FIRST + HL_SCOREBOARD].height >= 6 * CHAR_HEIGHT);
    }
    assert(sample_lines == 31);
    assert(!memcmp(saved_captured, captured, sizeof(captured)));
    assert(!memcmp(saved_bounds, last_bounds, sizeof(last_bounds)));
    assert(!memcmp(&saved_cl, &cl, sizeof(cl)));
    for (int i = 0; i < var_count; i++)
        assert(vars[i].value == saved_values[i] || (isnan(vars[i].value) && isnan(saved_values[i])));
    scenario = HUD_PREVIEW_LIVE;
    memset(last_bounds, 0, sizeof(last_bounds));
    HUD_LayoutPreview();
    assert(preview_bounds[HUD_EDIT_LAYOUT_FIRST + HL_VOTE].height == 40);
    editor_active = preview = false;
}

static void test_reminder_cvar_migration(void)
{
    Cvar_Get("hud_bind_reminders_visible", "0", CVAR_ARCHIVE);
    Cvar_Get("hud_bind_reminders_x", "24", CVAR_ARCHIVE);
    Cvar_Get("hud_bind_reminders_y", "-12", CVAR_ARCHIVE);
    Cvar_Get("hud_bind_reminders_scale", "1.5", CVAR_ARCHIVE);
    HUD_LayoutInit();
    const hud_layout_item_t *item = HUD_LayoutItem(HL_BIND_REMINDERS);
    assert(!strcmp(item->visible->name, "scr_bindreminders_visible"));
    assert(!strcmp(item->x->name, "scr_bindreminders_x"));
    assert(!strcmp(item->y->name, "scr_bindreminders_y"));
    assert(!strcmp(item->scale->name, "scr_bindreminders_scale"));
    assert(item->scale->value == 1.5f && !strcmp(item->scale->default_string, "1"));
    assert(item->visible->integer == 0 && item->x->value == 24 && item->y->value == -12);
    assert(!strcmp(item->visible->default_string, "1"));
    assert(!strcmp(item->x->default_string, "0") && !strcmp(item->y->default_string, "0"));
    Cvar_Reset(item->visible);
    Cvar_Reset(item->x);
    Cvar_Reset(item->y);
    Cvar_Reset(item->scale);
    HUD_LayoutInit();
    assert(item->scale->value == 1);
    assert(item->visible->integer == 1 && item->x->value == 0 && item->y->value == 0);
    assert(Cvar_FindVar("hud_bind_reminders_x")->value == 24);

    var_count = 0;
    Cvar_Get("hud_bind_reminders_visible", "1", CVAR_ARCHIVE);
    Cvar_Get("hud_bind_reminders_x", "24", CVAR_ARCHIVE);
    Cvar_Get("hud_bind_reminders_y", "-12", CVAR_ARCHIVE);
    Cvar_Get("scr_bindreminders_visible", "0", CVAR_ARCHIVE);
    Cvar_Get("scr_bindreminders_x", "0", CVAR_ARCHIVE);
    Cvar_Get("scr_bindreminders_y", "13", CVAR_ARCHIVE);
    Cvar_Get("hud_bind_reminders_scale", "3", CVAR_ARCHIVE);
    Cvar_Get("scr_bindreminders_scale", "1.25", CVAR_ARCHIVE);
    HUD_LayoutInit();
    assert(item->scale->value == 1.25f);
    assert(item->visible->integer == 0 && item->x->value == 0 && item->y->value == 13);
    assert(!strcmp(items[HL_TIMER].x->name, "hud_timer_x"));
    var_count = 0;
}

/* Compare visible extents in HUD units; outward physical-pixel rounding can
 * expand a fractional edge by one HUD unit. */
static void expect_scaled_bounds(vrect_t bounds, float x, float y, float w, float h)
{
    if (abs(bounds.x - Q_rint(x)) > 1 || abs(bounds.y - Q_rint(y)) > 1 ||
        abs(bounds.width - Q_rint(w)) > 1 || abs(bounds.height - Q_rint(h)) > 1)
        fprintf(stderr, "HUD bounds %d,%d %dx%d; expected %.2f,%.2f %.2fx%.2f\n",
                bounds.x, bounds.y, bounds.width, bounds.height, x, y, w, h);
    assert(abs(bounds.x - Q_rint(x)) <= 1);
    assert(abs(bounds.y - Q_rint(y)) <= 1);
    assert(abs(bounds.width - Q_rint(w)) <= 1);
    assert(abs(bounds.height - Q_rint(h)) <= 1);
}

static void test_item_scaling(void)
{
    var_count = 0;
    draft_var = NULL;
    focus = editor_active = preview = false;
    r_config.width = 1280;
    r_config.height = 720;
    HUD_LayoutInit();
    for (int i = 0; i < HUD_LayoutCount(); i++) {
        assert(items[i].scale->value == 1);
        assert(items[i].scale->flags & CVAR_ARCHIVE);
    }
    assert(!strcmp(items[HL_TIMER].scale->name, "hud_timer_scale"));
    const float hud_scales[] = { 1, .5f, .25f };
    const float item_scales[] = { .25f, .5f, 1, 1.5f, 4 };
    for (unsigned h = 0; h < q_countof(hud_scales); h++) {
        scr.hud_scale = hud_scales[h];
        int width = Q_rint(r_config.width * scr.hud_scale);
        int height = Q_rint(r_config.height * scr.hud_scale);
        for (unsigned k = 0; k < q_countof(item_scales); k++) {
            float scale = item_scales[k], x, y;
            items[HL_TIMER].scale->value = scale;
            items[HL_TIMER].x->value = 13;
            items[HL_TIMER].y->value = 7;
            editor_active = preview = false;
            HUD_LayoutFrame();
            HUD_LayoutScaleAnchor(HL_TIMER, &x, &y);
            assert(x == width && y == height);
            /* The number and label arrive in separate token groups. */
            HUD_LayoutBegin(HL_TIMER);
            rect(width - 94, height - 32, 64, 24);
            HUD_LayoutEnd();
            HUD_LayoutBegin(HL_TIMER);
            rect(width - 24, height - 16, 24, 16);
            HUD_LayoutEnd();
            vrect_t raw = last_bounds[HL_TIMER];
            assert(raw.x == width - 94 && raw.y == height - 32);
            assert(raw.width == 94 && raw.height == 32);
            editor_active = preview = true;
            HUD_LayoutPreview();
            vrect_t first = preview_bounds[HUD_EDIT_LAYOUT_FIRST + HL_TIMER];
            expect_scaled_bounds(first, width - 94 * scale + 13, height - 32 * scale + 7,
                                 94 * scale, 32 * scale);
            HUD_LayoutPreview();
            assert(!memcmp(&first, &preview_bounds[HUD_EDIT_LAYOUT_FIRST + HL_TIMER], sizeof(first)));
            assert(!memcmp(&raw, &last_bounds[HL_TIMER], sizeof(raw)));
        }
    }
    scr.hud_scale = .5f;
    float x, y;
    HUD_LayoutScaleAnchor(HL_HEALTH, &x, &y);
    assert(x == 320 && y == 360);
    HUD_LayoutScaleAnchor(HL_RENDER_FPS, &x, &y);
    assert(x == 640 && y == 0);
    r_config.width = 1920;
    r_config.height = 1080;
    HUD_LayoutScaleAnchor(HL_HEALTH, &x, &y);
    assert(x == 480 && y == 540);

    /* Draft preview scale cannot write through to gameplay. */
    items[HL_TIMER].scale->value = 2;
    draft_var = items[HL_TIMER].scale;
    draft_value = .5f;
    preview = true;
    HUD_LayoutBegin(HL_TIMER);
    assert(group.scale_x == .5f && group.scale_y == .5f);
    HUD_LayoutEnd();
    preview = false;
    HUD_LayoutBegin(HL_TIMER);
    assert(group.scale_x == 2 && items[HL_TIMER].scale->value == 2);
    HUD_LayoutEnd();
    draft_var = NULL;

    const float invalid[] = { NAN, INFINITY, -INFINITY, -5, 0, 100 };
    const float expected[] = { 1, 1, 1, HUD_SCALE_MIN, HUD_SCALE_MIN, HUD_SCALE_MAX };
    for (unsigned i = 0; i < q_countof(invalid); i++) {
        items[HL_TIMER].scale->value = invalid[i];
        HUD_LayoutBegin(HL_TIMER);
        assert(group.scale_x == expected[i] && group.scale_y == expected[i]);
        HUD_LayoutEnd();
    }
    items[HL_CROSSHAIR].scale->value = 4;
    HUD_LayoutBegin(HL_CROSSHAIR);
    rect(100, 100, 16, 16);
    assert(group.right - group.left == 16 / scr.hud_scale);
    HUD_LayoutEnd();
    items[HL_DEBUGGRAPH].scale->value = 2;
    HUD_LayoutBegin(HL_DEBUGGRAPH);
    assert(group.scale_x == 1 && group.scale_y == 2);
    HUD_LayoutEnd();

    /* Right-aligned dynamic strings retain their text anchor as digits change. */
    editor_active = preview = false;
    int id = HUD_LayoutObject("scale_test_dynamic");
    assert(items[id].scale->value == 1);
    items[id].scale->value = 1.5f;
    items[id].x->value = 5;
    HUD_LayoutSetScaleAnchor(id, 952, 30);
    HUD_LayoutScaleAnchor(id, &x, &y);
    assert(x == 952 && y == 30);
    for (int width = 24; width <= 64; width += 40) {
        HUD_LayoutFrame();
        HUD_LayoutBegin(id);
        rect(952 - width, 30, width, 8);
        assert(fabsf(group.right * scr.hud_scale - 957) < .001f);
        assert(fabsf((group.right - group.left) * scr.hud_scale - width * 1.5f) < .001f);
        HUD_LayoutEnd();
        assert(last_bounds[id].x == 952 - width && last_bounds[id].width == width);
        editor_active = preview = true;
        HUD_LayoutPreview();
        expect_scaled_bounds(preview_bounds[HUD_EDIT_LAYOUT_FIRST + id],
                             957 - width * 1.5f, 30, width * 1.5f, 12);
        editor_active = preview = false;
    }
    /* The caller resolves signed screen coordinates again after a resize. */
    r_config.width = 1280;
    HUD_LayoutSetScaleAnchor(id, 632, 30);
    HUD_LayoutScaleAnchor(id, &x, &y);
    assert(x == 632 && y == 30);
    HUD_LayoutSetScaleAnchor(id, NAN, INFINITY);
    HUD_LayoutScaleAnchor(id, &x, &y);
    assert(x == 0 && y == 0);
}

int main(void)
{
    test_reminder_cvar_migration();
    scr.hud_scale = .5f; r_config.width = 1280; r_config.height = 720;
    HUD_LayoutInit(); HUD_LayoutFrame();
    assert(HUD_LayoutCount() == HL_BUILTIN_COUNT);
    assert(!items[HL_RENDER_FPS].visible->value && items[HL_TIMER].visible->value);
    assert(items[HL_BIND_REMINDERS].visible->value);
    /* Existing archived visibility and offsets remain user-controlled. */
    items[HL_BIND_REMINDERS].visible->value = 0;
    items[HL_BIND_REMINDERS].x->value = 24;
    items[HL_BIND_REMINDERS].y->value = -12;
    HUD_LayoutInit();
    assert(!items[HL_BIND_REMINDERS].visible->value);
    assert(items[HL_BIND_REMINDERS].x->value == 24 && items[HL_BIND_REMINDERS].y->value == -12);
    items[HL_BIND_REMINDERS].visible->value = 1;
    items[HL_BIND_REMINDERS].x->value = items[HL_BIND_REMINDERS].y->value = 0;
    assert(!strcmp(items[HL_BIND_REMINDERS].key, "bind_reminders"));
    assert(!strcmp(items[HL_BIND_REMINDERS].name, "Bind reminders"));
    editor_active = preview = true;
    HUD_LayoutPreview();
    assert(reminder_previews == 1);
    assert(preview_bounds[HUD_EDIT_LAYOUT_FIRST + HL_BIND_REMINDERS].width == 110);
    assert(preview_bounds[HUD_EDIT_LAYOUT_FIRST + HL_BIND_REMINDERS].height == 90);
    assert(preview_bounds[HUD_EDIT_LAYOUT_FIRST + HL_BIND_REMINDERS].x == 522);
    assert(preview_bounds[HUD_EDIT_LAYOUT_FIRST + HL_BIND_REMINDERS].y == 189);
    HUD_LayoutFrame();
    editor_active = preview = false;
    items[HL_TIMER].x->value = 10; items[HL_TIMER].y->value = 20;
    HUD_LayoutBegin(HL_TIMER);
    assert(group.x == 20 && group.y == 40 && !group.hidden);
    rect(500, 300, 64, 24); HUD_LayoutEnd();
    assert(!group.active && current == -1);
    assert(last_bounds[HL_TIMER].x == 500 && last_bounds[HL_TIMER].y == 300);
    HUD_LayoutBegin(HL_TIMER); rect(570, 324, 24, 8); HUD_LayoutEnd();
    assert(last_bounds[HL_TIMER].width == 94 && last_bounds[HL_TIMER].height == 32);
    HUD_LayoutBegin(HL_SERVER_FPS); assert(group.x == 0 && group.y == 0); HUD_LayoutEnd();
    HUD_LayoutFrame(); HUD_LayoutBegin(HL_TIMER); rect(550, 300, 40, 24); HUD_LayoutEnd();
    assert(last_bounds[HL_TIMER].x == 550 && last_bounds[HL_TIMER].width == 40);
    editor_active = true; HUD_LayoutBegin(HL_TIMER); assert(group.hidden); HUD_LayoutEnd();
    preview = true; HUD_LayoutBegin(HL_TIMER); assert(!group.hidden); HUD_LayoutEnd();
    focus = true; selected = HUD_EDIT_LAYOUT_FIRST + HL_TIMER;
    HUD_LayoutBegin(HL_SERVER_FPS); assert(group.hidden);
    rect(12, 34, 56, 8); HUD_LayoutEnd();
    assert(preview_bounds[HUD_EDIT_LAYOUT_FIRST + HL_SERVER_FPS].x == 12);
    assert(preview_bounds[HUD_EDIT_LAYOUT_FIRST + HL_SERVER_FPS].width == 56);
    assert(items[HL_SERVER_FPS].visible->value == 1);
    HUD_LayoutBegin(HL_TIMER); assert(!group.hidden);
    rect(550, 300, 40, 24); HUD_LayoutEnd();
    assert(preview_bounds[selected].x == 560 && preview_bounds[selected].y == 320);
    assert(last_bounds[HL_TIMER].x == 550 && last_bounds[HL_TIMER].y == 300);
    preview = editor_active = false;
    HUD_LayoutBegin(HL_SERVER_FPS); assert(!group.hidden); HUD_LayoutEnd();
    focus = false;
    items[HL_TIMER].visible->value = 0;
    HUD_LayoutBegin(HL_TIMER); assert(group.hidden); HUD_LayoutFrame(); assert(!group.active);
    items[HL_TIMER].x->value = NAN;
    HUD_LayoutBegin(HL_TIMER); assert(group.x == 0); HUD_LayoutEnd();
    int a = HUD_LayoutObject("r_fps"); char saved[MAX_QPATH];
    Q_strlcpy(saved, items[a].x->name, sizeof(saved));
    HUD_LayoutInit(); HUD_LayoutObject("cl_fps"); a = HUD_LayoutObject("r_fps");
    assert(!strcmp(saved, items[a].x->name));
    editor_active = true; assert(HUD_LayoutObject("new_during_edit") == -1); editor_active = false;
    assert(HUD_LayoutMatch(server_statusbar_fixture));
    assert(!HUD_LayoutMatch("xl 0 yt 0 string other"));
    assert(HUD_LayoutMatch(server_statusbar_fixture));
    const char *timer = strstr(server_statusbar_fixture, "num 4 17");
    assert(HUD_LayoutToken(server_statusbar_fixture, timer) == HL_TIMER);
    test_preview_height();
    test_workbench_bounds();
    test_transient_preview();
    test_server_scope();
    test_ping_preview_geometry();
    test_debuggraph_draft_geometry();
    test_scenario_samples();
    test_item_scaling();
    puts("HUD registry, item scaling, anchors, raw capture, visibility, cache and preview bounds passed");
    return 0;
}
