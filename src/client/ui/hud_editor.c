/*
 * Native HUD editor. Renderers report their actual bounds and read a draft only
 * while drawing previews. Applying a layout is the sole cvar mutation path.
 */
#include "ui.h"
#include "../client.h"
#include "client/hud_editor.h"
#include "hud_editor_math.h"
#include "client/hud_layout.h"
#include "../../jump/strafe_helper.h"

typedef struct {
    const char *name;
    hud_edit_id_t owner;
    float low, high;
    cvar_t *var;
    float original, value;
    bool changed;
} hud_setting_t;

static const hud_setting_t native_settings[] = {
    { "sh_ups", HUD_EDIT_UPS, 0, 1 },
    { "sh_ups_x", HUD_EDIT_UPS, SH_UPS_X_MIN, SH_UPS_X_MAX },
    { "sh_ups_y", HUD_EDIT_UPS, SH_UPS_Y_MIN, SH_UPS_Y_MAX },
    { "sh_ups_scale", HUD_EDIT_UPS, SH_UPS_SCALE_MIN, SH_UPS_SCALE_MAX },
    { "sh_draw", HUD_EDIT_STRAFE, 0, 1 },
    { "sh_y", HUD_EDIT_STRAFE, -4000, 4000 },
    { "sh_height", HUD_EDIT_STRAFE, 4, 80 },
    { "sh_scale", HUD_EDIT_STRAFE, 0.25f, 8 },
    { "sh_efficiency", HUD_EDIT_EFFICIENCY, 0, 1 },
    { "sh_efficiency_x", HUD_EDIT_EFFICIENCY, SH_EFFICIENCY_X_MIN, SH_EFFICIENCY_X_MAX },
    { "sh_efficiency_y", HUD_EDIT_EFFICIENCY, SH_EFFICIENCY_Y_MIN, SH_EFFICIENCY_Y_MAX },
    { "sh_efficiency_width", HUD_EDIT_EFFICIENCY, SH_EFFICIENCY_WIDTH_MIN, SH_EFFICIENCY_WIDTH_MAX },
    { "sh_efficiency_height", HUD_EDIT_EFFICIENCY, SH_EFFICIENCY_HEIGHT_MIN, SH_EFFICIENCY_HEIGHT_MAX },
    { "sh_efficiency_text_scale", HUD_EDIT_EFFICIENCY, SH_EFFICIENCY_TEXT_SCALE_MIN, SH_EFFICIENCY_TEXT_SCALE_MAX },
    { "sh_netmeter", HUD_EDIT_NETWORK, 0, 3 },
    { "sh_lagometer_x", HUD_EDIT_NETWORK, -4000, 4000 },
    { "sh_lagometer_y", HUD_EDIT_NETWORK, -4000, 4000 },
    { "sh_netgraph_y", HUD_EDIT_NETWORK, -4000, 4000 },
    { "sh_netgraph_height", HUD_EDIT_NETWORK, 1, 4000 },
    { "sh_histogram_x", HUD_EDIT_NETWORK, -4000, 4000 },
    { "sh_histogram_y", HUD_EDIT_NETWORK, -4000, 4000 },
    { "sh_histogram_width", HUD_EDIT_NETWORK, 10, 4000 },
    { "sh_histogram_width_mode", HUD_EDIT_NETWORK, 0, 2 },
    { "sh_histogram_height", HUD_EDIT_NETWORK, 1, 4000 },
};
enum { HUD_NATIVE_SETTINGS = q_countof(native_settings) };
static hud_setting_t settings[HUD_NATIVE_SETTINGS + HUD_LAYOUT_MAX * 3];
static int setting_count;

static const struct {
    const char *name;
    const char *enable;
    const char *key;
} native_widgets[HUD_EDIT_LAYOUT_FIRST] = {
    [HUD_EDIT_UPS] = { "Speed (UPS)", "sh_ups", "ups" },
    [HUD_EDIT_STRAFE] = { "Strafe helper", "sh_draw", "strafe" },
    [HUD_EDIT_EFFICIENCY] = { "Strafe efficiency", "sh_efficiency", "efficiency" },
    [HUD_EDIT_NETWORK] = { "Network monitor", "sh_netmeter", "network" },
};

typedef enum {
    HUD_BUTTON_APPLY,
    HUD_BUTTON_CANCEL,
    HUD_BUTTON_RESET_SELECTED,
    HUD_BUTTON_SNAP,
    HUD_BUTTON_VISIBLE,
    HUD_BUTTON_HIDE_TOOLS,
    HUD_BUTTON_PREVIOUS,
    HUD_BUTTON_NEXT,
    HUD_BUTTON_MOVE_LEFT,
    HUD_BUTTON_MOVE_RIGHT,
    HUD_BUTTON_MOVE_UP,
    HUD_BUTTON_MOVE_DOWN,
    HUD_BUTTON_RESET_ALL,
    HUD_BUTTON_UNDO,
    HUD_BUTTON_REDO,
    HUD_BUTTON_LOCK,
    HUD_BUTTON_FOCUS,
    HUD_BUTTON_ALIGN_LEFT,
    HUD_BUTTON_ALIGN_HCENTER,
    HUD_BUTTON_ALIGN_RIGHT,
    HUD_BUTTON_ALIGN_TOP,
    HUD_BUTTON_ALIGN_VCENTER,
    HUD_BUTTON_ALIGN_BOTTOM,
    HUD_BUTTON_ARRANGE_SECTION,
    HUD_BUTTON_RESET_SECTION,
    HUD_BUTTON_COUNT
} hud_button_t;

typedef enum {
    HUD_CATEGORY_ALL,
    HUD_CATEGORY_CLIENT,
    HUD_CATEGORY_SERVER,
    HUD_CATEGORY_COUNT
} hud_category_t;

typedef enum {
    HUD_SECTION_NONE,
    HUD_SECTION_ARRANGE,
    HUD_SECTION_RESET
} hud_tools_section_t;

static struct {
    menuFrameWork_t menu;
    bool open, preview, dragging, resizing, snap, hide_tools, dragging_tools, focus;
    bool locked[HUD_EDIT_COUNT], original_locks[HUD_EDIT_COUNT];
    cvar_t *lock_vars[HUD_EDIT_COUNT];
    int history_pos, history_count;
    float guide_x, guide_y;
    int selected, network_mode, servercount, list_first, page_rows;
    hud_category_t category;
    hud_tools_section_t tools_section;
    float width, height, scale, mouse_x, mouse_y, grab_x, grab_y;
    float start_width, start_height, start_scale, start_x, start_y;
    float drag_values[q_countof(settings)];
    bool drag_changed[q_countof(settings)];
    vrect_t drag_bounds;
    vrect_t bounds[HUD_EDIT_COUNT], rows[HUD_EDIT_COUNT];
    vrect_t buttons[HUD_BUTTON_COUNT], categories[HUD_CATEGORY_COUNT];
    vrect_t tools, tools_title;
    float tools_grab_x, tools_grab_y;
    active_t active;
} editor;

#define HUD_HISTORY_STATES 65
static struct {
    float values[q_countof(settings)];
    /* Preview clamping alone is not an edit. */
    bool changed[q_countof(settings)];
    bool locked[HUD_EDIT_COUNT];
    int selected, network_mode;
} history[HUD_HISTORY_STATES];

static void HUD_HistoryCapture(int slot)
{
    for (size_t i = 0; i < setting_count; i++) {
        history[slot].values[i] = settings[i].value;
        history[slot].changed[i] = settings[i].changed;
    }
    memcpy(history[slot].locked, editor.locked, sizeof(editor.locked));
    history[slot].selected = editor.selected;
    history[slot].network_mode = editor.network_mode;
}

static void HUD_HistoryBegin(void)
{
    if (editor.history_count)
        return;
    editor.history_pos = 0;
    editor.history_count = 1;
    HUD_HistoryCapture(0);
}

static void HUD_HistoryCommit(void)
{
    if (!editor.history_count)
        return;
    int slot = editor.history_pos;
    bool changed = memcmp(history[slot].locked, editor.locked, sizeof(editor.locked)) != 0;
    for (size_t i = 0; i < setting_count; i++)
        changed |= settings[i].value != history[slot].values[i] ||
                   settings[i].changed != history[slot].changed[i];
    if (!changed)
        return;
    if (slot == HUD_HISTORY_STATES - 1) {
        memmove(history, history + 1, sizeof(history) - sizeof(history[0]));
        slot--;
    }
    editor.history_pos = slot + 1;
    editor.history_count = editor.history_pos + 1;
    HUD_HistoryCapture(editor.history_pos);
}

static void HUD_EndDrag(void)
{
    if (editor.dragging)
        HUD_HistoryCommit();
    editor.dragging = false;
    editor.guide_x = editor.guide_y = NAN;
}

static bool HUD_Locked(int id)
{
    /* Changing the helper geometry also moves its attached efficiency display. */
    return editor.locked[id] || (id == HUD_EDIT_STRAFE && editor.locked[HUD_EDIT_EFFICIENCY]);
}

static bool HUD_CanvasItem(int id)
{
    return !editor.focus || id == editor.selected;
}

bool HUD_EditorShow(int id)
{
    return !editor.preview || HUD_CanvasItem(id);
}

static int HUD_Limit(void)
{
    return HUD_EDIT_LAYOUT_FIRST + HUD_LayoutCount();
}

static bool HUD_Editable(int id)
{
    return id < HUD_EDIT_LAYOUT_FIRST || HUD_LayoutEditable(id - HUD_EDIT_LAYOUT_FIRST);
}

static hud_category_t HUD_Category(int id)
{
    if (id < HUD_EDIT_LAYOUT_FIRST)
        return HUD_CATEGORY_CLIENT;
    int layout = id - HUD_EDIT_LAYOUT_FIRST;
    return layout < HL_SERVER_COUNT || layout == HL_SCOREBOARD || layout == HL_OTHER_STATUS
        ? HUD_CATEGORY_SERVER : HUD_CATEGORY_CLIENT;
}

static bool HUD_ListItem(int id)
{
    return HUD_Editable(id) && (editor.category == HUD_CATEGORY_ALL || HUD_Category(id) == editor.category);
}
/* Rows are compact; selection and renderer bounds retain their stable HUD IDs. */
static int HUD_Row(int id)
{
    int row = 0;
    for (int i = 0; i < id; i++)
        if (HUD_ListItem(i))
            row++;
    return row;
}

static int HUD_Count(void)
{
    return HUD_Row(HUD_Limit());
}

static int HUD_Entry(int row)
{
    for (int i = 0; i < HUD_Limit(); i++)
        if (HUD_ListItem(i) && row-- == 0)
            return i;
    return HUD_EDIT_UPS;
}

static void HUD_SetCategory(hud_category_t category)
{
    HUD_EndDrag();
    editor.category = category;
    if (!HUD_Count())
        editor.category = HUD_CATEGORY_ALL;
    if (!HUD_ListItem(editor.selected))
        editor.selected = HUD_Entry(0);
    editor.list_first = 0;
    editor.dragging = false;
    memset(editor.rows, 0, sizeof(editor.rows));
}

static void HUD_SelectStep(int step)
{
    HUD_EndDrag();
    editor.selected = HUD_Entry(Q_clip(HUD_Row(editor.selected) + step, 0, HUD_Count() - 1));
    editor.dragging = false;
}

static void HUD_HistoryRestore(int slot, int selected)
{
    for (size_t i = 0; i < setting_count; i++) {
        settings[i].value = history[slot].values[i];
        settings[i].changed = history[slot].changed[i];
    }
    memcpy(editor.locked, history[slot].locked, sizeof(editor.locked));
    editor.network_mode = history[slot].network_mode;
    editor.history_pos = slot;
    editor.selected = selected;
    if (!HUD_ListItem(selected))
        HUD_SetCategory(HUD_CATEGORY_ALL);
    editor.guide_x = editor.guide_y = NAN;
}

static void HUD_Undo(void)
{
    HUD_EndDrag();
    if (editor.history_pos > 0)
        HUD_HistoryRestore(editor.history_pos - 1, history[editor.history_pos].selected);
}

static void HUD_Redo(void)
{
    HUD_EndDrag();
    if (editor.history_pos + 1 < editor.history_count)
        HUD_HistoryRestore(editor.history_pos + 1, history[editor.history_pos + 1].selected);
}

static const char *HUD_Name(int id)
{
    return id < HUD_EDIT_LAYOUT_FIRST ? native_widgets[id].name : HUD_LayoutItem(id - HUD_EDIT_LAYOUT_FIRST)->name;
}

static const char *HUD_Enable(int id)
{
    return id < HUD_EDIT_LAYOUT_FIRST ? native_widgets[id].enable : HUD_LayoutItem(id - HUD_EDIT_LAYOUT_FIRST)->visible->name;
}

static bool HUD_CanResize(void)
{
    return !HUD_Locked(editor.selected) && editor.selected < HUD_EDIT_LAYOUT_FIRST &&
           (editor.selected != HUD_EDIT_NETWORK || editor.network_mode != 1);
}

bool HUD_EditorSelected(int id)
{
    return editor.preview && editor.selected == id;
}

static hud_setting_t *HUD_Setting(const char *name)
{
    for (size_t i = 0; i < setting_count; i++)
        if (!strcmp(settings[i].name, name))
            return &settings[i];
    return NULL;
}

static float HUD_Value(const char *name)
{
    return HUD_Setting(name)->value;
}

static void HUD_Set(const char *name, float value)
{
    hud_setting_t *s = HUD_Setting(name);
    s->value = HudEdit_Clamp(value, s->low, s->high);
    s->changed = s->value != s->original;
}

bool HUD_EditorActive(void)
{
    return editor.open && uis.activeMenu == &editor.menu;
}

bool HUD_EditorPreview(void)
{
    return editor.preview;
}

int HUD_EditorNetworkMode(void)
{
    return editor.network_mode;
}

float HUD_EditorValue(const cvar_t *var)
{
    if (editor.preview)
        for (size_t i = 0; i < setting_count; i++)
            if (settings[i].var == var)
                return settings[i].value;
    return var->value;
}

float HUD_EditorClamp(cvar_t *var, float low, float high)
{
    if (editor.preview)
        return HudEdit_Clamp(HUD_EditorValue(var), low, high);
    return Cvar_ClampValue(var, low, high);
}

/* Multiple calls union the bar, text, and border of one widget. */
void HUD_EditorBounds(hud_edit_id_t id, float x, float y, float w, float h)
{
    if (!editor.preview || id < 0 || id >= HUD_EDIT_COUNT)
        return;
    vrect_t *r = &editor.bounds[id];
    int left = (int)floorf(x), top = (int)floorf(y);
    int right = (int)ceilf(x + w), bottom = (int)ceilf(y + h);
    if (r->width) {
        left = min(left, r->x);
        top = min(top, r->y);
        right = max(right, r->x + r->width);
        bottom = max(bottom, r->y + r->height);
    }
    *r = (vrect_t) { left, top, max(1, right - left), max(1, bottom - top) };
}

static void HUD_Pop(menuFrameWork_t *menu)
{
    editor.open = editor.preview = editor.dragging = false;
}

static void HUD_Size(menuFrameWork_t *menu)
{
    menu->mins[0] = menu->mins[1] = 0;
    menu->maxs[0] = uis.width;
    menu->maxs[1] = uis.height;
}

static void HUD_Reset(void)
{
    if (HUD_Locked(editor.selected))
        return;
    for (size_t i = 0; i < setting_count; i++)
        if (settings[i].owner == editor.selected)
            HUD_Set(settings[i].name, strtof(settings[i].var->default_string, NULL));
    editor.dragging = false;
}

static void HUD_ResetAll(void)
{
    for (size_t i = 0; i < setting_count; i++)
        if (HUD_Editable(settings[i].owner) && !HUD_Locked(settings[i].owner))
            HUD_Set(settings[i].name, strtof(settings[i].var->default_string, NULL));
    editor.dragging = false;
}

static void HUD_Apply(void)
{
    HUD_EndDrag();
    for (size_t i = 0; i < setting_count; i++)
        if (settings[i].changed)
            Cvar_SetValue(settings[i].var, settings[i].value, FROM_CONSOLE);
    for (int i = 0; i < HUD_Limit(); i++)
        if (editor.lock_vars[i] && editor.locked[i] != editor.original_locks[i])
            Cvar_SetValue(editor.lock_vars[i], editor.locked[i], FROM_CONSOLE);
    UI_PopMenu();
}

static void HUD_Toggle(void)
{
    if (HUD_Locked(editor.selected))
        return;
    const char *name = HUD_Enable(editor.selected);
    HUD_Set(name, HUD_Value(name) ? 0 :
            editor.selected == HUD_EDIT_NETWORK ? editor.network_mode : 1);
}

static void HUD_NetworkAxes(const char **x, const char **y)
{
    *x = editor.network_mode == 1 ? "sh_lagometer_x" :
         editor.network_mode == 3 ? "sh_histogram_x" : NULL;
    *y = editor.network_mode == 1 ? "sh_lagometer_y" :
         editor.network_mode == 3 ? "sh_histogram_y" : "sh_netgraph_y";
}

static void HUD_Move(float dx, float dy, bool snap)
{
    editor.guide_x = editor.guide_y = NAN;
    if (HUD_Locked(editor.selected))
        return;
    vrect_t r = editor.bounds[editor.selected];
    float x = r.x + dx, y = r.y + dy;
    if (snap) {
        hud_edit_snap_axis_t sx, sy;
        HudEdit_SnapAxisInit(&sx, x, r.width, editor.width, 6 * editor.scale);
        HudEdit_SnapAxisInit(&sy, y, r.height, editor.height, 6 * editor.scale);
        for (int i = 0; i < HUD_Limit(); i++) {
            const vrect_t *other = &editor.bounds[i];
            if (i == editor.selected || !HUD_Editable(i) || !other->width ||
                !HUD_Value(HUD_Enable(i))) continue;
            /* Do not snap an attached widget to geometry that moves with it. */
            if (editor.selected == HUD_EDIT_STRAFE && i == HUD_EDIT_EFFICIENCY)
                continue;
            HudEdit_SnapAxisTarget(&sx, other->x, other->width);
            HudEdit_SnapAxisTarget(&sy, other->y, other->height);
        }
        x = HudEdit_SnapAxisResult(&sx, &editor.guide_x);
        y = HudEdit_SnapAxisResult(&sy, &editor.guide_y);
        if (editor.selected == HUD_EDIT_STRAFE ||
            (editor.selected == HUD_EDIT_NETWORK && editor.network_mode == 2)) {
            x = r.x;
            editor.guide_x = NAN;
        }
    } else {
        x = HudEdit_Clamp(x, 0, editor.width - r.width);
        y = HudEdit_Clamp(y, 0, editor.height - r.height);
    }
    dx = x - r.x;
    dy = y - r.y;
    if (editor.selected >= HUD_EDIT_LAYOUT_FIRST) {
        const hud_layout_item_t *item = HUD_LayoutItem(editor.selected - HUD_EDIT_LAYOUT_FIRST);
        HUD_Set(item->x->name, HUD_Value(item->x->name) + dx);
        HUD_Set(item->y->name, HUD_Value(item->y->name) + dy);
        return;
    }
    switch (editor.selected) {
    case HUD_EDIT_UPS:
        HUD_Set("sh_ups_x", HUD_Value("sh_ups_x") + dx);
        HUD_Set("sh_ups_y", HUD_Value("sh_ups_y") + dy);
        break;
    case HUD_EDIT_STRAFE:
        HUD_Set("sh_y", HUD_Value("sh_y") + dy);
        break;
    case HUD_EDIT_EFFICIENCY: {
        float h = HUD_Value("sh_efficiency_height"), hh = HUD_Value("sh_height");
        float helper_top = (editor.height - hh) * 0.5f + HUD_Value("sh_y");
        float offset = HUD_Value("sh_efficiency_y");
        float top = offset >= 0 ? helper_top + hh + offset : helper_top + offset - h;
        HUD_Set("sh_efficiency_x", HUD_Value("sh_efficiency_x") + dx);
        HUD_Set("sh_efficiency_y", HudEdit_EfficiencyOffset(top + dy, h, helper_top, hh));
        break;
    }
    case HUD_EDIT_NETWORK: {
        const char *xn, *yn;
        HUD_NetworkAxes(&xn, &yn);
        if (xn)
            HUD_Set(xn, HudEdit_EdgeOffset(x, editor.width, r.width, HUD_Value(xn)));
        HUD_Set(yn, HudEdit_EdgeOffset(y, editor.height, r.height, HUD_Value(yn)));
        break;
    }
    default:
        break;
    }
}

static bool HUD_EfficiencyTextOnly(void)
{
    const char *style = cl_strafehelperEffStyle ? cl_strafehelperEffStyle->string : NULL;
    return style && (!Q_stricmp(style, "text") || !strcmp(style, "2"));
}

static void HUD_StartResize(void)
{
    vrect_t r = editor.bounds[editor.selected];
    editor.start_width = r.width;
    editor.start_height = r.height;
    editor.start_scale = 1;
    switch (editor.selected) {
    case HUD_EDIT_UPS:
        editor.start_scale = HUD_Value("sh_ups_scale");
        break;
    case HUD_EDIT_STRAFE:
        editor.start_scale = HUD_Value("sh_scale");
        editor.start_height = HUD_Value("sh_height");
        break;
    case HUD_EDIT_EFFICIENCY:
        if (HUD_EfficiencyTextOnly()) {
            editor.start_scale = HUD_Value("sh_efficiency_text_scale");
        } else {
            editor.start_width = HUD_Value("sh_efficiency_width");
            editor.start_height = HUD_Value("sh_efficiency_height");
        }
        break;
    default:
        break;
    }
}

static void HUD_Resize(float dx, float dy)
{
    if (HUD_Locked(editor.selected))
        return;
    switch (editor.selected) {
    case HUD_EDIT_UPS:
        HUD_Set("sh_ups_scale", editor.start_scale * fmaxf(0.1f,
                (editor.start_width + dx) / fmaxf(1, editor.start_width)));
        break;
    case HUD_EDIT_STRAFE:
        HUD_Set("sh_scale", editor.start_scale * fmaxf(0.1f,
                (editor.start_width + 2 * dx) / fmaxf(1, editor.start_width)));
        HUD_Set("sh_height", editor.start_height + dy);
        break;
    case HUD_EDIT_EFFICIENCY:
        if (HUD_EfficiencyTextOnly()) {
            HUD_Set("sh_efficiency_text_scale", editor.start_scale * fmaxf(0.1f,
                    (editor.start_width + dx) / fmaxf(1, editor.start_width)));
        } else {
            HUD_Set("sh_efficiency_width", fminf(editor.width, editor.start_width + 2 * dx));
            HUD_Set("sh_efficiency_height", editor.start_height + dy);
        }
        break;
    case HUD_EDIT_NETWORK:
        if (editor.network_mode == 2)
            HUD_Set("sh_netgraph_height", fminf(editor.height, editor.start_height + dy));
        else if (editor.network_mode == 3) {
            if (dx != 0)
                HUD_Set("sh_histogram_width_mode", 0);
            HUD_Set("sh_histogram_width", fmaxf(10, fminf(editor.width, editor.start_width + dx)));
            HUD_Set("sh_histogram_height", fminf(editor.height, editor.start_height + dy));
        }
        break;
    default:
        break;
    }
}

static void HUD_Align(hud_button_t button)
{
    vrect_t r = editor.bounds[editor.selected];
    float dx = 0, dy = 0;

    switch (button) {
    case HUD_BUTTON_ALIGN_LEFT:
        dx = -r.x;
        break;
    case HUD_BUTTON_ALIGN_HCENTER:
        dx = (editor.width - r.width) * 0.5f - r.x;
        break;
    case HUD_BUTTON_ALIGN_RIGHT:
        dx = editor.width - r.width - r.x;
        break;
    case HUD_BUTTON_ALIGN_TOP:
        dy = -r.y;
        break;
    case HUD_BUTTON_ALIGN_VCENTER:
        dy = (editor.height - r.height) * 0.5f - r.y;
        break;
    case HUD_BUTTON_ALIGN_BOTTOM:
        dy = editor.height - r.height - r.y;
        break;
    default:
        return;
    }
    HUD_Move(dx, dy, false);
}

static void HUD_Nudge(float dx, float dy, bool resize)
{
    HUD_HistoryBegin();
    if (resize) {
        HUD_StartResize();
        HUD_Resize(dx, dy);
    } else {
        HUD_Move(dx, dy, false);
    }
    HUD_HistoryCommit();
}
/* Invalidate geometry before a section change can receive another click. */
static void HUD_ClearToolsHits(void)
{
    memset(editor.buttons, 0, sizeof(editor.buttons));
    memset(editor.rows, 0, sizeof(editor.rows));
    memset(editor.categories, 0, sizeof(editor.categories));
    editor.tools_title = (vrect_t) { 0 };
}

static void HUD_Command(hud_button_t button)
{
    HUD_EndDrag();
    switch (button) {
    case HUD_BUTTON_APPLY:
        HUD_Apply();
        return;
    case HUD_BUTTON_CANCEL:
        UI_PopMenu();
        return;
    case HUD_BUTTON_SNAP:
        editor.snap = !editor.snap;
        return;
    case HUD_BUTTON_HIDE_TOOLS:
        editor.hide_tools = true;
        return;
    case HUD_BUTTON_PREVIOUS:
        HUD_SelectStep(-1);
        return;
    case HUD_BUTTON_NEXT:
        HUD_SelectStep(1);
        return;
    case HUD_BUTTON_UNDO:
        HUD_Undo();
        return;
    case HUD_BUTTON_REDO:
        HUD_Redo();
        return;
    case HUD_BUTTON_FOCUS:
        editor.focus = !editor.focus;
        return;
    case HUD_BUTTON_ARRANGE_SECTION:
        editor.tools_section = editor.tools_section == HUD_SECTION_ARRANGE
            ? HUD_SECTION_NONE : HUD_SECTION_ARRANGE;
        HUD_ClearToolsHits();
        return;
    case HUD_BUTTON_RESET_SECTION:
        editor.tools_section = editor.tools_section == HUD_SECTION_RESET
            ? HUD_SECTION_NONE : HUD_SECTION_RESET;
        HUD_ClearToolsHits();
        return;
    default:
        break;
    }

    HUD_HistoryBegin();
    switch (button) {
    case HUD_BUTTON_RESET_SELECTED:
        HUD_Reset();
        break;
    case HUD_BUTTON_VISIBLE:
        HUD_Toggle();
        break;
    case HUD_BUTTON_RESET_ALL:
        HUD_ResetAll();
        break;
    case HUD_BUTTON_LOCK:
        editor.locked[editor.selected] = !editor.locked[editor.selected];
        break;
    case HUD_BUTTON_ALIGN_LEFT:
    case HUD_BUTTON_ALIGN_HCENTER:
    case HUD_BUTTON_ALIGN_RIGHT:
    case HUD_BUTTON_ALIGN_TOP:
    case HUD_BUTTON_ALIGN_VCENTER:
    case HUD_BUTTON_ALIGN_BOTTOM:
        HUD_Align(button);
        break;
    case HUD_BUTTON_MOVE_LEFT:
    case HUD_BUTTON_MOVE_RIGHT:
    case HUD_BUTTON_MOVE_UP:
    case HUD_BUTTON_MOVE_DOWN: {
        float step = Key_IsDown(K_SHIFT) ? 10 : 1;
        float dx = button == HUD_BUTTON_MOVE_LEFT ? -step : button == HUD_BUTTON_MOVE_RIGHT ? step : 0;
        float dy = button == HUD_BUTTON_MOVE_UP ? -step : button == HUD_BUTTON_MOVE_DOWN ? step : 0;
        HUD_Move(dx, dy, false);
        break;
    }
    default:
        break;
    }
    HUD_HistoryCommit();
}

static bool HUD_Hit(const vrect_t *r, float x, float y)
{
    return r->width > 0 && x >= r->x && y >= r->y &&
           x < r->x + r->width && y < r->y + r->height;
}

/* Keep normal menu text size; shrink only when the toolbox cannot fit. */
static float HUD_ToolsZoom(void)
{
    if (uis.width <= 0 || uis.height <= 0)
        return 1;
    return min(1.0f, min(uis.width / 312.0f, uis.height / 270.0f));
}

static float HUD_ToolsMouse(int axis)
{
    return uis.mouseCoords[axis] / HUD_ToolsZoom();
}

static void HUD_ClampTools(void)
{
    editor.tools.x = Q_clip(editor.tools.x, 0, max(0, (int)(uis.width / HUD_ToolsZoom()) - editor.tools.width));
    editor.tools.y = Q_clip(editor.tools.y, 0, max(0, (int)(uis.height / HUD_ToolsZoom()) - editor.tools.height));
}

static bool HUD_ToolsVisible(void)
{
    return !editor.hide_tools && !editor.dragging;
}

void HUD_EditorMouse(int x, int y)
{
    if (!HUD_EditorActive())
        return;
    editor.mouse_x = x * editor.scale;
    editor.mouse_y = y * editor.scale;
    if (!Key_IsDown(K_MOUSE1) || cls.active != ACT_ACTIVATED) {
        HUD_EndDrag();
        editor.dragging_tools = false;
    }
    if (editor.dragging_tools) {
        editor.tools.x = Q_rint(x * uis.scale / HUD_ToolsZoom() - editor.tools_grab_x);
        editor.tools.y = Q_rint(y * uis.scale / HUD_ToolsZoom() - editor.tools_grab_y);
        HUD_ClampTools();
        return;
    }
    if (!editor.dragging)
        return;
    /* Mouse events can arrive several times before the next rendered frame. */
    for (size_t i = 0; i < setting_count; i++) {
        settings[i].value = editor.drag_values[i];
        settings[i].changed = editor.drag_changed[i];
    }
    vrect_t previous_bounds = editor.bounds[editor.selected];
    editor.bounds[editor.selected] = editor.drag_bounds;
    if (editor.resizing)
        HUD_Resize(editor.mouse_x - editor.start_x, editor.mouse_y - editor.start_y);
    else {
        vrect_t r = editor.bounds[editor.selected];
        HUD_Move(editor.mouse_x - editor.grab_x - r.x,
                 editor.mouse_y - editor.grab_y - r.y,
                 editor.snap && !Key_IsDown(K_ALT));
    }
    editor.bounds[editor.selected] = previous_bounds;
}

bool HUD_EditorKey(int key, bool down)
{
    if (!HUD_EditorActive())
        return false;
    if (!down) {
        if (key == K_MOUSE1) {
            HUD_EndDrag();
            editor.dragging_tools = false;
        }
        return true;
    }
    if (key != K_MOUSE1)
        HUD_EndDrag();
    if (Key_IsDown(K_CTRL) && key == 'z') {
        if (Key_IsDown(K_SHIFT))
            HUD_Redo();
        else
            HUD_Undo();
        return true;
    }
    if (Key_IsDown(K_CTRL) && key == 'y') {
        HUD_Redo();
        return true;
    }
    if (key == K_ESCAPE) {
        UI_PopMenu();
        return true;
    }
    if (key == K_ENTER) {
        HUD_Apply();
        return true;
    }
    if (key == K_TAB) {
        editor.selected = HUD_Entry((HUD_Row(editor.selected) + (Key_IsDown(K_SHIFT) ? HUD_Count() - 1 : 1)) % HUD_Count());
        editor.dragging = false;
    } else if (key == K_MWHEELDOWN || key == K_MWHEELUP || key == K_PGDN || key == K_PGUP) {
        int step = (key == K_PGDN || key == K_PGUP) ? max(1, editor.page_rows) : 1;
        if (key == K_MWHEELUP || key == K_PGUP)
            step = -step;
        HUD_SelectStep(step);
    } else if (key >= '1' && key <= '3') {
        HUD_SetCategory((hud_category_t)(key - '1'));
    } else if (key == 'r')
        HUD_Command(Key_IsDown(K_SHIFT) ? HUD_BUTTON_RESET_ALL : HUD_BUTTON_RESET_SELECTED);
    else if (key == 's')
        editor.snap = !editor.snap;
    else if (key == 'v')
        HUD_Command(HUD_BUTTON_VISIBLE);
    else if (key == 'l')
        HUD_Command(HUD_BUTTON_LOCK);
    else if (key == 'f')
        HUD_Command(HUD_BUTTON_FOCUS);
    else if (key == 'a')
        HUD_Command(HUD_BUTTON_ARRANGE_SECTION);
    else if (key == 'd')
        HUD_Command(HUD_BUTTON_RESET_SECTION);
    else if (key == 'h') {
        editor.hide_tools = !editor.hide_tools;
        editor.dragging_tools = false;
    } else if (key == K_LEFTARROW || key == K_RIGHTARROW || key == K_UPARROW || key == K_DOWNARROW) {
        float step = Key_IsDown(K_SHIFT) ? 10 : 1;
        float dx = key == K_LEFTARROW ? -step : key == K_RIGHTARROW ? step : 0;
        float dy = key == K_UPARROW ? -step : key == K_DOWNARROW ? step : 0;
        HUD_Nudge(dx, dy, Key_IsDown(K_CTRL));
    } else if (key == K_MOUSE1) {
        HUD_EndDrag();
        editor.dragging_tools = false;
        float ux = HUD_ToolsMouse(0), uy = HUD_ToolsMouse(1);
        for (int i = 0; !editor.hide_tools && i < HUD_CATEGORY_COUNT; i++) {
            if (!HUD_Hit(&editor.categories[i], ux, uy))
                continue;
            HUD_SetCategory((hud_category_t)i);
            return true;
        }
        for (int i = 0; !editor.hide_tools && i < q_countof(editor.buttons); i++) {
            if (!HUD_Hit(&editor.buttons[i], ux, uy))
                continue;
            HUD_Command((hud_button_t)i);
            return true;
        }
        for (int i = 0; !editor.hide_tools && i < HUD_Limit(); i++)
            if (HUD_ListItem(i) && HUD_Hit(&editor.rows[i], ux, uy)) {
                editor.selected = i;
                return true;
            }
        if (!editor.hide_tools && HUD_Hit(&editor.tools_title, ux, uy)) {
            editor.dragging_tools = true;
            editor.tools_grab_x = ux - editor.tools.x;
            editor.tools_grab_y = uy - editor.tools.y;
            return true;
        }
        /* Toolbox padding must not start dragging a HUD element underneath it. */
        if (!editor.hide_tools && HUD_Hit(&editor.tools, ux, uy))
            return true;
        /* Selected widget wins overlaps; Tab/list can select anything covered. */
        int hit = -1;
        for (int i = 0; i < HUD_Limit(); i++)
            if (HUD_Editable(i) && HUD_CanvasItem(i) && !HUD_Locked(i) && HUD_Hit(&editor.bounds[i], editor.mouse_x, editor.mouse_y))
                hit = i;
        vrect_t r = editor.bounds[editor.selected];
        int handle = max(4, Q_rint(8 * editor.scale));
        vrect_t corner = { r.x + r.width - handle, r.y + r.height - handle, handle * 2, handle * 2 };
        editor.resizing = HUD_Hit(&corner, editor.mouse_x, editor.mouse_y) &&
                          HUD_CanResize();
        if (!HUD_Locked(editor.selected) &&
            (HUD_Hit(&r, editor.mouse_x, editor.mouse_y) || editor.resizing))
            hit = editor.selected;
        if (hit < 0)
            return true;
        editor.selected = hit;
        r = editor.bounds[hit];
        if (!HUD_ListItem(hit))
            HUD_SetCategory(HUD_CATEGORY_ALL);
        HUD_HistoryBegin();
        editor.dragging = true;
        editor.grab_x = editor.mouse_x - r.x;
        editor.grab_y = editor.mouse_y - r.y;
        editor.start_x = editor.mouse_x;
        editor.start_y = editor.mouse_y;
        editor.drag_bounds = r;
        for (size_t i = 0; i < setting_count; i++) {
            editor.drag_values[i] = settings[i].value;
            editor.drag_changed[i] = settings[i].changed;
        }
        HUD_StartResize();
    }
    return true;
}

static void HUD_Outline(const vrect_t *r, uint32_t color)
{
    if (!r->width)
        return;
    R_DrawFill32(r->x, r->y, r->width, 1, color);
    R_DrawFill32(r->x, r->y + r->height - 1, r->width, 1, color);
    R_DrawFill32(r->x, r->y, 1, r->height, color);
    R_DrawFill32(r->x + r->width - 1, r->y, 1, r->height, color);
}

/* All toolbox controls share the same hover and active feedback. */
static void HUD_Button(vrect_t *rect, int x, int y, int width, const char *label, bool active)
{
    *rect = (vrect_t) { x, y, width, 18 };
    bool hover = HUD_Hit(rect, HUD_ToolsMouse(0), HUD_ToolsMouse(1));
    uint32_t color = active ? MakeColor(48, 91, 102, 255) : MakeColor(34, 44, 53, 255);
    if (hover)
        color = MakeColor(66, 89, 104, 255);
    R_DrawFill32(x, y, width, 18, color);
    if (active)
        R_DrawFill32(x, y + 17, width, 1, MakeColor(115, 218, 214, 255));
    UI_DrawString(x + width / 2, y + 5, UI_CENTER, label);
}

static void HUD_Draw(menuFrameWork_t *menu)
{
    if (!HUD_EditorActive())
        return;
    if (cl.servercount != editor.servercount || (Key_GetDest() & KEY_CONSOLE)) {
        UI_PopMenu();
        return;
    }
    float scale = scr.hud_scale > 0 ? scr.hud_scale : 1;
    float width = Q_rint(r_config.width * scale), height = Q_rint(r_config.height * scale);
    if (width != editor.width || height != editor.height || scale != editor.scale || cls.active != editor.active) {
        HUD_EndDrag();
        editor.dragging_tools = false;
    }
    if (HUD_Value("sh_netmeter") > 0)
        editor.network_mode = (int)HUD_Value("sh_netmeter");
    editor.width = width;
    editor.height = height;
    editor.scale = scale;
    editor.active = cls.active;
    int saved_width = scr.hud_width, saved_height = scr.hud_height;
    scr.hud_width = width;
    scr.hud_height = height;
    memset(editor.bounds, 0, sizeof(editor.bounds));
    editor.preview = true;
    R_SetScale(scale);
    R_ClearColor();
    const struct StrafeHelperParams params = {
        .center = cl_strafeHelperCenter->integer,
        .center_marker = cl_strafeHelperCenterMarker->integer,
        .scale = HUD_Value("sh_scale"), .height = HUD_Value("sh_height"),
        .y = HUD_Value("sh_y"), .hud_scale = scale,
    };
    StrafeHelper_DrawPreview(&params, width, height, scr.font_pic);
    SH_Ups_Draw(width, height, scale, scr.font_pic);
    SH_NetMeter_Draw();
    HUD_LayoutPreview();
    editor.preview = false;
    scr.hud_width = saved_width;
    scr.hud_height = saved_height;
    for (int i = 0; i < HUD_Limit(); i++)
        if (HUD_Editable(i) && HUD_CanvasItem(i))
            HUD_Outline(&editor.bounds[i], i == editor.selected ?
                    MakeColor(255, 205, 80, 255) : MakeColor(130, 153, 170, 95));
    vrect_t r = editor.bounds[editor.selected];
    if (r.width) {
        const char *label = HUD_Name(editor.selected);
        int label_width = min((int)strlen(label), 30) * CHAR_WIDTH + 8;
        int lx = Q_clip(r.x, 0, max(0, (int)width - label_width));
        int ly = Q_clip(r.y - 15, 0, max(0, (int)height - 14));
        R_DrawFill32(lx, ly, label_width, 13, MakeColor(35, 31, 20, 235));
        UI_DrawString(lx + 4, ly + 3, UI_LEFT, va("%.30s", label));
    }
    if (HUD_CanResize())
        R_DrawFill32(r.x + r.width - 3, r.y + r.height - 3, 6, 6, MakeColor(255, 205, 80, 255));
    if (editor.dragging && !editor.resizing) {
        if (isfinite(editor.guide_x))
            R_DrawFill32(editor.guide_x, 0, 1, height, MakeColor(115, 218, 214, 220));
        if (isfinite(editor.guide_y))
            R_DrawFill32(0, editor.guide_y, width, 1, MakeColor(115, 218, 214, 220));
    }
    if (editor.snap) {
        R_DrawFill32(width / 2, 0, 1, height, MakeColor(130, 150, 170, 50));
        R_DrawFill32(0, height / 2, width, 1, MakeColor(130, 150, 170, 50));
    }
    R_SetScale(uis.scale);
    R_ClearColor();
    if (!HUD_ToolsVisible())
        return;
    float zoom = HUD_ToolsZoom();
    int tools_width = uis.width / zoom, tools_height = uis.height / zoom;
    R_SetScale(uis.scale / zoom);
    int section_height = editor.tools_section == HUD_SECTION_ARRANGE ? 100 : editor.tools_section == HUD_SECTION_RESET ? 60 : 0;
    int rows = Q_clip((tools_height - 163 - section_height) / 16, 1, 6);
    int selected_row = HUD_Row(editor.selected);
    editor.page_rows = rows;
    editor.list_first = Q_clip(editor.list_first, max(0, selected_row - rows + 1), selected_row);
    int shown = min(rows, HUD_Count() - editor.list_first);
    editor.tools.width = 296;
    editor.tools.height = 147 + section_height + shown * 16;
    HUD_ClampTools();
    HUD_ClearToolsHits();
    int tx = editor.tools.x, ty = editor.tools.y;
    bool dirty = false;
    for (size_t i = 0; i < setting_count; i++)
        dirty |= settings[i].changed;
    for (int i = 0; i < HUD_Limit(); i++)
        dirty |= editor.locked[i] != editor.original_locks[i];
    R_DrawFill32(tx, ty, editor.tools.width, editor.tools.height, MakeColor(12, 18, 24, 245));
    HUD_Outline(&editor.tools, MakeColor(60, 81, 95, 255));
    editor.tools_title = (vrect_t) { tx, ty, editor.tools.width, 24 };
    R_DrawFill32(tx, ty, editor.tools.width, 24, MakeColor(27, 40, 51, 255));
    R_DrawFill32(tx, ty, 2, 24, MakeColor(115, 218, 214, 255));
    UI_DrawString(tx + 10, ty + 8, UI_LEFT, "HUD EDITOR");
    HUD_Button(&editor.buttons[HUD_BUTTON_UNDO], tx + 126, ty + 3, 48, "Undo", editor.history_pos > 0);
    HUD_Button(&editor.buttons[HUD_BUTTON_REDO], tx + 180, ty + 3, 48, "Redo", editor.history_pos + 1 < editor.history_count);
    HUD_Button(&editor.buttons[HUD_BUTTON_HIDE_TOOLS], tx + 234, ty + 3, 54, "Hide", false);
    static const char *const categories[HUD_CATEGORY_COUNT] = {
        [HUD_CATEGORY_ALL] = "All",
        [HUD_CATEGORY_CLIENT] = "Client",
        [HUD_CATEGORY_SERVER] = "Server",
    };
    for (int i = 0; i < HUD_CATEGORY_COUNT; i++)
        HUD_Button(&editor.categories[i], tx + 8 + i * 96, ty + 29, 88,
                   categories[i], editor.category == i);
    for (int row = 0; row < shown; row++) {
        int i = HUD_Entry(row + editor.list_first), y = ty + 53 + row * 16;
        editor.rows[i] = (vrect_t) { tx + 6, y, 280, 16 };
        bool hover = HUD_Hit(&editor.rows[i], HUD_ToolsMouse(0), HUD_ToolsMouse(1));
        if (i == editor.selected || hover)
            R_DrawFill32(tx + 6, y, 280, 16, i == editor.selected ?
                         MakeColor(39, 68, 80, 255) : MakeColor(29, 41, 50, 255));
        if (i == editor.selected)
            R_DrawFill32(tx + 6, y, 2, 16, MakeColor(115, 218, 214, 255));
        const char *name = HUD_Name(i);
        UI_DrawString(tx + 10, y + 4, UI_LEFT,
                      strlen(name) > 29 ? va("%.26s...", name) : name);
        UI_DrawString(tx + 282, y + 4, UI_RIGHT, editor.locked[i] ? "LOCK" : HUD_Value(HUD_Enable(i)) ? "ON" : "OFF");
    }
    int bottom = ty + 53 + shown * 16;
    int track_height = shown * 16;
    int thumb_height = max(5, track_height * shown / HUD_Count());
    int thumb_y = ty + 53 + (HUD_Count() > shown ?
        (track_height - thumb_height) * editor.list_first / (HUD_Count() - shown) : 0);
    R_DrawFill32(tx + 288, ty + 53, 2, track_height, MakeColor(30, 44, 54, 255));
    R_DrawFill32(tx + 288, thumb_y, 2, thumb_height, MakeColor(101, 156, 169, 255));
    UI_DrawString(tx + 10, bottom + 4, UI_LEFT, "Wheel / Tab to select");
    UI_DrawString(tx + 282, bottom + 4, UI_RIGHT, va("%d / %d", selected_row + 1, HUD_Count()));
    HUD_Button(&editor.buttons[HUD_BUTTON_VISIBLE], tx + 8, bottom + 20, 88,
               HUD_Value(HUD_Enable(editor.selected)) ? "Visible" : "Hidden", HUD_Value(HUD_Enable(editor.selected)) != 0);
    HUD_Button(&editor.buttons[HUD_BUTTON_LOCK], tx + 104, bottom + 20, 88,
               editor.locked[editor.selected] ? "Locked" : "Lock", editor.locked[editor.selected]);
    HUD_Button(&editor.buttons[HUD_BUTTON_FOCUS], tx + 200, bottom + 20, 88, "Focus", editor.focus);
    HUD_Button(&editor.buttons[HUD_BUTTON_ARRANGE_SECTION], tx + 8, bottom + 44, 136,
               editor.tools_section == HUD_SECTION_ARRANGE ? "[-] Arrange" : "[+] Arrange", editor.tools_section == HUD_SECTION_ARRANGE);
    HUD_Button(&editor.buttons[HUD_BUTTON_RESET_SECTION], tx + 152, bottom + 44, 136,
               editor.tools_section == HUD_SECTION_RESET ? "[-] Reset" : "[+] Reset", editor.tools_section == HUD_SECTION_RESET);
    if (editor.tools_section == HUD_SECTION_ARRANGE) {
        UI_DrawString(tx + 10, bottom + 68, UI_LEFT, va("X:%d Y:%d  %dx%d", r.x, r.y, r.width, r.height));
        static const struct {
            hud_button_t button;
            const char *label;
        } arrows[] = {
            { HUD_BUTTON_MOVE_LEFT, "<" },
            { HUD_BUTTON_MOVE_RIGHT, ">" },
            { HUD_BUTTON_MOVE_UP, "^" },
            { HUD_BUTTON_MOVE_DOWN, "v" },
        };
        for (int i = 0; i < q_countof(arrows); i++)
            HUD_Button(&editor.buttons[arrows[i].button], tx + 8 + i * 36,
                       bottom + 82, 30, arrows[i].label, false);
        HUD_Button(&editor.buttons[HUD_BUTTON_SNAP], tx + 164, bottom + 82, 124,
                   editor.snap ? "Snap: ON" : "Snap: OFF", editor.snap);
        UI_DrawString(tx + 10, bottom + 108, UI_LEFT, "Align to screen");
        static const struct {
            hud_button_t button;
            const char *label;
        } align[] = {
            { HUD_BUTTON_ALIGN_LEFT, "Left" },
            { HUD_BUTTON_ALIGN_HCENTER, "H-center" },
            { HUD_BUTTON_ALIGN_RIGHT, "Right" },
            { HUD_BUTTON_ALIGN_TOP, "Top" },
            { HUD_BUTTON_ALIGN_VCENTER, "V-center" },
            { HUD_BUTTON_ALIGN_BOTTOM, "Bottom" },
        };
        for (int i = 0; i < q_countof(align); i++)
            HUD_Button(&editor.buttons[align[i].button], tx + 8 + (i % 3) * 96,
                       bottom + 120 + (i / 3) * 22, 88, align[i].label, false);
    } else if (editor.tools_section == HUD_SECTION_RESET) {
        HUD_Button(&editor.buttons[HUD_BUTTON_RESET_SELECTED], tx + 8, bottom + 68, 280, "Reset selected element", false);
        HUD_Button(&editor.buttons[HUD_BUTTON_RESET_ALL], tx + 8, bottom + 92, 280, "Restore all defaults", false);
        UI_DrawString(tx + 10, bottom + 116, UI_LEFT, "Undo or Cancel can restore edits.");
    }
    R_DrawFill32(tx + 8, bottom + 64 + section_height, 280, 1, MakeColor(48, 64, 77, 255));
    HUD_Button(&editor.buttons[HUD_BUTTON_APPLY], tx + 8, bottom + 70 + section_height, 136, dirty ? "Apply *" : "Apply", true);
    HUD_Button(&editor.buttons[HUD_BUTTON_CANCEL], tx + 152, bottom + 70 + section_height, 136, "Cancel", false);

    static const char *const tips[HUD_BUTTON_COUNT] = {
        [HUD_BUTTON_APPLY] = "Apply changes and close [Enter]",
        [HUD_BUTTON_CANCEL] = "Discard changes and close [Esc]",
        [HUD_BUTTON_RESET_SELECTED] = "Reset this element only [R]",
        [HUD_BUTTON_SNAP] = "Snap to screen and HUD edges [S]",
        [HUD_BUTTON_VISIBLE] = "Toggle gameplay visibility [V]",
        [HUD_BUTTON_HIDE_TOOLS] = "Hide tools; press H to restore",
        [HUD_BUTTON_PREVIOUS] = "Previous element",
        [HUD_BUTTON_NEXT] = "Next element",
        [HUD_BUTTON_MOVE_LEFT] = "Move left; Shift moves 10 units",
        [HUD_BUTTON_MOVE_RIGHT] = "Move right; Shift moves 10 units",
        [HUD_BUTTON_MOVE_UP] = "Move up; Shift moves 10 units",
        [HUD_BUTTON_MOVE_DOWN] = "Move down; Shift moves 10 units",
        [HUD_BUTTON_RESET_ALL] = "Reset unlocked HUD [Shift+R]",
        [HUD_BUTTON_UNDO] = "Undo last edit [Ctrl+Z]",
        [HUD_BUTTON_REDO] = "Redo last edit [Ctrl+Y]",
        [HUD_BUTTON_LOCK] = "Lock/unlock selected element [L]",
        [HUD_BUTTON_FOCUS] = "Show selected preview only [F]",
        [HUD_BUTTON_ALIGN_LEFT] = "Align left edge",
        [HUD_BUTTON_ALIGN_HCENTER] = "Center horizontally",
        [HUD_BUTTON_ALIGN_RIGHT] = "Align right edge",
        [HUD_BUTTON_ALIGN_TOP] = "Align top edge",
        [HUD_BUTTON_ALIGN_VCENTER] = "Center vertically",
        [HUD_BUTTON_ALIGN_BOTTOM] = "Align bottom edge",
        [HUD_BUTTON_ARRANGE_SECTION] = "Position, snapping and alignment [A]",
        [HUD_BUTTON_RESET_SECTION] = "Restore HUD defaults [D]",
    };
    const char *tip = NULL;
    if (HUD_Hit(&editor.tools_title, HUD_ToolsMouse(0), HUD_ToolsMouse(1)))
        tip = "Drag this header to move the panel";
    for (int i = 0; i < q_countof(tips); i++)
        if (HUD_Hit(&editor.buttons[i], HUD_ToolsMouse(0), HUD_ToolsMouse(1)))
            tip = tips[i];
    for (int row = 0; row < shown; row++) {
        int i = HUD_Entry(row + editor.list_first);
        if (strlen(HUD_Name(i)) > 29 && HUD_Hit(&editor.rows[i], HUD_ToolsMouse(0), HUD_ToolsMouse(1)))
            tip = HUD_Name(i);
    }
    vrect_t details = { tx + 8, bottom + 66, 280, editor.tools_section == HUD_SECTION_ARRANGE ? 14 : 0 };
    if (HUD_Hit(&details, HUD_ToolsMouse(0), HUD_ToolsMouse(1)))
        tip = HUD_Locked(editor.selected) ? "Locked; unlock to edit [L]" :
              HUD_CanResize() ? "Drag corner / Ctrl+arrows: resize" : "Drag or use arrows to move";
    if (tip) {
        int columns = max(1, (tools_width - 16) / CHAR_WIDTH);
        int length = strlen(tip), lines = (length + columns - 1) / columns;
        int tw = min(length, columns) * CHAR_WIDTH + 12, th = lines * CHAR_HEIGHT + 10;
        int hx = Q_clip(tx, 0, max(0, tools_width - tw));
        int hy = ty + editor.tools.height + 4;
        if (hy + th > tools_height)
            hy = max(0, ty - th - 4);
        R_DrawFill32(hx, hy, tw, th, MakeColor(27, 40, 51, 250));
        for (int line = 0; line < lines; line++)
            UI_DrawString(hx + 6, hy + 5 + line * CHAR_HEIGHT, UI_LEFT, va("%.*s", columns, tip + line * columns));
    }
    R_SetScale(uis.scale);
}

static void HUD_Open_f(void)
{
    if (!uis.initialized)
        return;
    if (HUD_EditorActive()) {
        UI_PopMenu();
        return;
    }
    SCR_HudEditorPrepare();
    memcpy(settings, native_settings, sizeof(native_settings));
    setting_count = HUD_NATIVE_SETTINGS;
    for (int i = 0; i < HUD_LayoutCount(); i++) {
        const hud_layout_item_t *item = HUD_LayoutItem(i);
        cvar_t *vars[] = { item->x, item->y, item->visible };
        for (int j = 0; j < 3; j++)
            settings[setting_count++] = (hud_setting_t) { vars[j]->name,
                HUD_EDIT_LAYOUT_FIRST + i, j == 2 ? 0 : -16000, j == 2 ? 1 : 16000 };
    }
    for (size_t i = 0; i < setting_count; i++) {
        hud_setting_t *s = &settings[i];
        s->var = Cvar_FindVar(s->name);
        if (!s->var) {
            Com_Printf("HUD editor: missing %s\n", s->name);
            return;
        }
        s->original = s->var->value;
        s->value = HudEdit_Clamp(s->original, s->low, s->high);
        s->changed = false;
    }
    Key_ClearStates();
    editor.open = true;
    editor.dragging = false;
    editor.selected = HUD_EDIT_UPS;
    editor.list_first = 0;
    editor.snap = true;
    editor.hide_tools = false;
    editor.dragging_tools = false;
    editor.category = HUD_CATEGORY_ALL;
    editor.focus = false;
    editor.tools_section = HUD_SECTION_NONE;
    editor.page_rows = 6;
    HUD_ClearToolsHits();
    editor.history_count = editor.history_pos = 0;
    editor.guide_x = editor.guide_y = NAN;
    memset(editor.lock_vars, 0, sizeof(editor.lock_vars));
    memset(editor.locked, 0, sizeof(editor.locked));
    for (int i = 0; i < HUD_Limit(); i++) {
        if (!HUD_Editable(i))
            continue;
        const char *key = i < HUD_EDIT_LAYOUT_FIRST ? native_widgets[i].key : HUD_LayoutItem(i - HUD_EDIT_LAYOUT_FIRST)->key;
        char name[MAX_QPATH];
        Q_snprintf(name, sizeof(name), "hud_lock_%s", key);
        editor.lock_vars[i] = Cvar_Get(name, "0", CVAR_ARCHIVE);
        editor.locked[i] = editor.lock_vars[i]->integer != 0;
    }
    memcpy(editor.original_locks, editor.locked, sizeof(editor.locked));
    editor.tools = (vrect_t) { 8, 8, 296, 243 };
    editor.network_mode = (int)HUD_Value("sh_netmeter");
    if (!editor.network_mode)
        editor.network_mode = 3;
    editor.servercount = cl.servercount;
    editor.scale = scr.hud_scale > 0 ? scr.hud_scale : 1;
    UI_PushMenu(&editor.menu);
    HUD_EditorMouse(uis.mouseCoords[0] / uis.scale, uis.mouseCoords[1] / uis.scale);
}

void HUD_EditorInit(void)
{
    memset(&editor, 0, sizeof(editor));
    editor.menu.name = "hud_editor";
    editor.menu.transparent = true;
    editor.menu.draw = HUD_Draw;
    editor.menu.pop = HUD_Pop;
    editor.menu.size = HUD_Size;
    Cmd_AddCommand("hud_edit", HUD_Open_f);
}

void HUD_EditorShutdown(void)
{
    editor.open = editor.preview = editor.dragging = false;
    Cmd_RemoveCommand("hud_edit");
}
