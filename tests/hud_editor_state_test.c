/* Exercise the actual editor controller with cvar/input/render boundaries stubbed. */
#undef NDEBUG
#include "../src/client/ui/hud_editor.c"
#include <assert.h>

uiStatic_t uis;
client_static_t cls;
static cvar_t vars[q_countof(settings)];
static int held[256], writes, extra_count;
static cvar_t locks[HUD_EDIT_COUNT];
static char lock_names[HUD_EDIT_COUNT][MAX_QPATH];
static int lock_count;
static hud_layout_item_t layout_item;
static cvar_t efficiency_style;
cvar_t *cl_strafehelperEffStyle = &efficiency_style;

int Key_IsDown(int key) { return held[key]; }
void UI_PopMenu(void) { HUD_Pop(&editor.menu); uis.activeMenu = NULL; }
void Cvar_SetValue(cvar_t *var, float value, from_t from)
{
    assert(from == FROM_CONSOLE);
    var->value = value;
    writes++;
}
float Cvar_ClampValue(cvar_t *var, float low, float high)
{
    return HudEdit_Clamp(var->value, low, high);
}

static void setup(void)
{
    memset(&editor, 0, sizeof(editor));
    memset(&uis, 0, sizeof(uis));
    memset(held, 0, sizeof(held));
    writes = 0; extra_count = 0; setting_count = HUD_NATIVE_SETTINGS;
    memcpy(settings, native_settings, sizeof(native_settings));
    memset(locks, 0, sizeof(locks));
    lock_count = 0;
    efficiency_style.string = "bar";
    for (size_t i = 0; i < setting_count; i++) {
        vars[i] = (cvar_t) { 0 };
        vars[i].name = (char *)native_settings[i].name;
        vars[i].default_string = "0";
        settings[i].var = &vars[i];
        settings[i].original = settings[i].value = 0;
        settings[i].changed = false;
    }
    editor.open = true;
    editor.page_rows = 6;
    editor.scale = 0.5f;
    editor.width = 640; editor.height = 360;
    editor.bounds[HUD_EDIT_UPS] = (vrect_t) { 300, 170, 24, 8 };
    uis.activeMenu = &editor.menu;
    cls.active = ACT_ACTIVATED;
}

static void setup_layout(void)
{
    setup();
    extra_count = 1;
    const char *names[] = { "hud_timer_x", "hud_timer_y", "hud_timer_visible" };
    for (int i = 0; i < 3; i++) {
        vars[HUD_NATIVE_SETTINGS + i] = (cvar_t) { 0 };
        vars[HUD_NATIVE_SETTINGS + i].name = (char *)names[i];
        vars[HUD_NATIVE_SETTINGS + i].default_string = i == 2 ? "1" : "0";
        vars[HUD_NATIVE_SETTINGS + i].value = i == 2;
        settings[HUD_NATIVE_SETTINGS + i] = (hud_setting_t) { names[i], HUD_EDIT_LAYOUT_FIRST,
            i == 2 ? 0 : -16000, i == 2 ? 1 : 16000, &vars[HUD_NATIVE_SETTINGS + i], i == 2, i == 2, false };
    }
    setting_count = HUD_NATIVE_SETTINGS + 3;
    layout_item = (hud_layout_item_t) { "timer", "Run timer", &vars[HUD_NATIVE_SETTINGS], &vars[HUD_NATIVE_SETTINGS + 1], &vars[HUD_NATIVE_SETTINGS + 2] };
    editor.selected = HUD_EDIT_LAYOUT_FIRST;
    editor.bounds[HUD_EDIT_LAYOUT_FIRST] = (vrect_t) { 500, 300, 94, 32 };
}

/* Exercise every command slot, including Previous/Next which have no buttons. */
static void test_button_commands(void)
{
    assert(HUD_BUTTON_COUNT == 25);
    for (int id = 0; id < HUD_BUTTON_COUNT; id++) {
        hud_button_t button = (hud_button_t)id;
        setup();
        hud_setting_t *x = HUD_Setting("sh_ups_x");
        x->original = x->value = x->var->value = 7;
        editor.buttons[HUD_BUTTON_ALIGN_LEFT] = (vrect_t) { 1, 1, 10, 10 };

        if (button == HUD_BUTTON_APPLY || button == HUD_BUTTON_CANCEL)
            HUD_Set("sh_ups_x", 9);
        if (button == HUD_BUTTON_PREVIOUS)
            editor.selected = HUD_EDIT_STRAFE;
        if (button == HUD_BUTTON_UNDO || button == HUD_BUTTON_REDO)
            HUD_Nudge(1, 0, false);
        if (button == HUD_BUTTON_REDO)
            HUD_Undo();

        HUD_Command(button);
        switch (button) {
        case HUD_BUTTON_APPLY:
            assert(!editor.open && writes == 1 && x->var->value == 9);
            break;
        case HUD_BUTTON_CANCEL:
            assert(!editor.open && x->var->value == 7);
            break;
        case HUD_BUTTON_RESET_SELECTED:
        case HUD_BUTTON_RESET_ALL:
            assert(x->value == 0 && x->var->value == 7);
            break;
        case HUD_BUTTON_SNAP:
            assert(editor.snap && editor.history_count == 0);
            break;
        case HUD_BUTTON_VISIBLE:
            assert(HUD_Value("sh_ups") == 1 && HUD_Setting("sh_ups")->var->value == 0);
            break;
        case HUD_BUTTON_HIDE_TOOLS:
            assert(editor.hide_tools && editor.history_count == 0);
            break;
        case HUD_BUTTON_PREVIOUS:
            assert(editor.selected == HUD_EDIT_UPS && editor.history_count == 0);
            break;
        case HUD_BUTTON_NEXT:
            assert(editor.selected == HUD_EDIT_STRAFE && editor.history_count == 0);
            break;
        case HUD_BUTTON_MOVE_LEFT:
            assert(x->value == 6);
            break;
        case HUD_BUTTON_MOVE_RIGHT:
            assert(x->value == 8);
            break;
        case HUD_BUTTON_MOVE_UP:
            assert(HUD_Value("sh_ups_y") == -1);
            break;
        case HUD_BUTTON_MOVE_DOWN:
            assert(HUD_Value("sh_ups_y") == 1);
            break;
        case HUD_BUTTON_UNDO:
            assert(x->value == 7 && editor.history_pos == 0);
            break;
        case HUD_BUTTON_REDO:
            assert(x->value == 8 && editor.history_pos == 1);
            break;
        case HUD_BUTTON_LOCK:
            assert(editor.locked[HUD_EDIT_UPS] && editor.history_pos == 1);
            break;
        case HUD_BUTTON_FOCUS:
            assert(editor.focus && editor.history_count == 0);
            break;
        case HUD_BUTTON_ALIGN_LEFT:
            assert(x->value == -293);
            break;
        case HUD_BUTTON_ALIGN_HCENTER:
            assert(x->value == 15);
            break;
        case HUD_BUTTON_ALIGN_RIGHT:
            assert(x->value == 323);
            break;
        case HUD_BUTTON_ALIGN_TOP:
            assert(HUD_Value("sh_ups_y") == -170);
            break;
        case HUD_BUTTON_ALIGN_VCENTER:
            assert(HUD_Value("sh_ups_y") == 6);
            break;
        case HUD_BUTTON_ALIGN_BOTTOM:
            assert(HUD_Value("sh_ups_y") == 182);
            break;
        case HUD_BUTTON_ARRANGE_SECTION:
            assert(editor.tools_section == HUD_SECTION_ARRANGE);
            assert(!editor.buttons[HUD_BUTTON_ALIGN_LEFT].width && editor.history_count == 0);
            break;
        case HUD_BUTTON_RESET_SECTION:
            assert(editor.tools_section == HUD_SECTION_RESET);
            assert(!editor.buttons[HUD_BUTTON_ALIGN_LEFT].width && editor.history_count == 0);
            break;
        default:
            assert(!"Unverified HUD command");
        }
        if (button != HUD_BUTTON_APPLY)
            assert(writes == 0);
    }
}

static void test_native_settings(void)
{
    setup_layout();
    vars[HUD_NATIVE_SETTINGS - 1].value = 37;
    uis.initialized = true;
    uis.scale = 1;
    editor.open = false;
    uis.activeMenu = NULL;
    HUD_Open_f();
    assert(HUD_EditorActive());
    assert(setting_count == HUD_NATIVE_SETTINGS + 3);
    assert(HUD_Setting("sh_histogram_height")->var == &vars[HUD_NATIVE_SETTINGS - 1]);
    assert(HUD_Value("sh_histogram_height") == 37);
    assert(HUD_Setting("hud_timer_x")->owner == HUD_EDIT_LAYOUT_FIRST);
    for (size_t i = 0; i < q_countof(native_settings); i++) {
        assert(!strcmp(settings[i].name, native_settings[i].name));
        assert(settings[i].var == &vars[i]);
    }
    const char *expected_names[] = {
        "Speed (UPS)", "Strafe helper", "Strafe efficiency", "Network monitor"
    };
    const char *expected_enables[] = { "sh_ups", "sh_draw", "sh_efficiency", "sh_netmeter" };
    for (int id = 0; id < HUD_EDIT_LAYOUT_FIRST; id++) {
        assert(!strcmp(HUD_Name(id), expected_names[id]));
        assert(!strcmp(HUD_Enable(id), expected_enables[id]));
    }
    assert(HUD_EDIT_UPS == 0 && HUD_EDIT_STRAFE == 1);
    assert(HUD_EDIT_EFFICIENCY == 2 && HUD_EDIT_NETWORK == 3 && HUD_EDIT_LAYOUT_FIRST == 4);

    HUD_Set("sh_ups_x", 12);
    HUD_Command(HUD_BUTTON_APPLY);
    assert(writes == 1);
    HUD_Open_f();
    assert(HUD_EditorActive() && HUD_Value("sh_ups_x") == 12);
    HUD_Command(HUD_BUTTON_CANCEL);
    assert(writes == 1);
}

static void open_with_clamped_height(void)
{
    setup();
    for (size_t i = 0; i < setting_count; i++)
        vars[i].value = HudEdit_Clamp(0, settings[i].low, settings[i].high);
    HUD_Setting("sh_height")->var->value = 2;
    uis.initialized = true; uis.scale = 1;
    editor.open = false; uis.activeMenu = NULL;
    HUD_Open_f();
    assert(HUD_Value("sh_height") == 4 && !HUD_Setting("sh_height")->changed);
}

static void test_history_clamped_settings(void)
{
    /* Undo and Redo must not turn preview clamping into an unrelated edit. */
    for (int redo = 0; redo < 2; redo++) {
        open_with_clamped_height();
        HUD_EditorKey(K_RIGHTARROW, true);
        held[K_CTRL] = 1;
        HUD_EditorKey('z', true);
        if (redo)
            HUD_EditorKey('y', true);
        held[K_CTRL] = 0;
        assert(!HUD_Setting("sh_height")->changed);
        assert(HUD_Value("sh_ups_x") == redo);
        HUD_EditorKey(K_ENTER, true);
        assert(HUD_Setting("sh_height")->var->value == 2);
        assert(HUD_Setting("sh_ups_x")->var->value == redo && writes == redo);
    }

    /* Reset can intentionally accept a clamped value without changing the draft. */
    open_with_clamped_height();
    editor.selected = HUD_EDIT_STRAFE;
    HUD_Command(HUD_BUTTON_RESET_SELECTED);
    assert(HUD_Value("sh_height") == 4 && HUD_Setting("sh_height")->changed);
    assert(editor.history_count == 2);
    HUD_Undo();
    assert(HUD_Value("sh_height") == 4 && !HUD_Setting("sh_height")->changed);
    HUD_Redo();
    assert(HUD_Value("sh_height") == 4 && HUD_Setting("sh_height")->changed);
    HUD_Command(HUD_BUTTON_APPLY);
    assert(HUD_Setting("sh_height")->var->value == 4 && writes == 1);
}

static void setup_efficiency(const char *style)
{
    setup();
    efficiency_style.string = (char *)style;
    const char *names[] = { "sh_efficiency_width", "sh_efficiency_height", "sh_efficiency_text_scale" };
    const float values[] = { 80, 4, 1 };
    for (size_t i = 0; i < q_countof(names); i++) {
        hud_setting_t *s = HUD_Setting(names[i]);
        s->original = s->value = s->var->value = values[i];
    }
    editor.selected = HUD_EDIT_EFFICIENCY;
    editor.bounds[HUD_EDIT_EFFICIENCY] = (vrect_t) { 300, 200, 24, 8 };
}

static void test_efficiency_resize(void)
{
    const char *text_styles[] = { "text", "TeXt", "2" };
    for (size_t i = 0; i < q_countof(text_styles); i++) {
        for (int apply = 0; apply < 2; apply++) {
            setup_efficiency(text_styles[i]);
            held[K_CTRL] = 1;
            HUD_EditorKey(K_RIGHTARROW, true);
            float scale = HUD_Value("sh_efficiency_text_scale");
            assert(scale > 1 && writes == 0);
            HUD_EditorKey(K_DOWNARROW, true); /* Text scaling is horizontal. */
            assert(HUD_Value("sh_efficiency_text_scale") == scale);
            assert(HUD_Value("sh_efficiency_width") == 80 && !HUD_Setting("sh_efficiency_width")->changed);
            assert(HUD_Value("sh_efficiency_height") == 4 && !HUD_Setting("sh_efficiency_height")->changed);
            HUD_EditorKey('z', true);
            assert(HUD_Value("sh_efficiency_text_scale") == 1 && !HUD_Setting("sh_efficiency_text_scale")->changed);
            HUD_EditorKey('y', true);
            assert(HUD_Value("sh_efficiency_text_scale") == scale && writes == 0);
            held[K_CTRL] = 0;
            HUD_EditorKey(apply ? K_ENTER : K_ESCAPE, true);
            assert(HUD_Setting("sh_efficiency_text_scale")->var->value == (apply ? scale : 1));
            assert(writes == apply);
        }
    }

    /* Multiple queued mouse events resize from the initial text bounds. */
    setup_efficiency("text");
    editor.mouse_x = 324; editor.mouse_y = 208; held[K_MOUSE1] = 1;
    HUD_EditorKey(K_MOUSE1, true);
    assert(editor.dragging && editor.resizing);
    HUD_EditorMouse(672, 436);
    assert(HUD_Value("sh_efficiency_text_scale") == 1.5f);
    HUD_EditorMouse(696, 456);
    assert(HUD_Value("sh_efficiency_text_scale") == 2);
    HUD_EditorKey(K_MOUSE1, false); held[K_MOUSE1] = 0;
    assert(editor.history_count == 2 && writes == 0);
    assert(HUD_Value("sh_efficiency_width") == 80 && HUD_Value("sh_efficiency_height") == 4);
    HUD_Undo(); assert(HUD_Value("sh_efficiency_text_scale") == 1);
    HUD_Redo(); assert(HUD_Value("sh_efficiency_text_scale") == 2);
    HUD_Command(HUD_BUTTON_APPLY);
    assert(HUD_Setting("sh_efficiency_text_scale")->var->value == 2 && writes == 1);

    setup_efficiency("text");
    HUD_Nudge(100000, 0, true);
    assert(HUD_Value("sh_efficiency_text_scale") == SH_EFFICIENCY_TEXT_SCALE_MAX);
    HUD_Undo(); HUD_Nudge(-100000, 0, true);
    assert(HUD_Value("sh_efficiency_text_scale") == SH_EFFICIENCY_TEXT_SCALE_MIN);

    const char *bar_styles[] = { "bar", "both", "1", "3" };
    for (size_t i = 0; i < q_countof(bar_styles); i++) {
        setup_efficiency(bar_styles[i]);
        HUD_Nudge(1, 1, true);
        assert(HUD_Value("sh_efficiency_width") == 82 && HUD_Value("sh_efficiency_height") == 5);
        assert(HUD_Value("sh_efficiency_text_scale") == 1 && !HUD_Setting("sh_efficiency_text_scale")->changed);
    }
}

int main(void)
{
    test_button_commands();
    test_native_settings();
    test_history_clamped_settings();
    test_efficiency_resize();
    /* Opening preserves native string keys after compacting the widget list. */
    setup();
    uis.initialized = true; uis.scale = 1;
    editor.open = false; uis.activeMenu = NULL;
    HUD_Open_f();
    assert(HUD_EditorActive() && setting_count == HUD_NATIVE_SETTINGS);
    const char *expected_locks[] = {
        "hud_lock_ups", "hud_lock_strafe", "hud_lock_efficiency", "hud_lock_network"
    };
    assert(lock_count == q_countof(expected_locks) && lock_count == HUD_EDIT_LAYOUT_FIRST);
    for (int i = 0; i < lock_count; i++)
        assert(!strcmp(lock_names[i], expected_locks[i]));
    assert(editor.locked[HUD_EDIT_NETWORK] && !editor.locked[HUD_EDIT_EFFICIENCY]);
    assert(!strcmp(HUD_Name(HUD_EDIT_NETWORK), "Network monitor"));
    assert(!strcmp(HUD_Enable(HUD_EDIT_NETWORK), "sh_netmeter"));
    assert(HUD_Setting("sh_lagometer_x")->owner == HUD_EDIT_NETWORK);
    HUD_EditorKey(K_ESCAPE, true);
    assert(!editor.open && writes == 0);
    setup();
    HUD_EditorKey(K_RIGHTARROW, true);
    assert(HUD_Value("sh_ups_x") == 1);
    assert(HUD_Setting("sh_ups_x")->var->value == 0 && writes == 0);
    editor.preview = true;
    assert(HUD_EditorValue(HUD_Setting("sh_ups_x")->var) == 1);
    editor.preview = false;
    assert(HUD_EditorValue(HUD_Setting("sh_ups_x")->var) == 0);
    HUD_EditorKey(K_ESCAPE, true);
    assert(!editor.open && writes == 0);

    setup();
    HUD_EditorKey(K_RIGHTARROW, true);
    HUD_EditorKey(K_ENTER, true);
    assert(writes == 1 && HUD_Setting("sh_ups_x")->var->value == 1);

    setup();
    /* Opening clamps the preview, but untouched values must never be saved. */
    HUD_Setting("sh_efficiency_width")->original = 9000;
    HUD_Setting("sh_efficiency_width")->value = 4000;
    editor.mouse_x = 310; editor.mouse_y = 174;
    held[K_MOUSE1] = 1;
    HUD_EditorKey(K_MOUSE1, true);
    assert(editor.dragging);
    HUD_EditorMouse(640, 348); /* +10 HUD units */
    assert(HUD_Value("sh_ups_x") == 10);
    HUD_EditorMouse(660, 348); /* +20 total, without an intervening draw */
    assert(HUD_Value("sh_ups_x") == 20 && writes == 0);
    assert(!HUD_Setting("sh_efficiency_width")->changed);
    HUD_EditorKey(K_MOUSE1, false);
    assert(!editor.dragging);
    HUD_EditorMouse(680, 348);
    assert(HUD_Value("sh_ups_x") == 20);

    setup();
    editor.dragging = true;
    held[K_MOUSE1] = 1;
    cls.active = ACT_MINIMIZED;
    HUD_EditorMouse(640, 348);
    assert(!editor.dragging && writes == 0);

    setup();
    editor.selected = HUD_EDIT_NETWORK;
    editor.network_mode = 1;
    HUD_StartResize(); HUD_Resize(50, 50);
    for (size_t i = 0; i < setting_count; i++)
        assert(!settings[i].changed); /* fixed lagometer size */

    setup();
    editor.preview = true;
    HUD_EditorBounds(HUD_EDIT_EFFICIENCY, 10, 20, 100, 6);
    HUD_EditorBounds(HUD_EDIT_EFFICIENCY, 30, 30, 60, 9);
    assert(editor.bounds[HUD_EDIT_EFFICIENCY].x == 10);
    assert(editor.bounds[HUD_EDIT_EFFICIENCY].width == 100);
    assert(editor.bounds[HUD_EDIT_EFFICIENCY].height == 19);
    setup_layout();
    HUD_EditorKey(K_LEFTARROW, true);
    assert(HUD_Value("hud_timer_x") == -1 && vars[HUD_NATIVE_SETTINGS].value == 0 && writes == 0);
    HUD_EditorKey(K_ENTER, true);
    assert(vars[HUD_NATIVE_SETTINGS].value == -1 && writes == 1);

    setup_layout();
    HUD_EditorKey('v', true);
    assert(HUD_Value("hud_timer_visible") == 0 && vars[HUD_NATIVE_SETTINGS + 2].value == 1);
    HUD_EditorKey(K_ESCAPE, true);
    assert(writes == 0);

    setup_layout();
    HUD_EditorKey(K_LEFTARROW, true);
    HUD_EditorKey('r', true);
    HUD_EditorKey(K_ENTER, true);
    assert(writes == 0);

    setup_layout();
    assert(!HUD_CanResize());
    HUD_EditorKey(K_TAB, true);
    assert(editor.selected == HUD_EDIT_UPS);
    HUD_EditorKey(K_PGDN, true);
    assert(editor.selected == HUD_EDIT_LAYOUT_FIRST);
    /* Moving the toolbox uses menu coordinates and never edits a HUD setting. */
    setup();
    uis.width = 800; uis.height = 450; uis.scale = .25f;
    editor.tools = (vrect_t) { 8, 8, 230, 190 };
    editor.tools_title = (vrect_t) { 8, 8, 230, 17 };
    uis.mouseCoords[0] = 20; uis.mouseCoords[1] = 14;
    held[K_MOUSE1] = 1;
    HUD_EditorKey(K_MOUSE1, true);
    assert(editor.dragging_tools && !editor.dragging);
    HUD_EditorMouse(160, 96); /* UI cursor (40,24), toolbox zoom 1, HUD scale .5. */
    assert(editor.tools.x == 28 && editor.tools.y == 18);
    HUD_EditorMouse(200, 112); /* More events before draw must remain absolute. */
    assert(editor.tools.x == 38 && editor.tools.y == 22);
    HUD_EditorMouse(9999, 9999);
    assert(editor.tools.x == 570 && editor.tools.y == 260);
    HUD_EditorKey(K_MOUSE1, false);
    assert(!editor.dragging_tools && writes == 0);
    for (int i = 0; i < setting_count; i++) assert(!settings[i].changed);

    /* Toolbox padding captures clicks rather than dragging a covered overlay. */
    uis.mouseCoords[0] = 600; uis.mouseCoords[1] = 420;
    editor.mouse_x = 310; editor.mouse_y = 174;
    HUD_EditorKey(K_MOUSE1, true);
    assert(!editor.dragging && !editor.dragging_tools);

    setup();
    assert(HUD_ToolsVisible());
    editor.dragging = true;
    assert(!HUD_ToolsVisible());
    HUD_EditorKey(K_MOUSE1, false);
    assert(HUD_ToolsVisible());
    HUD_EditorKey('h', true);
    assert(!HUD_ToolsVisible());
    HUD_EditorKey('h', true);
    assert(HUD_ToolsVisible());
    editor.dragging_tools = true; held[K_MOUSE1] = 1;
    cls.active = ACT_MINIMIZED;
    HUD_EditorMouse(20, 20);
    assert(!editor.dragging_tools);
    /* Navigation skips removed groups and still reaches custom draw objects. */
    setup();
    extra_count = HL_BUILTIN_COUNT + 1;
    assert(HUD_Count() == HUD_EDIT_LAYOUT_FIRST + extra_count - 8);
    int visited = 1;
    do {
        HUD_EditorKey(K_TAB, true);
        assert(HUD_Editable(editor.selected));
        if (editor.selected != HUD_EDIT_UPS) visited++;
    } while (editor.selected != HUD_EDIT_UPS);
    assert(visited == HUD_Count());
    held[K_SHIFT] = 1;
    HUD_EditorKey(K_TAB, true);
    assert(editor.selected == HUD_EDIT_LAYOUT_FIRST + HL_BUILTIN_COUNT);
    held[K_SHIFT] = 0;
    HUD_EditorKey(K_MWHEELUP, true);
    assert(editor.selected == HUD_EDIT_LAYOUT_FIRST + HL_MOVE_FPS);
    editor.selected = HUD_EDIT_LAYOUT_FIRST + HL_TIMELEFT;
    HUD_EditorKey(K_MWHEELDOWN, true);
    assert(editor.selected == HUD_EDIT_LAYOUT_FIRST + HL_CHAT);
    HUD_EditorKey(K_PGDN, true);
    assert(editor.selected == HUD_EDIT_LAYOUT_FIRST + HL_TURTLE);
    /* Removed groups cannot intercept a click even with cached bounds. */
    editor.hide_tools = true;
    editor.mouse_x = 100; editor.mouse_y = 100;
    editor.bounds[HUD_EDIT_LAYOUT_FIRST + HL_CROSSHAIR] = (vrect_t) { 90, 90, 20, 20 };
    HUD_EditorKey(K_MOUSE1, true);
    assert(!editor.dragging);
    /* Categories contain the right stable IDs and never mutate settings. */
    setup(); extra_count = HL_BUILTIN_COUNT + 1;
    HUD_EditorKey('2', true);
    assert(editor.category == HUD_CATEGORY_CLIENT && HUD_Count() == 16);
    assert(HUD_ListItem(HUD_EDIT_NETWORK));
    assert(HUD_ListItem(HUD_EDIT_LAYOUT_FIRST + HL_CHAT));
    held[K_SHIFT] = 1; HUD_EditorKey(K_TAB, true); held[K_SHIFT] = 0;
    assert(editor.selected == HUD_EDIT_LAYOUT_FIRST + HL_BUILTIN_COUNT);
    HUD_EditorKey('3', true);
    assert(editor.category == HUD_CATEGORY_SERVER && HUD_Count() == HL_SERVER_COUNT + 2);
    assert(editor.selected == HUD_EDIT_LAYOUT_FIRST + HL_HEALTH);
    assert(HUD_ListItem(HUD_EDIT_LAYOUT_FIRST + HL_SCOREBOARD));
    assert(HUD_ListItem(HUD_EDIT_LAYOUT_FIRST + HL_OTHER_STATUS));
    HUD_EditorKey('4', true); /* Old fourth tab shortcut does nothing. */
    assert(editor.category == HUD_CATEGORY_SERVER);
    HUD_EditorKey('1', true);
    assert(editor.selected == HUD_EDIT_LAYOUT_FIRST + HL_HEALTH && writes == 0);
    /* Selecting an empty category falls back to All. */
    setup(); HUD_EditorKey('3', true);
    assert(editor.category == HUD_CATEGORY_ALL && HUD_Count() == 4);
    /* Paging follows the visible row count, including a short-window list. */
    setup(); extra_count = HL_BUILTIN_COUNT;
    editor.page_rows = 3;
    HUD_EditorKey(K_PGDN, true);
    assert(editor.selected == HUD_EDIT_NETWORK);
    HUD_EditorKey(K_PGUP, true);
    assert(editor.selected == HUD_EDIT_UPS);
    editor.page_rows = 0;
    HUD_EditorKey(K_PGDN, true);
    assert(editor.selected == HUD_EDIT_STRAFE);

    /* Section changes invalidate cached controls before another queued click. */
    setup(); HUD_EditorKey(K_RIGHTARROW, true);
    int section_history = editor.history_pos, section_count = editor.history_count;
    assert(editor.tools_section == HUD_SECTION_NONE);
    HUD_EditorKey('a', true);
    assert(editor.tools_section == HUD_SECTION_ARRANGE);
    editor.buttons[HUD_BUTTON_ALIGN_LEFT] = (vrect_t) { 10, 10, 48, 18 };
    editor.rows[HUD_EDIT_EFFICIENCY] = (vrect_t) { 60, 10, 100, 16 };
    editor.categories[HUD_CATEGORY_CLIENT] = (vrect_t) { 170, 10, 80, 18 };
    editor.tools_title = (vrect_t) { 10, 40, 230, 22 };
    HUD_Command(HUD_BUTTON_ARRANGE_SECTION); /* Collapse Arrange without waiting for a draw. */
    assert(editor.tools_section == HUD_SECTION_NONE);
    for (size_t i = 0; i < q_countof(editor.buttons); i++) assert(!editor.buttons[i].width);
    for (size_t i = 0; i < q_countof(editor.rows); i++) assert(!editor.rows[i].width);
    for (size_t i = 0; i < q_countof(editor.categories); i++) assert(!editor.categories[i].width);
    assert(!editor.tools_title.width);
    uis.mouseCoords[0] = editor.mouse_x = 20;
    uis.mouseCoords[1] = editor.mouse_y = 20;
    HUD_EditorKey(K_MOUSE1, true);
    assert(HUD_Value("sh_ups_x") == 1 && !editor.dragging && !editor.dragging_tools);
    HUD_Command(HUD_BUTTON_ARRANGE_SECTION); /* Reopen Arrange, then switch directly to Reset. */
    editor.buttons[HUD_BUTTON_ALIGN_LEFT] = (vrect_t) { 10, 10, 48, 18 };
    HUD_EditorKey('d', true);
    assert(editor.tools_section == HUD_SECTION_RESET && !editor.buttons[HUD_BUTTON_ALIGN_LEFT].width);
    HUD_EditorKey(K_MOUSE1, true);
    assert(HUD_Value("sh_ups_x") == 1);
    editor.buttons[HUD_BUTTON_RESET_ALL] = (vrect_t) { 10, 10, 100, 18 };
    HUD_Command(HUD_BUTTON_ARRANGE_SECTION); /* Opening Arrange replaces Reset. */
    assert(editor.tools_section == HUD_SECTION_ARRANGE && !editor.buttons[HUD_BUTTON_RESET_ALL].width);
    HUD_EditorKey(K_MOUSE1, true);
    assert(HUD_Value("sh_ups_x") == 1);
    HUD_Command(HUD_BUTTON_RESET_SECTION); HUD_Command(HUD_BUTTON_RESET_SECTION);
    assert(editor.tools_section == HUD_SECTION_NONE);
    assert(editor.history_pos == section_history && editor.history_count == section_count && writes == 0);
    HUD_Undo(); assert(HUD_Value("sh_ups_x") == 0);
    HUD_Redo(); assert(HUD_Value("sh_ups_x") == 1);

    /* Mouse nudge buttons update the draft and honor Shift precision. */
    setup();
    editor.buttons[HUD_BUTTON_MOVE_RIGHT] = (vrect_t) { 10, 10, 24, 18 };
    uis.mouseCoords[0] = 20; uis.mouseCoords[1] = 20;
    HUD_EditorKey(K_MOUSE1, true);
    assert(HUD_Value("sh_ups_x") == 1 && writes == 0 && !editor.dragging);
    held[K_SHIFT] = 1;
    HUD_EditorKey(K_MOUSE1, true);
    assert(HUD_Value("sh_ups_x") == 11 && writes == 0);
    HUD_EditorKey(K_ESCAPE, true);
    assert(writes == 0);
    /* Clicking a canvas widget outside the category returns the list to All. */
    setup(); extra_count = HL_BUILTIN_COUNT;
    HUD_SetCategory(HUD_CATEGORY_SERVER);
    editor.hide_tools = true;
    editor.mouse_x = 310; editor.mouse_y = 174;
    HUD_EditorKey(K_MOUSE1, true);
    assert(editor.selected == HUD_EDIT_UPS && editor.category == HUD_CATEGORY_ALL && editor.dragging);
    /* Reset all crosses category boundaries but leaves excluded settings alone. */
    for (int apply = 0; apply < 2; apply++) {
        setup_layout();
        hud_setting_t *ups = HUD_Setting("sh_ups_x");
        ups->original = ups->value = ups->var->value = 42;
        settings[HUD_NATIVE_SETTINGS].original = settings[HUD_NATIVE_SETTINGS].value = vars[HUD_NATIVE_SETTINGS].value = 25;
        settings[HUD_NATIVE_SETTINGS + 2].original = settings[HUD_NATIVE_SETTINGS + 2].value = vars[HUD_NATIVE_SETTINGS + 2].value = 0;
        vars[HUD_NATIVE_SETTINGS + 3] = (cvar_t) { 0 };
        vars[HUD_NATIVE_SETTINGS + 3].default_string = "0"; vars[HUD_NATIVE_SETTINGS + 3].value = 9;
        settings[HUD_NATIVE_SETTINGS + 3] = (hud_setting_t) { "hud_crosshair_x",
            HUD_EDIT_LAYOUT_FIRST + HL_CROSSHAIR, -16000, 16000, &vars[HUD_NATIVE_SETTINGS + 3], 9, 9, false };
        setting_count = HUD_NATIVE_SETTINGS + 4;
        HUD_SetCategory(HUD_CATEGORY_CLIENT); /* Client tab still resets Server settings. */
        if (apply) {
            editor.buttons[HUD_BUTTON_RESET_ALL] = (vrect_t) { 10, 10, 121, 18 };
            uis.mouseCoords[0] = 20; uis.mouseCoords[1] = 20;
            HUD_EditorKey(K_MOUSE1, true);
        } else {
            held[K_SHIFT] = 1; HUD_EditorKey('r', true); held[K_SHIFT] = 0;
        }
        assert(ups->value == 0 && settings[HUD_NATIVE_SETTINGS].value == 0 && settings[HUD_NATIVE_SETTINGS + 2].value == 1);
        assert(ups->var->value == 42 && vars[HUD_NATIVE_SETTINGS].value == 25 && vars[HUD_NATIVE_SETTINGS + 2].value == 0 && writes == 0);
        assert(settings[HUD_NATIVE_SETTINGS + 3].value == 9 && !settings[HUD_NATIVE_SETTINGS + 3].changed);
        HUD_EditorKey(apply ? K_ENTER : K_ESCAPE, true);
        assert(ups->var->value == (apply ? 0 : 42));
        assert(vars[HUD_NATIVE_SETTINGS].value == (apply ? 0 : 25));
        assert(vars[HUD_NATIVE_SETTINGS + 2].value == (apply ? 1 : 0));
        assert(vars[HUD_NATIVE_SETTINGS + 3].value == 9);
        if (!apply) assert(writes == 0);
    }
    /* A multi-event drag is one undo action, not one action per mouse event. */
    setup(); editor.mouse_x = 310; editor.mouse_y = 174; held[K_MOUSE1] = 1;
    HUD_EditorKey(K_MOUSE1, true);
    HUD_EditorMouse(640, 348); HUD_EditorMouse(660, 348);
    HUD_EditorKey(K_MOUSE1, false); held[K_MOUSE1] = 0;
    assert(HUD_Value("sh_ups_x") == 20 && editor.history_pos == 1);
    HUD_Undo(); assert(HUD_Value("sh_ups_x") == 0 && !HUD_Setting("sh_ups_x")->changed);
    HUD_Redo(); assert(HUD_Value("sh_ups_x") == 20 && writes == 0);
    HUD_Undo(); HUD_EditorKey(K_RIGHTARROW, true); HUD_Redo();
    assert(HUD_Value("sh_ups_x") == 1 && editor.history_count == 2);
    held[K_CTRL] = 1; HUD_EditorKey('z', true);
    assert(HUD_Value("sh_ups_x") == 0);
    held[K_SHIFT] = 1; HUD_EditorKey('z', true); held[K_SHIFT] = held[K_CTRL] = 0;
    assert(HUD_Value("sh_ups_x") == 1);
    setup();
    for (int i = 0; i < 80; i++) HUD_EditorKey(K_RIGHTARROW, true);
    assert(editor.history_count == HUD_HISTORY_STATES);
    for (int i = 0; i < 80; i++) HUD_Undo();
    assert(HUD_Value("sh_ups_x") == 16); /* most recent 64 actions retained */
    /* Lock blocks edits, including Reset all, and is itself undoable. */
    setup(); HUD_EditorKey(K_RIGHTARROW, true); HUD_EditorKey('l', true);
    assert(editor.locked[HUD_EDIT_UPS]);
    int locked_pos = editor.history_pos;
    HUD_EditorKey(K_RIGHTARROW, true); HUD_EditorKey('v', true); HUD_EditorKey('r', true);
    assert(HUD_Value("sh_ups_x") == 1 && editor.history_pos == locked_pos);
    editor.mouse_x = 310; editor.mouse_y = 174; HUD_EditorKey(K_MOUSE1, true);
    assert(!editor.dragging);
    HUD_Command(HUD_BUTTON_RESET_ALL); assert(HUD_Value("sh_ups_x") == 1);
    HUD_Undo(); /* reset of other defaults */
    HUD_Undo(); assert(!editor.locked[HUD_EDIT_UPS]);
    HUD_Redo(); assert(editor.locked[HUD_EDIT_UPS]);
    setup(); editor.locked[HUD_EDIT_EFFICIENCY] = true; editor.selected = HUD_EDIT_STRAFE;
    HUD_EditorKey(K_DOWNARROW, true);
    assert(HUD_Value("sh_y") == 0 && !HUD_CanResize());
    /* Lock writes are deferred to Apply; focus has no cvar or history effect. */
    for (int apply = 0; apply < 2; apply++) {
        setup(); cvar_t lock_var = { 0 }; editor.lock_vars[HUD_EDIT_UPS] = &lock_var;
        HUD_Command(HUD_BUTTON_LOCK); assert(lock_var.value == 0 && writes == 0);
        int history_count = editor.history_count;
        HUD_Command(HUD_BUTTON_FOCUS); editor.preview = true;
        assert(HUD_EditorShow(HUD_EDIT_UPS) && !HUD_EditorShow(HUD_EDIT_EFFICIENCY));
        editor.preview = false; assert(HUD_EditorShow(HUD_EDIT_EFFICIENCY));
        assert(editor.history_count == history_count && writes == 0);
        HUD_Command(apply ? HUD_BUTTON_APPLY : HUD_BUTTON_CANCEL);
        assert(lock_var.value == apply && writes == apply);
    }
    /* Screen alignment respects movement constraints and remains undoable. */
    setup(); HUD_Command(HUD_BUTTON_ALIGN_LEFT); assert(HUD_Value("sh_ups_x") == -300);
    HUD_Undo(); HUD_Command(HUD_BUTTON_ALIGN_RIGHT); assert(HUD_Value("sh_ups_x") == 316);
    HUD_Undo(); HUD_Command(HUD_BUTTON_ALIGN_HCENTER); assert(HUD_Value("sh_ups_x") == 8);
    HUD_Undo(); HUD_Command(HUD_BUTTON_ALIGN_TOP); assert(HUD_Value("sh_ups_y") == -170);
    HUD_Undo(); HUD_Command(HUD_BUTTON_ALIGN_BOTTOM); assert(HUD_Value("sh_ups_y") == 182);
    HUD_Undo(); HUD_Command(HUD_BUTTON_ALIGN_VCENTER); assert(HUD_Value("sh_ups_y") == 6);
    setup(); editor.selected = HUD_EDIT_STRAFE;
    HUD_Command(HUD_BUTTON_ALIGN_RIGHT); assert(HUD_Value("sh_y") == 0 && editor.history_pos == 0);
    puts("hud-editor draft/input tests passed");
    return 0;
}
/* Rendering and startup are boundary stubs; controller state remains real. */
client_state_t cl;
scr_t scr;
refcfg_t r_config;
cvar_t *cl_strafeHelperCenter, *cl_strafeHelperCenterMarker;
void Com_LPrintf(print_type_t type, const char *fmt, ...) {}
char *va(const char *format, ...) { return ""; }
void Cmd_AddCommand(const char *name, xcommand_t fn) {}
void Cmd_RemoveCommand(const char *name) {}
int Q_strcasecmp(const char *a, const char *b)
{
    while (*a && Q_tolower(*a) == Q_tolower(*b)) { a++; b++; }
    return Q_tolower(*a) - Q_tolower(*b);
}
cvar_t *Cvar_FindVar(const char *name)
{
    for (size_t i = 0; i < setting_count; i++)
        if (vars[i].name && !strcmp(vars[i].name, name))
            return &vars[i];
    return NULL;
}
keydest_t Key_GetDest(void) { return KEY_MENU; }
void Key_ClearStates(void) { memset(held, 0, sizeof(held)); }
void R_SetScale(float scale) {}
void UI_PushMenu(menuFrameWork_t *menu) { uis.activeMenu = menu; }
void UI_DrawString(int x, int y, int flags, const char *text) {}
void UI_ClearColor_Wrapper(void) {}
void UI_DrawFill32_Wrapper(int x, int y, int w, int h, uint32_t color) {}
void SH_NetMeter_Draw(void) {}
void StrafeHelper_DrawPreview(const struct StrafeHelperParams *p, float w, float h, int font) {}
void SH_Ups_Draw(float w, float h, float scale, int font) {}


int HUD_LayoutCount(void) { return extra_count; }
const hud_layout_item_t *HUD_LayoutItem(int id) { return id == 0 ? &layout_item : NULL; }
void HUD_LayoutPreview(void) {}
void SCR_HudEditorPrepare(void) {}

cvar_t *Cvar_Get(const char *name, const char *value, int flags)
{
    assert(lock_count < q_countof(locks));
    snprintf(lock_names[lock_count], sizeof(lock_names[lock_count]), "%s", name);
    locks[lock_count].integer = !strcmp(name, "hud_lock_network");
    return &locks[lock_count++];
}

size_t Q_snprintf(char *dst, size_t size, const char *fmt, ...)
{
    va_list args; va_start(args, fmt); int n = vsnprintf(dst, size, fmt, args); va_end(args);
    return n < 0 ? 0 : (size_t)n;
}
