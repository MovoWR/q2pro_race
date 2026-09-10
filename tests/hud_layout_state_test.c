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
static cvar_t vars[HUD_LAYOUT_MAX * 3];
static char var_names[HUD_LAYOUT_MAX * 3][MAX_QPATH];
static int var_count;
static bool editor_active, preview, focus;
static int selected = -1;
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
    Q_strlcpy(var_names[var_count], name, MAX_QPATH); v->name = var_names[var_count++];
    v->value = strtof(value, NULL); v->integer = v->value; v->flags = flags;
    return v;
}
bool HUD_EditorActive(void) { return editor_active; }
bool HUD_EditorPreview(void) { return preview; }
bool HUD_EditorSelected(int id) { return false; }
bool HUD_EditorShow(int id) { return !preview || !focus || id == selected; }
float HUD_EditorValue(const cvar_t *v) { return v->value; }
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
bool R_EndDrawGroup(vrect_t *r)
{
    bool have = group.have_bounds;
    if (have && r) *r = (vrect_t) { group.left, group.top, group.right - group.left, group.bottom - group.top };
    group = (draw_group_t) { 0 }; return have;
}
void R_DrawFill32(int x, int y, int w, int h, uint32_t color) {}
void R_SetColor(uint32_t color) {}
void R_ClearColor(void) {}
int R_DrawString(int x, int y, int flags, size_t n, const char *text, qhandle_t font) { return x; }
static void rect(float x, float y, float w, float h)
{
    DrawGroup_Rect(&group, scr.hud_scale, &x, &y, w, h);
}
int main(void)
{
    scr.hud_scale = .5f; r_config.width = 1280; r_config.height = 720;
    HUD_LayoutInit(); HUD_LayoutFrame();
    assert(HUD_LayoutCount() == HL_BUILTIN_COUNT);
    assert(!items[HL_RENDER_FPS].visible->value && items[HL_TIMER].visible->value);
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
    puts("HUD registry, scoped offsets, multi-fragment bounds, visibility and cache passed");
    return 0;
}
