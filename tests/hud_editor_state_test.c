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
static cvar_t test_game;
cvar_t *fs_game = &test_game;

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
    test_game.string = "jump";
    fs_game = &test_game;
    for (size_t i = 0; i < setting_count; i++) {
        vars[i] = (cvar_t) { 0 };
        vars[i].name = (char *)native_settings[i].name;
        bool visual_scale = !strncmp(native_settings[i].name, "hud_", 4);
        vars[i].default_string = visual_scale ? "1" : "0";
        vars[i].value = visual_scale ? 1 : 0;
        settings[i].var = &vars[i];
        settings[i].original = settings[i].value = vars[i].value;
        settings[i].changed = false;
    }
    editor.open = true;
    editor.ghosts = true;
    editor.preview_zoom = 1;
    editor.reference = -1;
    editor.held_button = -1;
    editor.step = 1;
    IF_Init(&editor.filter, 21, 64);
    IF_Init(&editor.input, 8, 24);
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
    const char *names[] = { "hud_timer_x", "hud_timer_y", "hud_timer_visible", "hud_timer_scale" };
    for (int i = 0; i < 4; i++) {
        vars[HUD_NATIVE_SETTINGS + i] = (cvar_t) { 0 };
        vars[HUD_NATIVE_SETTINGS + i].name = (char *)names[i];
        vars[HUD_NATIVE_SETTINGS + i].default_string = i >= 2 ? "1" : "0";
        vars[HUD_NATIVE_SETTINGS + i].value = i >= 2;
        settings[HUD_NATIVE_SETTINGS + i] = (hud_setting_t) { names[i], HUD_EDIT_LAYOUT_FIRST,
            i == 3 ? HUD_SCALE_MIN : i == 2 ? 0 : -16000,
            i == 3 ? HUD_SCALE_MAX : i == 2 ? 1 : 16000,
            &vars[HUD_NATIVE_SETTINGS + i], i >= 2, i >= 2, false };
    }
    setting_count = HUD_NATIVE_SETTINGS + 4;
    layout_item = (hud_layout_item_t) { "timer", "Run timer", &vars[HUD_NATIVE_SETTINGS],
        &vars[HUD_NATIVE_SETTINGS + 1], &vars[HUD_NATIVE_SETTINGS + 2], &vars[HUD_NATIVE_SETTINGS + 3] };
    editor.selected = HUD_EDIT_LAYOUT_FIRST;
    editor.bounds[HUD_EDIT_LAYOUT_FIRST] = (vrect_t) { 500, 300, 94, 32 };
}

/* Exercise every command slot, including Previous/Next which have no buttons. */
static void test_button_commands(void)
{
    assert(HUD_BUTTON_COUNT == 40);
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
        case HUD_BUTTON_EDITOR_VISIBLE:
            assert(!HUD_CanvasItem(HUD_EDIT_UPS) && !editor.history_count && x->value == 7);
            break;
        case HUD_BUTTON_GHOSTS:
            assert(!editor.ghosts && !editor.history_count);
            break;
        case HUD_BUTTON_STEP:
            assert(editor.step == 5 && !editor.history_count);
            break;
        case HUD_BUTTON_REFERENCE:
            assert(editor.align_reference && !editor.history_count);
            break;
        case HUD_BUTTON_CHANGES:
            assert(editor.changes_open && !editor.history_count);
            break;
        case HUD_BUTTON_KEYS:
            assert(editor.keys_open && !editor.history_count);
            break;
        case HUD_BUTTON_SCENARIO_LIVE:
        case HUD_BUTTON_SCENARIO_RUNNING:
        case HUD_BUTTON_SCENARIO_SPECTATING:
        case HUD_BUTTON_SCENARIO_VOTING:
        case HUD_BUTTON_SCENARIO_SCOREBOARD:
        case HUD_BUTTON_SCENARIO_NETWORK:
            assert(editor.scenario == button - HUD_BUTTON_SCENARIO_LIVE && !editor.history_count);
            break;
        case HUD_BUTTON_FIT:
        case HUD_BUTTON_100:
        case HUD_BUTTON_SELECTION:
        case HUD_BUTTON_ZOOM_OUT:
        case HUD_BUTTON_ZOOM_IN:
            assert(x->value == 7 && !editor.history_count);
            break;
        case HUD_BUTTON_INFO:
            assert(editor.info_expanded && !editor.history_count);
            break;
        default:
            assert(!"Unverified HUD command");
        }
        if (button != HUD_BUTTON_APPLY)
            assert(writes == 0);
    }
}

static void test_game_availability(void)
{
    const struct {
        char *game;
        bool available;
    } cases[] = {
        { NULL, false }, { "", false }, { "baseq2", false },
        { "race", false }, { "jumpmod", false },
        { "jump", true }, { "JUMP", true },
    };
    menuFrameWork_t previous_menu = { 0 };

    for (size_t i = 0; i < q_countof(cases); i++) {
        setup();
        uis.initialized = true;
        uis.scale = 1;
        editor.open = false;
        uis.activeMenu = &previous_menu;
        test_game.string = cases[i].game;
        fs_game = cases[i].game ? &test_game : NULL;
        held['w'] = 1;

        HUD_Open_f();
        assert(HUD_EditorActive() == cases[i].available);
        assert(uis.activeMenu == (cases[i].available ? &editor.menu : &previous_menu));
        assert(held['w'] == !cases[i].available);
        assert(writes == 0);
        if (cases[i].available) {
            HUD_Open_f();
            assert(!editor.open && !uis.activeMenu);
        } else {
            assert(lock_count == 0);
        }
    }
    uis.activeMenu = NULL;
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
    assert(setting_count == HUD_NATIVE_SETTINGS + 4);
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
        vars[i].value = !strncmp(settings[i].name, "hud_", 4) ? 1 : HudEdit_Clamp(0, settings[i].low, settings[i].high);
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

static void test_workbench_fields(void)
{
    setup();
    HUD_EditorKey('/', true);
    HUD_EditorChar('/');
    HUD_EditorKey('/', false);
    assert(editor.field == HUD_FIELD_FILTER && !editor.filter.text[0]);
    const char *query = "NETWORK";
    for (const char *c = query; *c; c++)
        HUD_EditorChar(*c);
    assert(HUD_Count() == 1 && HUD_Entry(0) == HUD_EDIT_NETWORK && writes == 0);
    HUD_EditorKey(K_ENTER, true);
    assert(editor.selected == HUD_EDIT_NETWORK && editor.field == HUD_FIELD_NONE && editor.open);
    HUD_FocusField(HUD_FIELD_FILTER);
    held[K_CTRL] = 1;
    HUD_EditorKey('a', true);
    held[K_CTRL] = 0;
    HUD_EditorChar('!');
    assert(HUD_Count() == 0);
    HUD_EditorKey(K_TAB, true);
    HUD_EditorKey(K_TAB, true);
    assert(editor.open && editor.selected == HUD_EDIT_NETWORK && writes == 0);
    IF_Clear(&editor.filter);

    setup();
    HUD_EditorKey('x', true);
    HUD_EditorChar('x');
    HUD_EditorKey('x', false);
    assert(!strcmp(editor.input.text, "300"));
    HUD_EditorChar('4');
    HUD_EditorChar('0');
    HUD_EditorChar('0');
    assert(HUD_Value("sh_ups_x") == 0 && writes == 0);
    HUD_EditorKey(K_ENTER, true);
    assert(editor.field == HUD_FIELD_NONE && editor.open && HUD_Value("sh_ups_x") == 100 && writes == 0);
    HUD_Undo();
    assert(HUD_Value("sh_ups_x") == 0);
    HUD_Redo();
    assert(HUD_Value("sh_ups_x") == 100);
    static const char *const invalid[] = { "", "12junk", "nan", "inf", "1e9999", "   " };
    for (int i = 0; i < q_countof(invalid); i++) {
        HUD_FocusField(HUD_FIELD_X);
        IF_Replace(&editor.input, invalid[i]);
        int history_pos = editor.history_pos;
        HUD_EditorKey(K_ENTER, true);
        assert(editor.field_bad && editor.field == HUD_FIELD_X && editor.open);
        assert(HUD_Value("sh_ups_x") == 100 && history_pos == editor.history_pos && writes == 0);
        HUD_EditorKey(K_ESCAPE, true);
    }
    HUD_FocusField(HUD_FIELD_Y);
    IF_Replace(&editor.input, "-900");
    HUD_EditorKey(K_ENTER, true);
    assert(HUD_Value("sh_ups_y") == -170);
    HUD_FocusField(HUD_FIELD_SCALE);
    IF_Replace(&editor.input, "2.5");
    HUD_EditorKey(K_ENTER, true);
    assert(HUD_Value("hud_ups_scale") == 2.5f);
    editor.locked[HUD_EDIT_UPS] = true;
    HUD_FocusField(HUD_FIELD_X);
    assert(editor.field == HUD_FIELD_NONE);
    editor.selected = HUD_EDIT_STRAFE;
    HUD_FocusField(HUD_FIELD_X);
    assert(editor.field == HUD_FIELD_NONE);
    editor.selected = HUD_EDIT_NETWORK;
    editor.network_mode = 2;
    HUD_FocusField(HUD_FIELD_SCALE);
    assert(editor.field == HUD_FIELD_SCALE);
    HUD_EditorKey(K_ESCAPE, true);
    HUD_FocusField(HUD_FIELD_X);
    assert(editor.field == HUD_FIELD_NONE);

    /* Modifier events precede shifted characters on layouts such as AZERTY. */
    const int modifiers[] = { K_SHIFT, K_LSHIFT, K_RSHIFT, K_CTRL, K_LCTRL,
                              K_RCTRL, K_ALT, K_LALT, K_RALT };
    for (size_t i = 0; i < q_countof(modifiers); i++) {
        setup();
        HUD_FocusField(HUD_FIELD_X);
        held[modifiers[i]] = 1;
        HUD_EditorKey(modifiers[i], true);
        assert(editor.replace_field);
        HUD_EditorChar('4');
        HUD_EditorKey(modifiers[i], false);
        held[modifiers[i]] = 0;
        assert(!strcmp(editor.input.text, "4"));
        HUD_EditorKey(K_ENTER, true);
        assert(HUD_Value("sh_ups_x") == -296 && writes == 0);
        HUD_Undo();
        assert(HUD_Value("sh_ups_x") == 0);
    }

    /* A handled cursor key still cancels replacement and edits in place. */
    setup();
    HUD_FocusField(HUD_FIELD_X);
    HUD_EditorKey(K_LEFTARROW, true);
    HUD_EditorChar('4');
    assert(!strcmp(editor.input.text, "3040") && !editor.replace_field);

    /* Clicking the active numeric field preserves its uncommitted input. */
    static const struct {
        hud_field_t field;
        const char *text, *setting;
        float expected;
    } clicks[] = {
        { HUD_FIELD_X, "456", "sh_ups_x", 156 },
        { HUD_FIELD_Y, "210", "sh_ups_y", 40 },
        { HUD_FIELD_SCALE, "2.5", "hud_ups_scale", 2.5f },
    };
    for (size_t i = 0; i < q_countof(clicks); i++) {
        setup();
        uis.width = 640;
        uis.height = 360;
        uis.scale = .5f;
        HUD_WorkbenchLayout();
        HUD_WorkbenchDraw();
        vrect_t field = editor.fields[clicks[i].field];
        assert(field.width > 0);
        uis.mouseCoords[0] = (field.x + field.width / 2) * HUD_ToolsZoom();
        uis.mouseCoords[1] = (field.y + field.height / 2) * HUD_ToolsZoom();
        HUD_EditorKey(K_MOUSE1, true);
        HUD_EditorKey(K_MOUSE1, false);
        for (const char *c = clicks[i].text; *c; c++)
            HUD_EditorChar(*c);
        HUD_EditorKey(K_LEFTARROW, true);
        size_t cursor = editor.input.cursorPos;
        HUD_EditorKey(K_MOUSE1, true);
        HUD_EditorKey(K_MOUSE1, false);
        assert(editor.field == clicks[i].field && !strcmp(editor.input.text, clicks[i].text));
        assert(editor.input.cursorPos == cursor && !editor.replace_field);
        float initial = clicks[i].field == HUD_FIELD_SCALE ? 1 : 0;
        assert(!editor.history_count && HUD_Value(clicks[i].setting) == initial && writes == 0);
        HUD_EditorKey(K_ENTER, true);
        assert(editor.field == HUD_FIELD_NONE && HUD_Value(clicks[i].setting) == clicks[i].expected);
        assert(editor.history_count == 2 && writes == 0);
        HUD_Undo();
        assert(HUD_Value(clicks[i].setting) == initial);
    }
}

static void test_visual_scaling(void)
{
    for (int apply = 0; apply < 2; apply++) {
        setup_layout();
        HUD_FocusField(HUD_FIELD_SCALE);
        IF_Replace(&editor.input, "2");
        HUD_EditorKey(K_ENTER, true);
        assert(HUD_Value("hud_timer_scale") == 2 && writes == 0);
        /* Original top-left 500,300 stays fixed about the 640,360 pivot. */
        assert(HUD_Value("hud_timer_x") == 140 && HUD_Value("hud_timer_y") == 60);
        HUD_Undo();
        assert(HUD_Value("hud_timer_scale") == 1 && HUD_Value("hud_timer_x") == 0);
        HUD_Redo();
        assert(HUD_Value("hud_timer_scale") == 2 && HUD_Value("hud_timer_x") == 140);
        HUD_Command(apply ? HUD_BUTTON_APPLY : HUD_BUTTON_CANCEL);
        assert(layout_item.scale->value == (apply ? 2 : 1));
        assert(layout_item.x->value == (apply ? 140 : 0));
        assert(writes == (apply ? 3 : 0));
    }
    setup_layout();
    HUD_StartResize(); HUD_Resize(94, 32);
    assert(HUD_Value("hud_timer_scale") == 2);
    editor.locked[editor.selected] = true;
    assert(!HUD_CanResize());
    HUD_Resize(940, 320);
    assert(HUD_Value("hud_timer_scale") == 2);
    HUD_FocusField(HUD_FIELD_SCALE);
    assert(editor.field == HUD_FIELD_NONE);
    editor.locked[editor.selected] = false;
    HUD_Reset();
    assert(HUD_Value("hud_timer_scale") == 1 && HUD_Value("hud_timer_x") == 0);
    HUD_SetVisualScale(100);
    assert(HUD_Value("hud_timer_scale") == HUD_SCALE_MAX);
    HUD_SetVisualScale(-1);
    assert(HUD_Value("hud_timer_scale") == HUD_SCALE_MIN);
    assert(writes == 0);

    /* Queued movement and proportional resizing use the latest draft bounds. */
    setup_layout();
    HUD_Move(5, 0, false);
    assert(editor.bounds[editor.selected].x == 505);
    HUD_SetVisualScale(2);
    assert(HUD_Value("hud_timer_x") == 145);
    assert(editor.bounds[editor.selected].width == 188);
    HUD_StartResize(); HUD_Resize(94, 32);
    assert(HUD_Value("hud_timer_scale") == 3);
    assert(editor.bounds[editor.selected].width == 282);

    /* Undo/redo and reset/revert must also refresh geometry before queued input. */
    for (int restore = 0; restore < 3; restore++) {
        setup_layout();
        editor.width = 1280; editor.height = 720;
        HUD_HistoryBegin(); HUD_SetVisualScale(2); HUD_HistoryCommit();
        HUD_Nudge(40, 0, false);
        assert(editor.bounds[editor.selected].x == 540);
        HUD_Undo();
        assert(editor.bounds[editor.selected].x == 500);
        HUD_Redo();
        assert(editor.bounds[editor.selected].x == 540);
        if (restore == 0) {
            HUD_Undo();
        } else if (restore == 1) {
            HUD_Reset();
        } else {
            HUD_Revert(editor.selected);
        }
        assert(editor.bounds[editor.selected].x == 500);
        assert(editor.bounds[editor.selected].width == (restore ? 94 : 188));
        HUD_SetVisualScale(3);
        assert(HUD_Value("hud_timer_x") == 280 && HUD_Value("hud_timer_y") == 120);
        assert(editor.bounds[editor.selected].x == 500 && editor.bounds[editor.selected].y == 300);
        assert(editor.bounds[editor.selected].width == 282);
    }

    for (int resize = 0; resize < 2; resize++) {
        setup_layout();
        editor.width = 1280; editor.height = 720;
        editor.mouse_x = resize ? 594 : 510;
        editor.mouse_y = resize ? 332 : 310;
        held[K_MOUSE1] = 1;
        HUD_EditorKey(K_MOUSE1, true);
        assert(editor.dragging && editor.resizing == resize);
        HUD_EditorMouse(resize ? 1376 : 1100, resize ? 728 : 620);
        assert(editor.bounds[editor.selected].x == (resize ? 500 : 540));
        assert(editor.bounds[editor.selected].width == (resize ? 188 : 94));
        HUD_EditorKey(K_MOUSE1, false);
        HUD_Undo();
        assert(editor.bounds[editor.selected].x == 500 && editor.bounds[editor.selected].width == 94);
        HUD_SetVisualScale(3);
        assert(HUD_Value("hud_timer_x") == 280 && HUD_Value("hud_timer_y") == 120);
        assert(editor.bounds[editor.selected].x == 500 && editor.bounds[editor.selected].width == 282);
    }
    setup_layout();
    HUD_Setting("hud_timer_scale")->original = NAN;
    HUD_SetVisualScale(2);
    HUD_Revert(editor.selected);
    assert(HUD_Value("hud_timer_scale") == 1 && !HUD_Setting("hud_timer_scale")->changed);

    /* Native dimension handles work in unscaled units and do not jump on grab. */
    for (int mode = 2; mode <= 3; mode++) {
        setup();
        editor.selected = HUD_EDIT_NETWORK;
        editor.network_mode = mode;
        HUD_Set("hud_network_scale", 2);
        HUD_Set("sh_netgraph_height", 20);
        HUD_Set("sh_histogram_width", 160);
        HUD_Set("sh_histogram_height", 64);
        editor.bounds[editor.selected] = (vrect_t) { 0, 0, mode == 2 ? 640 : 320, mode == 2 ? 40 : 128 };
        HUD_StartResize(); HUD_Resize(0, 0);
        assert(HUD_Value("sh_netgraph_height") == 20);
        assert(HUD_Value("sh_histogram_width") == 160 && HUD_Value("sh_histogram_height") == 64);
        HUD_Resize(20, 20);
        if (mode == 2)
            assert(HUD_Value("sh_netgraph_height") == 30);
        else
            assert(HUD_Value("sh_histogram_width") == 170 && HUD_Value("sh_histogram_height") == 74);
    }

    /* Every native mode exposes visual size independently of native dimensions. */
    for (int id = 0; id < HUD_EDIT_LAYOUT_FIRST; id++) {
        setup();
        editor.selected = id;
        for (int mode = 1; mode <= 3; mode++) {
            editor.network_mode = mode;
            assert(HUD_FieldEnabled(HUD_FIELD_SCALE));
            HUD_SetVisualScale(1.5f);
            assert(HUD_Value(HUD_ScaleSetting()) == 1.5f);
            assert(HUD_Value("sh_scale") == 0 && HUD_Value("sh_ups_scale") == 0);
            assert(HUD_Value("sh_efficiency_width") == 0 && writes == 0);
        }
    }
}

static void test_workbench_cancel(void)
{
    static const char *const invalid[] = { "", "12junk", "nan", "inf", "1e9999", "   " };

    for (int field = HUD_FIELD_X; field <= HUD_FIELD_SCALE; field++) {
        for (int i = 0; i < q_countof(invalid); i++) {
            setup();
            uis.width = 640;
            uis.height = 360;
            uis.scale = .5f;
            HUD_Nudge(4, 3, false);
            HUD_WorkbenchLayout();
            HUD_FocusField((hud_field_t)field);
            IF_Replace(&editor.input, invalid[i]);
            HUD_WorkbenchDraw();

            /* Apply validates the field and leaves the draft open on failure. */
            vrect_t button = editor.buttons[HUD_BUTTON_APPLY];
            assert(button.width > 0 && button.height > 0);
            uis.mouseCoords[0] = (button.x + button.width / 2) * HUD_ToolsZoom();
            uis.mouseCoords[1] = (button.y + button.height / 2) * HUD_ToolsZoom();
            HUD_EditorKey(K_MOUSE1, true);
            assert(editor.open && editor.field_bad && writes == 0);
            assert(HUD_Value("sh_ups_x") == 4 && HUD_Value("sh_ups_y") == 3);
            HUD_EditorKey(K_MOUSE1, false);

            /* Cancel must remain reachable without accepting the invalid value. */
            button = editor.buttons[HUD_BUTTON_CANCEL];
            assert(button.width > 0 && button.height > 0);
            uis.mouseCoords[0] = (button.x + button.width / 2) * HUD_ToolsZoom();
            uis.mouseCoords[1] = (button.y + button.height / 2) * HUD_ToolsZoom();
            HUD_EditorKey(K_MOUSE1, true);
            assert(!editor.open && uis.activeMenu == NULL && writes == 0);
            for (int j = 0; j < setting_count; j++)
                assert(settings[j].var->value == settings[j].original);
        }
    }
}

static void test_workbench_preview(void)
{
    setup();
    uis.width = 640;
    uis.height = 360;
    uis.scale = .5f;
    r_config.width = 1280;
    r_config.height = 720;
    HUD_WorkbenchLayout();
    assert(fabsf(editor.preview_zoom - .720f) < .002f);
    vrect_t viewport = { 0 };
    assert(HUD_EditorViewport(&viewport) && viewport.x == editor.stage.x);
    int mx = Q_rint(editor.view_x + 310 * editor.preview_zoom / editor.scale) / 2 * 2;
    int my = Q_rint(editor.view_y + 174 * editor.preview_zoom / editor.scale) / 2 * 2;
    uis.mouseCoords[0] = mx * uis.scale;
    uis.mouseCoords[1] = my * uis.scale;
    HUD_EditorMouse(mx, my);
    held[K_MOUSE1] = 1;
    HUD_EditorKey(K_MOUSE1, true);
    assert(editor.dragging);
    HUD_EditorMouse(mx + Q_rint(20 * editor.preview_zoom / editor.scale), my);
    assert(fabsf(HUD_Value("sh_ups_x") - 20) < .6f && writes == 0);
    HUD_EditorKey(K_MOUSE1, false);
    HUD_Undo();
    assert(HUD_Value("sh_ups_x") == 0);
    editor.open = false;
    viewport = (vrect_t){ 1, 2, 3, 4 };
    assert(!HUD_EditorViewport(&viewport) && viewport.x == 1 && viewport.width == 3);

    setup();
    editor.preview = true;
    editor.ghosts = false;
    assert(HUD_EditorShow(HUD_EDIT_UPS) && !HUD_EditorShow(HUD_EDIT_EFFICIENCY));
    HUD_Set("sh_efficiency", 1);
    assert(HUD_EditorShow(HUD_EDIT_EFFICIENCY));
    editor.focus = true;
    assert(!HUD_EditorShow(HUD_EDIT_EFFICIENCY));
    editor.preview = false;
    assert(HUD_EditorShow(HUD_EDIT_EFFICIENCY));
    setup();
    editor.collapsed[0] = true;
    assert(HUD_Count() == 1 && HUD_Entry(0) == HUD_EDIT_NETWORK);
    IF_Replace(&editor.filter, "strafe");
    assert(HUD_Count() == 2 && HUD_Entry(0) == HUD_EDIT_STRAFE);
}

static bool HUD_Dirty(int id)
{
    bool dirty[HUD_EDIT_COUNT];
    HUD_DirtyFlags(dirty);
    return dirty[id];
}

static void test_workbench_changes(void)
{
    setup();
    HUD_Nudge(4, 3, false);
    HUD_Command(HUD_BUTTON_LOCK);
    assert(HUD_Dirty(HUD_EDIT_UPS));
    HUD_Revert(HUD_EDIT_UPS);
    assert(!HUD_Dirty(HUD_EDIT_UPS) && !editor.locked[HUD_EDIT_UPS] && writes == 0);
    HUD_Undo();
    assert(HUD_Dirty(HUD_EDIT_UPS) && editor.locked[HUD_EDIT_UPS]);
    HUD_Redo();
    assert(!HUD_Dirty(HUD_EDIT_UPS));
    /* The actual footer button resets the draft and retains locked settings. */
    setup();
    editor.network_mode = 3;
    HUD_Set("sh_netmeter", 3);
    editor.locked[HUD_EDIT_NETWORK] = true;
    HUD_Nudge(4, 3, false);
    uis.width = 640;
    uis.height = 360;
    uis.scale = .5f;
    HUD_WorkbenchLayout();
    HUD_WorkbenchDraw();
    vrect_t reset = editor.buttons[HUD_BUTTON_RESET_ALL];
    assert(reset.width > 0 && reset.x + reset.width <= editor.rail.width);
    uis.mouseCoords[0] = (reset.x + reset.width / 2) * HUD_ToolsZoom();
    uis.mouseCoords[1] = (reset.y + reset.height / 2) * HUD_ToolsZoom();
    HUD_EditorKey(K_MOUSE1, true);
    assert(HUD_Value("sh_ups_x") == 0 && HUD_Value("sh_ups_y") == 0);
    assert(HUD_Value("sh_netmeter") == 3 && editor.locked[HUD_EDIT_NETWORK]);
    assert(editor.history_count == 3 && writes == 0);
    HUD_Undo();
    assert(HUD_Value("sh_ups_x") == 4 && HUD_Value("sh_ups_y") == 3);
    HUD_Redo();
    assert(HUD_Value("sh_ups_x") == 0 && HUD_Value("sh_ups_y") == 0 && writes == 0);
    setup();
    editor.selected = HUD_EDIT_EFFICIENCY;
    editor.bounds[HUD_EDIT_EFFICIENCY] = (vrect_t){ 100, 100, 20, 10 };
    editor.reference = HUD_EDIT_UPS;
    editor.align_reference = true;
    HUD_Command(HUD_BUTTON_ALIGN_LEFT);
    assert(HUD_Value("sh_efficiency_x") == 200);
    HUD_Undo();
    assert(HUD_Value("sh_efficiency_x") == 0);
}


static void test_workbench_draw_and_hold(void)
{
    static cvar_t center;
    cl_strafeHelperCenter = cl_strafeHelperCenterMarker = &center;
    const int sizes[][2] = { {640, 480}, {1280, 720}, {1920, 1080}, {3440, 1440} };
    for (int s = 0; s < q_countof(sizes) * 2; s++) {
        setup();
        editor.info_expanded = s % 2 != 0;
        uis.scale = .5f;
        scr.hud_scale = .5f;
        r_config.width = sizes[s / 2][0];
        r_config.height = sizes[s / 2][1];
        uis.width = r_config.width * uis.scale;
        uis.height = r_config.height * uis.scale;
        HUD_WorkbenchLayout();
        HUD_WorkbenchDraw();
        float zoom = HUD_ToolsZoom();
        int width = uis.width / zoom, height = uis.height / zoom;
        assert(editor.stage.x + editor.stage.width <= r_config.width + 1);
        assert(editor.stage.y + editor.stage.height <= editor.dock.y * zoom / uis.scale + 1);
        for (int i = 0; i < HUD_BUTTON_COUNT; i++) {
            vrect_t r = editor.buttons[i];
            if (!r.width)
                continue;
            assert(r.x >= 0 && r.y >= 0 && r.x + r.width <= width && r.y + r.height <= height);
            for (int j = i + 1; j < HUD_BUTTON_COUNT; j++) {
                vrect_t q = editor.buttons[j];
                assert(!q.width || r.x >= q.x + q.width || q.x >= r.x + r.width ||
                       r.y >= q.y + q.height || q.y >= r.y + r.height);
            }
        }
        for (int id = 0; id < HUD_Limit(); id++) {
            vrect_t row = editor.rows[id], view = editor.preview_toggles[id], hud = editor.toggles[id];
            if (!row.width)
                continue;
            assert(view.width > 0 && hud.width > 0);
            assert(view.x >= row.x && view.x + view.width <= hud.x && hud.x + hud.width <= row.x + row.width);
            assert(view.y >= row.y && view.y + view.height <= row.y + row.height);
            assert(hud.y >= row.y && hud.y + hud.height <= row.y + row.height);
        }
        vrect_t pad = editor.buttons[HUD_BUTTON_MOVE_RIGHT];
        uis.mouseCoords[0] = (pad.x + 8) * zoom;
        uis.mouseCoords[1] = (pad.y + 8) * zoom;
        editor.scale = scr.hud_scale;
        editor.width = r_config.width * scr.hud_scale;
        editor.height = r_config.height * scr.hud_scale;
        editor.active = cls.active;
        editor.bounds[HUD_EDIT_UPS] = (vrect_t) {100, 100, 24, 8};
        held[K_MOUSE1] = 1;
        HUD_EditorKey(K_MOUSE1, true);
        assert(HUD_Value("sh_ups_x") == 1 && editor.held_changed);
        for (int repeat = 0; repeat < 4; repeat++) {
            editor.bounds[HUD_EDIT_UPS] = (vrect_t) {100 + repeat + 1, 100, 24, 8};
            com_localTime = editor.hold_time;
            HUD_Draw(&editor.menu);
        }
        assert(HUD_Value("sh_ups_x") == 5 && editor.history_count == 2 && writes == 0);
        HUD_EditorKey(K_MOUSE1, false);
        HUD_Undo();
        assert(HUD_Value("sh_ups_x") == 0);
        HUD_Redo();
        assert(HUD_Value("sh_ups_x") == 5);
        HUD_Command(HUD_BUTTON_KEYS);
        HUD_WorkbenchDraw();
        assert(editor.overlay.x >= 0 && editor.overlay.y >= 0 &&
               editor.overlay.y + editor.overlay.height < editor.dock.y);
    }
}

static bool layout_available = true, layout_captured;
static int backdrop_draws, backdrop_width = 1600, backdrop_height = 900;
static vrect_t backdrop_rect;

static void test_preview_context(void)
{
    setup_layout();
    editor.selected = HUD_EDIT_UPS;
    layout_available = false;
    assert(!HUD_CanvasItem(HUD_EDIT_LAYOUT_FIRST));
    editor.selected = HUD_EDIT_LAYOUT_FIRST;
    assert(HUD_CanvasItem(HUD_EDIT_LAYOUT_FIRST));
    editor.selected = HUD_EDIT_UPS;
    layout_available = true;
    assert(HUD_CanvasItem(HUD_EDIT_LAYOUT_FIRST));
    editor.stage = (vrect_t){ 200, 0, 800, 450 };
    editor.backdrop = 1;
    backdrop_draws = 0;
    connstate_t saved_state = cls.state;
    cls.state = ca_disconnected;
    HUD_DrawBackdrop();
    assert(backdrop_draws == 1 && backdrop_rect.x == 200 && backdrop_rect.width == 800 && backdrop_rect.height == 450);
    editor.stage = (vrect_t) { 200, 0, 800, 600 };
    HUD_DrawBackdrop();
    assert(backdrop_draws == 2 && backdrop_rect.x < 200 && backdrop_rect.width > 800 && backdrop_rect.height == 600);
    cls.state = ca_active;
    HUD_DrawBackdrop();
    assert(backdrop_draws == 2);
    editor.scenario = HUD_PREVIEW_VOTING;
    HUD_DrawBackdrop();
    assert(backdrop_draws == 3); /* Samples cover an active map. */
    editor.scenario = HUD_PREVIEW_LIVE;
    editor.manual_zoom = editor.preview_zoom = 2;
    editor.view_x = -100;
    editor.view_y = -50;
    HUD_DrawBackdrop();
    assert(backdrop_draws == 4 && backdrop_rect.x == -100 && backdrop_rect.y == -50);
    assert(backdrop_rect.width == 2560 && backdrop_rect.height == 1440);
    editor.manual_zoom = 0;
    HUD_DrawBackdrop();
    assert(backdrop_draws == 4);
    backdrop_draws = 2;
    cls.state = ca_disconnected;
    backdrop_width = 0;
    HUD_DrawBackdrop();
    assert(backdrop_draws == 2);
    backdrop_width = 1600;
    editor.backdrop = 0;
    HUD_DrawBackdrop();
    assert(backdrop_draws == 2 && writes == 0);
    cls.state = saved_state;
}

static void test_shared_network_locks(void)
{
    setup();
    editor.selected = HUD_EDIT_NETWORK;
    editor.network_mode = 1;
    editor.bounds[HUD_EDIT_NETWORK] = (vrect_t){ 200, 100, 48, 48 };
    hud_setting_t *x = HUD_Setting("sh_lagometer_x"), *y = HUD_Setting("sh_lagometer_y");
    x->original = x->value = x->var->value = 200;
    y->original = y->value = y->var->value = 100;
    editor.locked[HUD_EDIT_LAYOUT_FIRST + HL_NETICON] = true;
    HUD_Command(HUD_BUTTON_MOVE_RIGHT);
    HUD_Set("sh_lagometer_y", 300);
    assert(x->value == 200 && y->value == 100 && HUD_Locked(HUD_EDIT_NETWORK));
    /* Resets/reverts of an inactive network mode must respect the same lock. */
    editor.network_mode = 3;
    assert(!HUD_Locked(HUD_EDIT_NETWORK));
    HUD_ResetAll();
    assert(x->value == 200 && y->value == 100);
    HUD_Set("sh_histogram_x", 30);
    assert(HUD_Value("sh_histogram_x") == 30);
    x->original = 150;
    x->changed = true; /* Draft movement made before locking. */
    HUD_Revert(HUD_EDIT_NETWORK);
    assert(x->value == 200 && x->changed && y->value == 100);
    editor.locked[HUD_EDIT_LAYOUT_FIRST + HL_DEBUGGRAPH] = true;
    hud_setting_t *h = HUD_Setting("sh_netgraph_height");
    h->original = h->value = h->var->value = 40;
    HUD_Set("sh_netgraph_height", 80);
    HUD_ResetAll();
    assert(h->value == 40);
    HUD_Set("sh_histogram_height", 12);
    assert(HUD_Value("sh_histogram_height") == 12);
    editor.network_mode = 2;
    assert(HUD_Locked(HUD_EDIT_NETWORK));
    editor.locked[HUD_EDIT_LAYOUT_FIRST + HL_NETICON] = false;
    editor.locked[HUD_EDIT_LAYOUT_FIRST + HL_DEBUGGRAPH] = false;
    HUD_Set("sh_lagometer_y", 300);
    HUD_Set("sh_netgraph_height", 80);
    assert(y->value == 300 && h->value == 80);
    HUD_Apply();
    assert(y->var->value == 300 && h->var->value == 80);
}

static void test_element_information(void)
{
    setup();
    extra_count = HL_BUILTIN_COUNT + 1;
    int reminders = HUD_EDIT_LAYOUT_FIRST + HL_BIND_REMINDERS;
    assert(HUD_Editable(reminders));
    assert(HUD_Category(reminders) == HUD_CATEGORY_CLIENT);
    assert(HUD_Group(reminders) == HUD_Group(HUD_EDIT_LAYOUT_FIRST + HL_INPUTS));
    for (int id = 0; id < HUD_Limit(); id++) {
        if (!HUD_Editable(id))
            continue;
        const hud_info_t *info = HUD_Info(id);
        assert(info->contents && info->contents[0] && info->appears && info->appears[0]);
        assert(strlen(info->contents) < 115 && strlen(info->appears) < 113);
        assert(HUD_EditDescription(id)[0]);
    }
    setup_layout();
    connstate_t saved_state = cls.state;
    const char *reason;
    HUD_Set("sh_ups", 0);
    assert(HUD_Status(HUD_EDIT_UPS, &reason) == HUD_STATUS_DISABLED && strstr(reason, "Disabled in HUD"));
    HUD_Set("sh_ups", 1);
    cls.state = ca_disconnected;
    assert(HUD_Status(HUD_EDIT_UPS, &reason) == HUD_STATUS_WAITING && strstr(reason, "Load a map"));
    cls.state = ca_active;
    assert(HUD_Status(HUD_EDIT_UPS, &reason) == HUD_STATUS_PREVIEW);
    HUD_Set("sh_efficiency", 1);
    HUD_Set("sh_draw", 0);
    assert(HUD_Status(HUD_EDIT_EFFICIENCY, &reason) == HUD_STATUS_DISABLED && strstr(reason, "helper"));
    HUD_Set("hud_timer_visible", 1);
    layout_captured = true;
    assert(HUD_Status(HUD_EDIT_LAYOUT_FIRST, &reason) == HUD_STATUS_VISIBLE);
    layout_captured = false;
    assert(HUD_Status(HUD_EDIT_LAYOUT_FIRST, &reason) == HUD_STATUS_WAITING);
    HUD_Set("hud_timer_visible", 0);
    layout_captured = true;
    assert(HUD_Status(HUD_EDIT_LAYOUT_FIRST, &reason) == HUD_STATUS_DISABLED);
    layout_captured = false;
    cls.state = saved_state;
}

static void test_scenarios_and_view_controls(void)
{
    setup();
    uis.width = 640;
    uis.height = 360;
    uis.scale = .5f;
    r_config.width = 1280;
    r_config.height = 720;
    HUD_WorkbenchLayout();
    vrect_t stage = editor.stage;
    int dock_y = editor.dock.y;
    HUD_Command(HUD_BUTTON_INFO);
    assert(editor.info_expanded && editor.dock.y == dock_y - 42);
    assert(editor.stage.height < stage.height);
    HUD_Command(HUD_BUTTON_INFO);
    assert(!editor.info_expanded && editor.dock.y == dock_y);
    assert(!memcmp(&stage, &editor.stage, sizeof(stage)));

    /* Every scenario keeps excluded groups excluded and has its event sample. */
    const int event[] = {HL_CENTER, HL_TARGET, HL_VOTE, HL_SCOREBOARD, HL_NETALERT};
    for (int s = HUD_PREVIEW_RUNNING; s < HUD_PREVIEW_COUNT; s++) {
        HUD_Command((hud_button_t)(HUD_BUTTON_SCENARIO_LIVE + s));
        assert(HUD_EditorScenario() == s && HUD_ContextAvailable(event[s - 1]));
        for (int id = 0; id < HL_BUILTIN_COUNT; id++)
            if (!HUD_LayoutEditable(id))
                assert(!HUD_ContextAvailable(id));
        assert(HUD_ContextAvailable(HL_TIMER) && HUD_ContextAvailable(HL_RENDER_FPS));
        for (int other = HUD_PREVIEW_RUNNING; other < HUD_PREVIEW_COUNT; other++)
            if (s != other)
                assert(!HUD_ContextAvailable(event[other - 1]));
    }
    const char *reason;
    HUD_Set("sh_ups", 1);
    connstate_t saved = cls.state;
    cls.state = ca_disconnected;
    assert(HUD_Status(HUD_EDIT_UPS, &reason) == HUD_STATUS_PREVIEW && strstr(reason, "Scenario sample"));
    HUD_Command(HUD_BUTTON_SCENARIO_LIVE);
    assert(HUD_Status(HUD_EDIT_UPS, &reason) == HUD_STATUS_WAITING);
    cls.state = saved;
    HUD_Set("sh_ups", 0);

    /* Zoom about a pointer preserves its HUD coordinate and the scene viewport. */
    int mx = (stage.x + stage.width / 3) / 2 * 2, my = (stage.y + stage.height / 3) / 2 * 2;
    uis.mouseCoords[0] = mx * uis.scale;
    uis.mouseCoords[1] = my * uis.scale;
    HUD_EditorMouse(mx, my);
    float hx = editor.mouse_x, hy = editor.mouse_y, fit = editor.preview_zoom;
    held[K_CTRL] = 1;
    HUD_EditorKey(K_MWHEELUP, true);
    held[K_CTRL] = 0;
    HUD_EditorMouse(mx, my);
    assert(editor.preview_zoom > fit && fabsf(editor.mouse_x - hx) < .001f && fabsf(editor.mouse_y - hy) < .001f);
    vrect_t viewport;
    assert(HUD_EditorViewport(&viewport) && !memcmp(&viewport, &stage, sizeof(stage)));
    HUD_Command(HUD_BUTTON_100);
    assert(editor.preview_zoom == 1);
    HUD_Command(HUD_BUTTON_SELECTION);
    assert(editor.preview_zoom == 4 && editor.center_x == 312 && editor.center_y == 174);
    HUD_Command(HUD_BUTTON_FIT);
    assert(!editor.manual_zoom && fabsf(editor.preview_zoom - fit) < .001f);
    HUD_ZoomAt(100, mx, my);
    assert(editor.preview_zoom == 4);
    HUD_ZoomAt(.001f, mx, my);
    assert(editor.preview_zoom == .25f);
    HUD_Command(HUD_BUTTON_100);

    /* Pan changes the view only; both supported gestures release cleanly. */
    float cx = editor.center_x, cy = editor.center_y;
    held[K_MOUSE3] = 1;
    HUD_EditorKey(K_MOUSE3, true);
    assert(editor.panning && !editor.dragging);
    HUD_EditorMouse(mx + 40, my + 20);
    assert(fabsf(editor.center_x - (cx - 20)) < .001f && fabsf(editor.center_y - (cy - 10)) < .001f);
    HUD_EditorKey(K_MOUSE3, false);
    held[K_MOUSE3] = 0;
    assert(!editor.panning);
    held[K_SPACE] = held[K_MOUSE1] = 1;
    HUD_EditorKey(K_MOUSE1, true);
    assert(editor.panning && !editor.dragging);
    HUD_EditorKey(K_SPACE, false);
    held[K_SPACE] = held[K_MOUSE1] = 0;
    assert(!editor.panning);
    held[K_MOUSE3] = 1;
    HUD_EditorKey(K_MOUSE3, true);
    cls.active = ACT_MINIMIZED;
    HUD_EditorMouse(mx, my);
    assert(!editor.panning);
    cls.active = ACT_ACTIVATED;
    held[K_MOUSE3] = 0;

    /* Text/modal input and toolbar space do not start view gestures. */
    editor.field = HUD_FIELD_FILTER;
    held[K_CTRL] = held[K_MOUSE3] = 1;
    float before_zoom = editor.preview_zoom;
    HUD_EditorKey(K_MWHEELUP, true);
    HUD_EditorKey(K_MOUSE3, true);
    assert(editor.preview_zoom == before_zoom && !editor.panning);
    editor.field = HUD_FIELD_NONE;
    editor.keys_open = true;
    HUD_EditorKey(K_MWHEELUP, true);
    HUD_EditorKey(K_MOUSE3, true);
    assert(editor.preview_zoom == before_zoom && !editor.panning);
    editor.keys_open = false;
    editor.changes_open = true;
    HUD_EditorKey(K_MWHEELUP, true);
    HUD_EditorKey(K_MOUSE3, true);
    assert(editor.preview_zoom == before_zoom && !editor.panning);
    editor.changes_open = false;
    uis.mouseCoords[0] = 10;
    uis.mouseCoords[1] = 10;
    HUD_EditorKey(K_MOUSE3, true);
    assert(!editor.panning);
    held[K_CTRL] = held[K_MOUSE3] = 0;
    for (int i = 0; i < setting_count; i++)
        assert(settings[i].value == settings[i].original && !settings[i].changed);
    assert(editor.history_count == 0 && writes == 0);

    /* A drag after zoom uses the inverse view transform, not fitted coordinates. */
    HUD_Select(HUD_EDIT_UPS);
    HUD_Command(HUD_BUTTON_SELECTION);
    editor.snap = false;
    mx = Q_rint(editor.view_x + 310 * editor.preview_zoom / editor.scale) / 2 * 2;
    my = Q_rint(editor.view_y + 174 * editor.preview_zoom / editor.scale) / 2 * 2;
    uis.mouseCoords[0] = mx * uis.scale;
    uis.mouseCoords[1] = my * uis.scale;
    HUD_EditorMouse(mx, my);
    held[K_MOUSE1] = 1;
    HUD_EditorKey(K_MOUSE1, true);
    assert(editor.dragging && !editor.panning);
    HUD_EditorMouse(mx + 80, my + 40);
    assert(HUD_Value("sh_ups_x") == 10 && HUD_Value("sh_ups_y") == 5);
    HUD_EditorKey(K_MOUSE1, false);
    held[K_MOUSE1] = 0;
    HUD_Undo();
    assert(HUD_Value("sh_ups_x") == 0 && HUD_Value("sh_ups_y") == 0 && writes == 0);
}

static void test_independent_visibility(void)
{
    /* Editor hiding must survive selection, HUD undo/redo and resets. */
    setup();
    hud_setting_t *ups = HUD_Setting("sh_ups");
    ups->original = ups->value = ups->var->value = 1;
    HUD_EditorKey('e', true);
    assert(!HUD_CanvasItem(HUD_EDIT_UPS) && ups->value == 1 && !ups->changed);
    assert(editor.history_count == 0 && writes == 0);
    HUD_Select(HUD_EDIT_STRAFE);
    HUD_Select(HUD_EDIT_UPS);
    assert(!HUD_CanvasItem(HUD_EDIT_UPS));
    HUD_EditorKey('v', true);
    assert(ups->value == 0 && ups->var->value == 1 && !HUD_CanvasItem(HUD_EDIT_UPS) && writes == 0);
    HUD_Undo();
    assert(ups->value == 1 && !HUD_CanvasItem(HUD_EDIT_UPS));
    HUD_Redo();
    assert(ups->value == 0 && !HUD_CanvasItem(HUD_EDIT_UPS));
    HUD_EditorKey(K_ENTER, true);
    assert(ups->var->value == 0 && writes == 1 && !editor.open);

    /* Editor Show can reveal disabled/unavailable samples without enabling them. */
    setup_layout();
    editor.selected = HUD_EDIT_UPS;
    editor.ghosts = false;
    HUD_Set("hud_timer_visible", 0);
    layout_available = false;
    assert(!HUD_CanvasItem(HUD_EDIT_LAYOUT_FIRST));
    HUD_SetEditorVisible(HUD_EDIT_LAYOUT_FIRST, true);
    editor.preview = true;
    assert(HUD_EditorShow(HUD_EDIT_LAYOUT_FIRST) && !HUD_Value("hud_timer_visible"));
    editor.bounds[HUD_EDIT_LAYOUT_FIRST] = (vrect_t) { 0 };
    HUD_EditorBounds(HUD_EDIT_LAYOUT_FIRST, 50, 60, 70, 8);
    HUD_SetEditorVisible(HUD_EDIT_LAYOUT_FIRST, false);
    assert(!HUD_EditorShow(HUD_EDIT_LAYOUT_FIRST) && editor.bounds[HUD_EDIT_LAYOUT_FIRST].width == 70);
    editor.preview = false;
    assert(HUD_EditorShow(HUD_EDIT_LAYOUT_FIRST));
    assert(!editor.history_count && writes == 0);
    layout_available = true;

    /* A locked element can be hidden/shown in the editor while its HUD is protected. */
    setup();
    ups = HUD_Setting("sh_ups");
    ups->original = ups->value = ups->var->value = 1;
    editor.locked[HUD_EDIT_UPS] = true;
    HUD_Command(HUD_BUTTON_EDITOR_VISIBLE);
    assert(!HUD_CanvasItem(HUD_EDIT_UPS));
    HUD_Command(HUD_BUTTON_VISIBLE);
    assert(ups->value == 1 && !ups->changed);
    HUD_Command(HUD_BUTTON_EDITOR_VISIBLE);
    assert(HUD_CanvasItem(HUD_EDIT_UPS));
    HUD_Select(HUD_EDIT_STRAFE);
    editor.ghosts = false;
    assert(HUD_CanvasItem(HUD_EDIT_UPS));
    HUD_Select(HUD_EDIT_UPS);
    HUD_Command(HUD_BUTTON_EDITOR_VISIBLE);
    HUD_Command(HUD_BUTTON_RESET_ALL);
    assert(!HUD_CanvasItem(HUD_EDIT_UPS) && ups->value == 1);
    HUD_EditorKey(K_ESCAPE, true);
    assert(ups->var->value == 1 && writes == 0);

    /* Hidden selected bounds and resize handles must not intercept canvas clicks. */
    setup();
    editor.hide_tools = true;
    HUD_SetEditorVisible(HUD_EDIT_UPS, false);
    editor.mouse_x = 310;
    editor.mouse_y = 174;
    HUD_EditorKey(K_MOUSE1, true);
    assert(!editor.dragging && !editor.resizing);
    editor.mouse_x = 324;
    editor.mouse_y = 178;
    HUD_EditorKey(K_MOUSE1, true);
    assert(!editor.dragging && !editor.resizing);
    editor.bounds[HUD_EDIT_NETWORK] = editor.bounds[HUD_EDIT_UPS];
    HUD_SetEditorVisible(HUD_EDIT_NETWORK, true);
    editor.mouse_x = 310;
    editor.mouse_y = 174;
    HUD_EditorKey(K_MOUSE1, true);
    assert(editor.selected == HUD_EDIT_NETWORK && editor.dragging);
    HUD_EditorKey(K_MOUSE1, false);

    /* Click the rendered row controls; Show must use the state before selecting. */
    setup();
    uis.width = 640;
    uis.height = 360;
    uis.scale = .5f;
    editor.focus = true;
    HUD_WorkbenchLayout();
    HUD_WorkbenchDraw();
    vrect_t button = editor.preview_toggles[HUD_EDIT_EFFICIENCY];
    assert(button.width > 0 && !HUD_CanvasItem(HUD_EDIT_EFFICIENCY));
    uis.mouseCoords[0] = (button.x + button.width / 2) * HUD_ToolsZoom();
    uis.mouseCoords[1] = (button.y + button.height / 2) * HUD_ToolsZoom();
    HUD_EditorKey(K_MOUSE1, true);
    assert(editor.selected == HUD_EDIT_EFFICIENCY && HUD_CanvasItem(HUD_EDIT_EFFICIENCY));
    assert(!HUD_Value("sh_efficiency") && !editor.history_count && writes == 0);
    HUD_WorkbenchDraw();
    button = editor.toggles[HUD_EDIT_EFFICIENCY];
    uis.mouseCoords[0] = (button.x + button.width / 2) * HUD_ToolsZoom();
    uis.mouseCoords[1] = (button.y + button.height / 2) * HUD_ToolsZoom();
    HUD_EditorKey(K_MOUSE1, true);
    assert(HUD_Value("sh_efficiency") == 1 && HUD_CanvasItem(HUD_EDIT_EFFICIENCY) && writes == 0);
    HUD_Command(HUD_BUTTON_EDITOR_VISIBLE);
    HUD_Command(HUD_BUTTON_VISIBLE);
    assert(!HUD_Value("sh_efficiency") && !HUD_CanvasItem(HUD_EDIT_EFFICIENCY));
    HUD_Undo();
    assert(HUD_Value("sh_efficiency") == 1 && !HUD_CanvasItem(HUD_EDIT_EFFICIENCY));
    HUD_EditorKey(K_ESCAPE, true);
    assert(HUD_Setting("sh_efficiency")->var->value == 0 && writes == 0);

    /* Preview choices are reset when opening a new editor session. */
    setup();
    editor.open = false;
    uis.activeMenu = NULL;
    uis.initialized = true;
    HUD_SetEditorVisible(HUD_EDIT_UPS, false);
    HUD_Open_f();
    assert(editor.editor_visibility[HUD_EDIT_UPS] == HUD_EDITOR_AUTO && HUD_CanvasItem(HUD_EDIT_UPS));
    HUD_EditorKey(K_ESCAPE, true);
    assert(writes == 0);
}

/* Regressions from the workbench review: selection, text input, locks and view. */
static void test_review_regressions(void)
{
    /* Undo keeps the undone element selected and reveals its collapsed row. */
    setup();
    HUD_Nudge(1, 0, false);
    editor.collapsed[HUD_Group(HUD_EDIT_UPS)] = true;
    HUD_Select(HUD_EDIT_NETWORK);
    HUD_Command(HUD_BUTTON_LOCK);
    HUD_Undo();
    assert(editor.selected == HUD_EDIT_NETWORK);
    HUD_Undo();
    assert(editor.selected == HUD_EDIT_UPS && HUD_ListItem(HUD_EDIT_UPS));
    assert(!editor.collapsed[HUD_Group(HUD_EDIT_UPS)] && HUD_Value("sh_ups_x") == 0);
    /* ...and clears a filter that hides it. */
    setup();
    HUD_Nudge(1, 0, false);
    IF_Replace(&editor.filter, "network");
    HUD_Undo();
    assert(editor.selected == HUD_EDIT_UPS && !editor.filter.text[0] && writes == 0);

    /* Keys are positional but chars follow the layout: QWERTZ 'y' types 'z'. */
    setup();
    HUD_EditorKey('y', true);
    HUD_EditorChar('z');
    HUD_EditorKey('y', false);
    assert(editor.field == HUD_FIELD_Y && !strcmp(editor.input.text, "170"));
    HUD_EditorKey(K_ESCAPE, true);
    HUD_EditorKey('/', true);
    HUD_EditorChar('-');
    HUD_EditorKey('/', false);
    assert(editor.field == HUD_FIELD_FILTER && !editor.filter.text[0]);
    HUD_EditorChar('u');
    assert(!strcmp(editor.filter.text, "u"));

    /* Revert restores the helper's own state but not efficiency-protected geometry. */
    setup();
    editor.bounds[HUD_EDIT_STRAFE] = (vrect_t) { 200, 170, 240, 8 };
    HUD_Select(HUD_EDIT_STRAFE);
    HUD_Nudge(0, 5, false);
    assert(HUD_Value("sh_y") == 5);
    editor.locked[HUD_EDIT_EFFICIENCY] = true;
    HUD_Revert(HUD_EDIT_STRAFE);
    assert(HUD_Value("sh_y") == 5 && editor.locked[HUD_EDIT_EFFICIENCY]);
    editor.locked[HUD_EDIT_EFFICIENCY] = false;
    HUD_Revert(HUD_EDIT_STRAFE);
    assert(HUD_Value("sh_y") == 0 && writes == 0);

    /* Tab and Shift+Tab from a filtered-out selection reach the first and last rows. */
    setup();
    IF_Replace(&editor.filter, "strafe");
    held[K_SHIFT] = 1;
    HUD_EditorKey(K_TAB, true);
    held[K_SHIFT] = 0;
    assert(editor.selected == HUD_EDIT_EFFICIENCY);
    editor.selected = HUD_EDIT_UPS;
    HUD_EditorKey(K_TAB, true);
    assert(editor.selected == HUD_EDIT_STRAFE);

    /* Snap tolerance is six screen pixels at every preview zoom. */
    setup();
    editor.preview_zoom = 1;
    HUD_Move(7, 0, true);
    assert(HUD_Value("sh_ups_x") == 8);
    setup();
    editor.preview_zoom = 4;
    HUD_Move(7, 0, true);
    assert(HUD_Value("sh_ups_x") == 7);

    /* Zooming beyond the canvas edge keeps a valid center, so panning cannot jump. */
    setup();
    uis.width = 1280;
    uis.height = 720;
    uis.scale = 1;
    editor.manual_zoom = .25f;
    HUD_WorkbenchLayout();
    int px = editor.stage.x + editor.stage.width - 1, py = editor.stage.y + editor.stage.height / 2;
    HUD_ZoomAt(4, px, py);
    assert(editor.center_x >= 0 && editor.center_x <= editor.width);
    float center = editor.center_x;
    uis.mouseCoords[0] = px;
    uis.mouseCoords[1] = py;
    held[K_MOUSE3] = 1;
    HUD_EditorKey(K_MOUSE3, true);
    assert(editor.panning);
    HUD_EditorMouse(px, py);
    assert(editor.center_x == center);
    held[K_MOUSE3] = 0;

    /* Native previews draw efficiency, helper, UPS, then network, regardless of IDs. */
    setup();
    for (int id = 0; id < HUD_EDIT_LAYOUT_FIRST; id++)
        editor.bounds[id] = (vrect_t) { 300, 170, 24, 8 };
    assert(HUD_CanvasHit(310, 174, false) == HUD_EDIT_NETWORK);
    editor.editor_visibility[HUD_EDIT_NETWORK] = HUD_EDITOR_HIDDEN;
    assert(HUD_CanvasHit(310, 174, false) == HUD_EDIT_UPS);
    editor.editor_visibility[HUD_EDIT_UPS] = HUD_EDITOR_HIDDEN;
    assert(HUD_CanvasHit(310, 174, false) == HUD_EDIT_STRAFE);
    editor.editor_visibility[HUD_EDIT_STRAFE] = HUD_EDITOR_HIDDEN;
    assert(HUD_CanvasHit(310, 174, false) == HUD_EDIT_EFFICIENCY);

    /* A locked element on top never blocks dragging the unlocked element beneath it. */
    setup();
    editor.bounds[HUD_EDIT_STRAFE] = (vrect_t) { 290, 160, 60, 30 };
    editor.bounds[HUD_EDIT_EFFICIENCY] = (vrect_t) { 10, 10, 20, 8 };
    editor.locked[HUD_EDIT_STRAFE] = true;
    for (int selected = HUD_EDIT_EFFICIENCY; selected >= HUD_EDIT_STRAFE; selected--) {
        editor.selected = selected;
        editor.mouse_x = 305;
        editor.mouse_y = 172;
        held[K_MOUSE1] = 1;
        HUD_EditorKey(K_MOUSE1, true);
        assert(editor.selected == HUD_EDIT_UPS && editor.dragging);
        held[K_MOUSE1] = 0;
        HUD_EditorKey(K_MOUSE1, false);
    }
    /* Where only the locked element is hit, it is still selected but not dragged. */
    editor.mouse_x = 340;
    editor.mouse_y = 185;
    held[K_MOUSE1] = 1;
    HUD_EditorKey(K_MOUSE1, true);
    assert(editor.selected == HUD_EDIT_STRAFE && !editor.dragging && writes == 0);
    held[K_MOUSE1] = 0;
}

int main(void)
{
    test_review_regressions();
    test_independent_visibility();
    test_scenarios_and_view_controls();
    test_shared_network_locks();
    test_element_information();
    test_preview_context();
    test_button_commands();
    test_workbench_draw_and_hold();
    test_workbench_fields();
    test_visual_scaling();
    test_workbench_cancel();
    test_workbench_preview();
    test_workbench_changes();
    test_game_availability();
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
    editor.bounds[HUD_EDIT_NETWORK] = (vrect_t) { 20, 20, 48, 48 };
    HUD_StartResize(); HUD_Resize(48, 48);
    assert(HUD_Value("hud_network_scale") == 2 && writes == 0);
    assert(!HUD_Setting("sh_lagometer_x")->changed && !HUD_Setting("sh_lagometer_y")->changed);

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
    assert(HUD_CanResize());
    HUD_EditorKey(K_TAB, true);
    assert(editor.selected == HUD_EDIT_NETWORK);
    HUD_EditorKey(K_PGDN, true);
    assert(editor.selected == HUD_EDIT_NETWORK);
    /* A fixed rail and command bar reserve a proportional preview at any size. */
    setup();
    uis.width = 1280;
    uis.height = 720;
    uis.scale = 1;
    HUD_WorkbenchLayout();
    assert(editor.stage.x >= editor.rail.width * HUD_ToolsZoom() / uis.scale - 1 && editor.stage.width > 900);
    assert(editor.stage.height < 600);
    assert(editor.dock.height >= 94);
    editor.dragging = true;
    assert(HUD_ToolsVisible());
    HUD_EditorKey(K_MOUSE1, false);
    HUD_EditorKey('h', true);
    assert(!HUD_ToolsVisible());
    HUD_EditorKey('h', true);
    assert(HUD_ToolsVisible());
    held[K_MOUSE1] = 1;
    cls.active = ACT_MINIMIZED;
    HUD_EditorMouse(20, 20);
    assert(!editor.dragging);
    /* Navigation skips removed groups and still reaches custom draw objects. */
    setup();
    extra_count = HL_BUILTIN_COUNT + 1;
    assert(HUD_Count() == HUD_EDIT_LAYOUT_FIRST + extra_count - 9);
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
    assert(editor.selected == HUD_EDIT_LAYOUT_FIRST + HL_INVENTORY);
    editor.selected = HUD_EDIT_LAYOUT_FIRST + HL_TIMELEFT;
    HUD_EditorKey(K_MWHEELDOWN, true);
    assert(editor.selected == HUD_EDIT_LAYOUT_FIRST + HL_HEALTH);
    HUD_EditorKey(K_PGDN, true);
    /* The six-row page includes Bind reminders in the Player state group. */
    assert(editor.selected == HUD_EDIT_LAYOUT_FIRST + HL_CHAT);
    /* Removed groups cannot intercept a click even with cached bounds. */
    editor.hide_tools = true;
    editor.mouse_x = 100; editor.mouse_y = 100;
    editor.bounds[HUD_EDIT_LAYOUT_FIRST + HL_CROSSHAIR] = (vrect_t) { 90, 90, 20, 20 };
    editor.bounds[HUD_EDIT_LAYOUT_FIRST + HL_OTHER_STATUS] = (vrect_t) { 90, 90, 20, 20 };
    HUD_EditorKey(K_MOUSE1, true);
    assert(!editor.dragging);
    /* Categories contain the right stable IDs and never mutate settings. */
    setup(); extra_count = HL_BUILTIN_COUNT + 1;
    HUD_EditorKey('2', true);
    assert(editor.category == HUD_CATEGORY_CLIENT && HUD_Count() == 17);
    assert(HUD_ListItem(HUD_EDIT_NETWORK));
    assert(HUD_ListItem(HUD_EDIT_LAYOUT_FIRST + HL_CHAT));
    held[K_SHIFT] = 1; HUD_EditorKey(K_TAB, true); held[K_SHIFT] = 0;
    assert(editor.selected == HUD_EDIT_LAYOUT_FIRST + HL_BUILTIN_COUNT);
    HUD_EditorKey('3', true);
    assert(editor.category == HUD_CATEGORY_SERVER && HUD_Count() == HL_SERVER_COUNT + 1);
    assert(editor.selected == HUD_EDIT_LAYOUT_FIRST + HL_SPEED);
    assert(HUD_ListItem(HUD_EDIT_LAYOUT_FIRST + HL_SCOREBOARD));
    assert(!HUD_ListItem(HUD_EDIT_LAYOUT_FIRST + HL_OTHER_STATUS));
    HUD_EditorKey('4', true); /* Old fourth tab shortcut does nothing. */
    assert(editor.category == HUD_CATEGORY_SERVER);
    HUD_EditorKey('1', true);
    assert(editor.selected == HUD_EDIT_LAYOUT_FIRST + HL_SPEED && writes == 0);
    /* An empty category remains visible and navigation safely does nothing. */
    setup(); HUD_EditorKey('3', true);
    assert(editor.category == HUD_CATEGORY_SERVER && HUD_Count() == 0);
    HUD_EditorKey(K_TAB, true);
    HUD_EditorKey(K_MWHEELDOWN, true);
    assert(editor.selected == HUD_EDIT_UPS && writes == 0);
    /* Paging follows the visible row count, including a short-window list. */
    setup(); extra_count = HL_BUILTIN_COUNT;
    editor.page_rows = 3;
    HUD_EditorKey(K_PGDN, true);
    assert(editor.selected == HUD_EDIT_LAYOUT_FIRST + HL_SPEED);
    HUD_EditorKey(K_PGUP, true);
    assert(editor.selected == HUD_EDIT_UPS);
    editor.page_rows = 0;
    HUD_EditorKey(K_PGDN, true);
    assert(editor.selected == HUD_EDIT_STRAFE);

    /* Overlay transitions invalidate cached controls before queued clicks. */
    setup(); HUD_EditorKey(K_RIGHTARROW, true);
    editor.buttons[HUD_BUTTON_ALIGN_LEFT] = (vrect_t) { 10, 10, 48, 18 };
    HUD_Command(HUD_BUTTON_CHANGES);
    assert(editor.changes_open && !editor.buttons[HUD_BUTTON_ALIGN_LEFT].width);
    HUD_EditorKey(K_ESCAPE, true);
    assert(!editor.changes_open && editor.open && HUD_Value("sh_ups_x") == 1);
    HUD_Command(HUD_BUTTON_KEYS);
    HUD_EditorKey(K_RIGHTARROW, true);
    assert(HUD_Value("sh_ups_x") == 1);
    HUD_EditorKey(K_ESCAPE, true);
    assert(editor.open && !editor.keys_open);
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
        vars[HUD_NATIVE_SETTINGS + 4] = (cvar_t) { 0 };
        vars[HUD_NATIVE_SETTINGS + 4].default_string = "0"; vars[HUD_NATIVE_SETTINGS + 4].value = 9;
        settings[HUD_NATIVE_SETTINGS + 4] = (hud_setting_t) { "hud_crosshair_x",
            HUD_EDIT_LAYOUT_FIRST + HL_CROSSHAIR, -16000, 16000, &vars[HUD_NATIVE_SETTINGS + 4], 9, 9, false };
        vars[HUD_NATIVE_SETTINGS + 5] = (cvar_t){ 0 };
        vars[HUD_NATIVE_SETTINGS + 5].default_string = "0";
        vars[HUD_NATIVE_SETTINGS + 5].value = 17;
        settings[HUD_NATIVE_SETTINGS + 5] = (hud_setting_t){ "hud_other_status_x",
                                                             HUD_EDIT_LAYOUT_FIRST + HL_OTHER_STATUS,
                                                             -16000,
                                                             16000,
                                                             &vars[HUD_NATIVE_SETTINGS + 5],
                                                             17,
                                                             17,
                                                             false };
        setting_count = HUD_NATIVE_SETTINGS + 6;
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
        assert(settings[HUD_NATIVE_SETTINGS + 4].value == 9 && !settings[HUD_NATIVE_SETTINGS + 4].changed);
        HUD_EditorKey(apply ? K_ENTER : K_ESCAPE, true);
        assert(ups->var->value == (apply ? 0 : 42));
        assert(vars[HUD_NATIVE_SETTINGS].value == (apply ? 0 : 25));
        assert(vars[HUD_NATIVE_SETTINGS + 2].value == (apply ? 1 : 0));
        assert(vars[HUD_NATIVE_SETTINGS + 4].value == 9);
        assert(settings[HUD_NATIVE_SETTINGS + 5].value == 17 && !settings[HUD_NATIVE_SETTINGS + 5].changed);
        assert(vars[HUD_NATIVE_SETTINGS + 5].value == 17);
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
char *va(const char *format, ...)
{
    static char buffers[8][1024];
    static unsigned next;
    char *buffer = buffers[next++ % 8];
    va_list args;
    va_start(args, format);
    vsnprintf(buffer, 1024, format, args);
    va_end(args);
    return buffer;
}

void Cmd_AddCommand(const char *name, xcommand_t fn) {}
void Cmd_RemoveCommand(const char *name) {}
int Q_strcasecmp(const char *a, const char *b)
{
    while (*a && Q_tolower(*a) == Q_tolower(*b)) { a++; b++; }
    return Q_tolower(*a) - Q_tolower(*b);
}

char *Q_strcasestr(const char *s1, const char *s2)
{
    for (size_t n = strlen(s2); *s1 || !n; s1++) {
        size_t i = 0;
        while (i < n && s1[i] && Q_tolower(s1[i]) == Q_tolower(s2[i]))
            i++;
        if (i == n)
            return (char *)s1;
    }
    return NULL;
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
void HUD_LayoutScaleAnchor(int id, float *x, float *y) { *x = 640; *y = 360; }
bool HUD_LayoutPreviewAvailable(int id)
{
    return layout_available;
}

bool HUD_LayoutCaptured(int id)
{
    return layout_captured;
}

qhandle_t R_RegisterImage(const char *name, imagetype_t type, imageflags_t flags)
{
    return 1;
}

bool R_GetPicSize(int *w, int *h, qhandle_t pic)
{
    *w = backdrop_width;
    *h = backdrop_height;
    return false;
}

void R_DrawStretchPic(int x, int y, int w, int h, qhandle_t pic)
{
    backdrop_draws++;
    backdrop_rect = (vrect_t){ x, y, w, h };
}

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

/* Use the real native field editor with only platform/render boundaries stubbed. */
#include "client/video.h"
const vid_driver_t *vid;
unsigned com_localTime;
static bool overstrike;
bool Key_GetOverstrikeMode(void)
{
    return overstrike;
}

void Key_SetOverstrikeMode(bool state)
{
    overstrike = state;
}

size_t Q_strlcpy(char *dst, const char *src, size_t size)
{
    size_t len = strlen(src);
    if (size) {
        size_t n = min(len, size - 1);
        memcpy(dst, src, n);
        dst[n] = 0;
    }
    return len;
}

void R_SetDrawTransform(float x, float y, float zoom)
{
}

void R_SetClipRect(const clipRect_t *clip)
{
}

void UI_SetColor_Wrapper(uint32_t color)
{
}

int R_DrawString(int x, int y, int flags, size_t maxlen, const char *text, qhandle_t font)
{
    return x;
}

void R_DrawChar(int x, int y, int flags, int ch, qhandle_t font)
{
}

void Com_Error(error_type_t code, const char *fmt, ...)
{
    abort();
}
