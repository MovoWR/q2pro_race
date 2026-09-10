/* Saved client-side placement; server stats and layout text stay authoritative. */
#include "client.h"
#include "client/hud_editor.h"
#include "client/hud_layout.h"
#include "hud_layout_match.h"
#include "hud_layout_server.h"

static hud_layout_item_t items[HUD_LAYOUT_MAX];
static char object_names[HUD_LAYOUT_MAX][MAX_QPATH];
static char object_keys[HUD_LAYOUT_MAX][32];
static int item_count, current = -1, bounds_servercount;
static vrect_t last_bounds[HUD_LAYOUT_MAX];
static int last_width[HUD_LAYOUT_MAX], last_height[HUD_LAYOUT_MAX];
static float current_x, current_y;
static bool captured[HUD_LAYOUT_MAX];
static char matched_layout[HUD_LAYOUT_TEXT_MAX];
static hud_layout_token_t matched_tokens[HUD_LAYOUT_TOKEN_MAX];
static int matched_count;

static const struct {
    const char *key, *name, *sample;
    float ax, ay;
    int x, y, width, height;
} defaults[HL_BUILTIN_COUNT] = {
    { "health", "Health", "100 Health", .5f, 1, 150, -32, 54, 32 },
    { "item", "Weapon icon", "Gun", .5f, 1, 120, -32, 24, 24 },
    { "speed", "Speed (server)", "742 Speed", .5f, 1, 40, -32, 66, 32 },
    { "target", "Target player name", "Player", .5f, 1, -48, -58, 96, 8 },
    { "timer", "Run timer", "12.3 Time", 1, 1, -94, -32, 94, 32 },
    { "inputs", "Input keys", "W A S D Jump", 0, 1, 2, -42, 64, 42 },
    { "server_fps", "Reported player FPS", "120 FPS", 0, 1, 0, -76, 78, 32 },
    { "vote", "Vote information", "Vote: next map", 0, 1, 2, -136, 200, 40 },
    { "mapcount", "Available maps", "42 Maps", 1, 0, -32, 42, 32, 16 },
    { "status1", "Team / mode", "Team: Hard", .5f, 1, -88, -32, 160, 8 },
    { "status2", "Run info line 1", "Run info line 1", .5f, 1, -88, -24, 160, 8 },
    { "status3", "Run info line 2", "Run info line 2", .5f, 1, -88, -16, 160, 8 },
    { "status4", "Run info line 3", "Run info line 3", .5f, 1, -88, -8, 160, 8 },
    { "map", "Current map", "Current map", 1, 0, -128, 2, 128, 8 },
    { "prevmap1", "Previous map 1", "Previous map 1", 1, 0, -128, 10, 128, 8 },
    { "prevmap2", "Previous map 2", "Previous map 2", 1, 0, -128, 18, 128, 8 },
    { "prevmap3", "Previous map 3", "Previous map 3", 1, 0, -128, 26, 128, 8 },
    { "addedtime", "Map time adjustment", "+5", 1, 0, -32, 100, 32, 8 },
    { "timeleft", "Map time remaining", "Time 15", 1, 0, -50, 64, 50, 34 },
    { "crosshair", "Crosshair", "+", .5f, .5f, -8, -8, 16, 16 },
    { "hitmarker", "Hit marker", "X", .5f, .5f, -12, -12, 24, 24 },
    { "chat", "Chat history", "Player: nice jump!", 0, 1, 8, -174, 224, 32 },
    { "center", "Center messages", "Checkpoint reached", .5f, .25f, -80, -4, 160, 8 },
    { "notify", "Console messages", "Connected to server", 0, 0, 8, 0, 240, 32 },
    { "message", "Chat input", "say: message...", 0, 0, 8, 32, 240, 8 },
    { "netalert", "Network alerts", "PACKET LOSS", .5f, .5f, -44, -4, 88, 8 },
    { "neticon", "Ping graph / connection icon", "Ping", 0, 0, 64, 0, 32, 32 },
    { "turtle", "Frame / prediction warnings", "PRED", 0, 0, 0, 0, 32, 32 },
    { "pause", "Pause indicator", "PAUSED", .5f, .5f, -48, -12, 96, 24 },
    { "inventory", "Inventory", "Inventory", .5f, .5f, -128, -120, 256, 240 },
    { "scoreboard", "Scoreboard / server menus", "Scoreboard / server menus", .5f, .5f, -160, -120, 320, 240 },
    { "other_status", "Other server HUD", "Unrecognized server HUD", .5f, 1, -160, -48, 320, 48 },
    { "demo", "Demo progress", "Demo progress 50%", 0, 1, 0, -8, 320, 8 },
    { "loading", "Loading indicator", "Loading", .5f, .5f, -48, -12, 96, 24 },
    { "debuggraph", "Classic network / debug graph", "Network / debug graph", 0, 1, 0, -32, 320, 32 },
    { "nerdstats", "Nerd stats panel", "Physics and frame timings", 0, 0, 0, 0, 240, 240 },
    { "render_fps", "Render FPS", "Render: 120 FPS", 1, 0, -128, 40, 120, 8 },
    { "move_fps", "Movement rate (MPS)", "Moves: 120 MPS", 1, 0, -128, 50, 120, 8 },
    { "debug_stats", "Debug frame stats", "Frame / network statistics", 0, 0, 0, 100, 200, 96 },
    { "debug_pmove", "Debug movement", "Movement state", 0, 0, 0, 200, 200, 96 },
};

static void HUD_RegisterItem(int id, const char *key, const char *name, bool visible)
{
    char buffer[MAX_QPATH];
    items[id].key = key;
    items[id].name = name;
    Q_snprintf(buffer, sizeof(buffer), "hud_%s_x", key);
    items[id].x = Cvar_Get(buffer, "0", CVAR_ARCHIVE);
    Q_snprintf(buffer, sizeof(buffer), "hud_%s_y", key);
    items[id].y = Cvar_Get(buffer, "0", CVAR_ARCHIVE);
    Q_snprintf(buffer, sizeof(buffer), "hud_%s_visible", key);
    items[id].visible = Cvar_Get(buffer, visible ? "1" : "0", CVAR_ARCHIVE);
}

void HUD_LayoutInit(void)
{
    current = -1;
    item_count = HL_BUILTIN_COUNT;
    matched_layout[0] = 0;
    matched_count = 0;
    memset(last_bounds, 0, sizeof(last_bounds));
    for (int i = 0; i < item_count; i++)
        HUD_RegisterItem(i, defaults[i].key, defaults[i].name,
                         i != HL_RENDER_FPS && i != HL_MOVE_FPS);
}

void HUD_LayoutFrame(void)
{
    R_EndDrawGroup(NULL);
    current = -1;
    memset(captured, 0, sizeof(captured));
}

int HUD_LayoutCount(void)
{
    return item_count;
}

const hud_layout_item_t *HUD_LayoutItem(int id)
{
    return id >= 0 && id < item_count ? &items[id] : NULL;
}

int HUD_LayoutObject(const char *name)
{
    /* Stable bounded cvar names, independent of registration order/name length. */
    char key[32];
    uint64_t hash = UINT64_C(14695981039346656037);
    for (const byte *p = (const byte *)name; *p; p++) {
        hash ^= *p;
        hash *= UINT64_C(1099511628211);
    }
    Q_snprintf(key, sizeof(key), "draw_%016" PRIx64, hash);
    for (int i = HL_BUILTIN_COUNT; i < item_count; i++)
        if (!strcmp(object_keys[i], key))
            return i;
    if (!item_count || item_count == HUD_LAYOUT_MAX || HUD_EditorActive())
        return -1;
    int id = item_count++;
    Q_strlcpy(object_names[id], name, sizeof(object_names[id]));
    Q_strlcpy(object_keys[id], key, sizeof(object_keys[id]));
    HUD_RegisterItem(id, object_keys[id], object_names[id], true);
    return id;
}

void HUD_LayoutBegin(int id)
{
    HUD_LayoutEnd();
    if (id < 0 || id >= item_count)
        return;
    if (bounds_servercount != cl.servercount) {
        memset(last_bounds, 0, sizeof(last_bounds));
        bounds_servercount = cl.servercount;
    }
    const hud_layout_item_t *item = &items[id];
    float scale = scr.hud_scale > 0 ? scr.hud_scale : 1;
    current_x = HUD_EditorValue(item->x);
    current_y = HUD_EditorValue(item->y);
    current_x = isfinite(current_x) ? Q_clip(current_x, -16000, 16000) : 0;
    current_y = isfinite(current_y) ? Q_clip(current_y, -16000, 16000) : 0;
    bool hidden = !HUD_EditorValue(item->visible) || HUD_EditorActive();
    if (HUD_EditorPreview())
        hidden = !HUD_EditorShow(HUD_EDIT_LAYOUT_FIRST + id);
    R_BeginDrawGroup(current_x / scale, current_y / scale, hidden);
    current = id;
}

void HUD_LayoutEnd(void)
{
    if (current < 0)
        return;
    vrect_t bounds;
    if (R_EndDrawGroup(&bounds)) {
        float scale = scr.hud_scale > 0 ? scr.hud_scale : 1;
        bounds.x = Q_rint(bounds.x * scale);
        bounds.y = Q_rint(bounds.y * scale);
        bounds.width = Q_rint(bounds.width * scale);
        bounds.height = Q_rint(bounds.height * scale);
        if (HUD_EditorPreview()) {
            HUD_EditorBounds(HUD_EDIT_LAYOUT_FIRST + current, bounds.x, bounds.y, bounds.width, bounds.height);
        } else {
            bounds.x -= Q_rint(current_x);
            bounds.y -= Q_rint(current_y);
            if (captured[current]) {
                vrect_t old = last_bounds[current];
                int right = max(old.x + old.width, bounds.x + bounds.width);
                int bottom = max(old.y + old.height, bounds.y + bounds.height);
                bounds.x = min(old.x, bounds.x);
                bounds.y = min(old.y, bounds.y);
                bounds.width = right - bounds.x;
                bounds.height = bottom - bounds.y;
            }
            captured[current] = true;
            last_bounds[current] = bounds;
            last_width[current] = Q_rint(r_config.width * scale);
            last_height[current] = Q_rint(r_config.height * scale);
        }
    }
    current = -1;
}

bool HUD_LayoutMatch(const char *layout)
{
    if (strlen(layout) >= sizeof(matched_layout))
        return false;
    if (strcmp(layout, matched_layout)) {
        Q_strlcpy(matched_layout, layout, sizeof(matched_layout));
        matched_count = HudLayout_Match(layout, server_statusbar_layout,
                                        q_countof(server_statusbar_layout),
                                        matched_tokens, q_countof(matched_tokens));
    }
    return matched_count > 0;
}

int HUD_LayoutToken(const char *layout, const char *position)
{
    while (*position && (unsigned char)*position <= ' ')
        position++;
    size_t offset = position - layout;
    for (int i = 0; i < matched_count; i++)
        if (matched_tokens[i].start == offset)
            return matched_tokens[i].group;
    return -1;
}

void HUD_LayoutSetBounds(int id, vrect_t bounds)
{
    if (id < 0 || id >= item_count)
        return;
    last_bounds[id] = bounds;
    last_width[id] = Q_rint(r_config.width * scr.hud_scale);
    last_height[id] = Q_rint(r_config.height * scr.hud_scale);
}

static int HUD_SourceValue(const char *name, int fallback)
{
    cvar_t *var = Cvar_FindVar(name);
    return var && isfinite(var->value) ? (int)Q_clip(var->value, -16000, 16000) : fallback;
}

/* Honor existing position cvars even when their overlay has never been shown. */
static void HUD_PreviewOrigin(int id, int width, int height, vrect_t *r)
{
    if (id == HL_CROSSHAIR || id == HL_HITMARKER) {
        r->x += HUD_SourceValue("ch_x", 0);
        r->y += HUD_SourceValue("ch_y", 0);
    } else if (id == HL_NETALERT) {
        int x = HUD_SourceValue("sh_netalert_x", 0), y = HUD_SourceValue("sh_netalert_y", 353);
        r->x = x == 0 ? (width - r->width) / 2 : x < 0 ? width + x + 1 - r->width : x;
        r->y = y == 0 ? (height - r->height) / 2 : y < 0 ? height + y - r->height + 1 : y;
    } else if (id == HL_CHAT) {
        int x = HUD_SourceValue("scr_chathud_x", 8), y = HUD_SourceValue("scr_chathud_y", -150);
        int lines = Q_clip(HUD_SourceValue("scr_chathud_lines", 4), 1, 32);
        r->height = lines * CHAR_HEIGHT;
        r->x = x < 0 ? width + x + 1 - r->width : x;
        r->y = y < 0 ? height + y - r->height + 1 : y;
    } else if (id == HL_DEMO || id == HL_DEBUGGRAPH) {
        r->width = width;
    }
}

void HUD_LayoutPreview(void)
{
    float scale = scr.hud_scale > 0 ? scr.hud_scale : 1;
    int width = Q_rint(r_config.width * scale), height = Q_rint(r_config.height * scale);
    bool server_selected = false;
    for (int i = 0; i < HL_SERVER_COUNT; i++)
        if (HUD_EditorSelected(HUD_EDIT_LAYOUT_FIRST + i))
            server_selected = true;
    for (int i = 0; i < item_count; i++) {
        if (!HUD_LayoutEditable(i))
            continue;
        bool selected = HUD_EditorSelected(HUD_EDIT_LAYOUT_FIRST + i);
        if (!selected && !(i < HL_SERVER_COUNT && server_selected) &&
            !(last_bounds[i].width && HUD_EditorValue(items[i].visible))) continue;
        vrect_t r = last_bounds[i];
        const char *sample = items[i].name;
        if (i < HL_BUILTIN_COUNT)
            sample = defaults[i].sample;
        if (!r.width || last_width[i] != width || last_height[i] != height) {
            if (i < HL_BUILTIN_COUNT) {
                r = (vrect_t) { Q_rint(width * defaults[i].ax) + defaults[i].x,
                    Q_rint(height * defaults[i].ay) + defaults[i].y, defaults[i].width, defaults[i].height };
            } else {
                r = (vrect_t) { width - 136, 80 + (i - HL_BUILTIN_COUNT) * 10, 128, 8 };
            }
            HUD_PreviewOrigin(i, width, height, &r);
        }
        HUD_LayoutBegin(i);
        R_DrawFill32(r.x, r.y, r.width, r.height, MakeColor(35, 50, 62, 75));
        R_SetColor(MakeColor(180, 205, 220, 220));
        R_DrawString(r.x, r.y, UI_NOSHADOW, max(1, r.width / CHAR_WIDTH), sample, scr.font_pic);
        HUD_LayoutEnd();
    }
    R_ClearColor();
}
