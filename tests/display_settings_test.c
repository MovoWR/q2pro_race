/* Offline tests execute the production controller, parser and menu with boundary stubs. */
#include "../src/client/display.c"
#include "../src/client/ui/video.c"
#undef NDEBUG
#include <assert.h>

uiStatic_t uis;
list_t ui_menus;
static cvar_t variables[64];
static int num_variables, allocations, apply_calls, fail_calls, activation_calls;
static unsigned now;
static bool connected[2] = { true, true };
static vid_display_settings_t current;
static int swap_value;
static bool wrong_readback;
static void FreeMenus(void);
client_static_t cls;
cmdbuf_t cmd_buffer;
int cvar_modified;
cvar_t *developer;

void Cbuf_AddText(cmdbuf_t *buffer, const char *text) { assert(!"unexpected command execution"); }
void Com_SetLastError(const char *message) { }
const char *Com_GetLastError(void) { return "mock renderer"; }
void Z_LeakTest(memtag_t tag) { }
void Prompt_AddMatch(genctx_t *ctx, const char *name) { }
bool R_Init(bool total) { swap_value = 0; return true; }
void R_Shutdown(bool total) { }
void UI_Init(void) { M_Menu_Video(); }
void UI_Shutdown(void) { FreeMenus(); }
void CL_RestartRefresh(bool total) { CL_ShutdownRefresh(); CL_InitRefresh(); }
void V_Init(void) { }
void V_Shutdown(void) { }
void Con_RegisterMedia(void) { }
void SCR_Init(void) { }
void SCR_Shutdown(void) { }
void SCR_RegisterMedia(void) { }

static const vid_display_t monitors[] = {
    { "DISPLAY1", "First", { 0, 0, 1920, 1080 }, 144, true },
    { "DISPLAY2", "Second", { -2560, -200, 2560, 1440 }, 120, false }
};

unsigned Sys_Milliseconds(void) { return now; }
void IN_Activate(void) { activation_calls++; }
void Com_LPrintf(print_type_t type, const char *format, ...) { }
void Com_Error(error_type_t code, const char *format, ...)
{
    va_list args;
    va_start(args, format);
    vfprintf(stderr, format, args);
    va_end(args);
    abort();
}

void *Z_Malloc(size_t size) { void *p = malloc(size); assert(p); allocations++; return p; }
void *Z_TagMalloc(size_t size, memtag_t tag) { return Z_Malloc(size); }
void *Z_TagMallocz(size_t size, memtag_t tag) { return memset(Z_Malloc(size), 0, size); }
void *Z_Realloc(void *p, size_t size)
{
    if (!p) allocations++;
    p = realloc(p, size);
    assert(p);
    return p;
}
void Z_Free(void *p) { if (p) { free(p); allocations--; } }
char *Z_TagCopyString(const char *s, memtag_t tag) { return strcpy(Z_Malloc(strlen(s) + 1), s); }

static char *Copy(const char *s)
{
    char *p = malloc(strlen(s) + 1);
    assert(p);
    return strcpy(p, s);
}

cvar_t *Cvar_FindVar(const char *name)
{
    for (int i = 0; i < num_variables; i++)
        if (!strcmp(variables[i].name, name)) return &variables[i];
    return NULL;
}
cvar_t *Cvar_Get(const char *name, const char *value, int flags)
{
    cvar_t *var = Cvar_FindVar(name);
    if (var) { var->flags |= flags; return var; }
    assert(num_variables < q_countof(variables));
    var = &variables[num_variables++];
    memset(var, 0, sizeof(*var));
    var->name = Copy(name);
    var->string = Copy(value);
    var->default_string = Copy(value);
    var->flags = flags;
    var->integer = atoi(value);
    var->value = strtof(value, NULL);
    return var;
}
void Cvar_SetByVar(cvar_t *var, const char *value, from_t from)
{
    if (!strcmp(var->string, value)) return;
    char *copy = Copy(value);
    free(var->string);
    var->string = copy;
    var->integer = atoi(value);
    var->value = strtof(value, NULL);
    if (var->changed) var->changed(var);
}
cvar_t *Cvar_Set(const char *name, const char *value)
{
    cvar_t *var = Cvar_Get(name, value, 0);
    Cvar_SetByVar(var, value, FROM_CODE);
    return var;
}
void Cvar_SetInteger(cvar_t *var, int value, from_t from)
{
    char buffer[32];
    snprintf(buffer, sizeof(buffer), "%d", value);
    Cvar_SetByVar(var, buffer, from);
}
const char *Cvar_VariableString(const char *name)
{
    cvar_t *var = Cvar_FindVar(name);
    return var ? var->string : "";
}
int Cvar_VariableInteger(const char *name) { return atoi(Cvar_VariableString(name)); }

static vid_display_t *GetDisplays(int *count)
{
    vid_display_t *list = Z_Malloc(sizeof(monitors));
    *count = 0;
    for (int i = 0; i < 2; i++)
        if (connected[i]) list[(*count)++] = monitors[i];
    return list;
}
static vid_display_resolution_t *GetModes(const char *display, int *count)
{
    static const vid_display_resolution_t first[] = {
        { 1920, 1080, 60 }, { 1920, 1080, 144 }, { 1280, 720, 60 }
    };
    static const vid_display_resolution_t second[] = {
        { 2560, 1440, 120 }, { 1920, 1080, 120 }
    };
    int i = !strcmp(display, monitors[1].id);
    *count = connected[i] ? (i ? q_countof(second) : q_countof(first)) : 0;
    vid_display_resolution_t *list = Z_Malloc(sizeof(first));
    memcpy(list, i ? second : first, *count * sizeof(*list));
    return list;
}
static bool GetSettings(vid_display_settings_t *settings)
{
    *settings = current;
    if (current.mode != VID_DISPLAY_WINDOWED)
        VID_GetGeometry(&settings->window);
    settings->vsync = swap_value;
    if (wrong_readback && current.mode == VID_DISPLAY_EXCLUSIVE)
        settings->mode = VID_DISPLAY_WINDOWED;
    return true;
}
static bool ApplySettings(const vid_display_settings_t *settings)
{
    apply_calls++;
    if (fail_calls) { fail_calls--; return false; }
    int monitor = -1;
    for (int i = 0; i < 2; i++)
        if (connected[i] && (!settings->display[0] || !strcmp(settings->display, monitors[i].id))) {
            monitor = i;
            break;
        }
    if (monitor < 0) return false;
    current = *settings;
    Q_strlcpy(current.display, monitors[monitor].id, sizeof(current.display));
    if (current.mode == VID_DISPLAY_BORDERLESS) {
        current.width = monitors[monitor].desktop.width;
        current.height = monitors[monitor].desktop.height;
        current.refresh = monitors[monitor].refresh;
    }
    return true;
}
static void SwapInterval(int value) { swap_value = value; }
static void PumpEvents(void) { }
static bool Probe(void) { return true; }
static char *ModeList(void) { return Z_CopyString("desktop 1920x1080@144"); }
static void SetMode(void)
{
    const vid_display_settings_t *pending = VID_PendingDisplaySettings();
    if (pending) {
        assert(ApplySettings(pending));
        SwapInterval(pending->vsync);
    }
}
#define MOCK_DRIVER_FIELDS \
    .probe = Probe, .get_mode_list = ModeList, .set_mode = SetMode, \
    .pump_events = PumpEvents, \
    .get_displays = GetDisplays, .get_display_modes = GetModes, \
    .get_display_settings = GetSettings, .apply_display_settings = ApplySettings, \
    .swap_interval = SwapInterval
static const vid_driver_t mock_driver = { MOCK_DRIVER_FIELDS };
const vid_driver_t vid_win32wgl = { .name = "win32wgl", MOCK_DRIVER_FIELDS };
const vid_driver_t vid_win32egl = { .name = "win32egl", MOCK_DRIVER_FIELDS };
const vid_driver_t vid_sdl = { .name = "sdl", MOCK_DRIVER_FIELDS };
const vid_driver_t vid_x11 = { .name = "x11", MOCK_DRIVER_FIELDS };
const vid_driver_t vid_wayland = { .name = "wayland", MOCK_DRIVER_FIELDS };

void Menu_AddItem(menuFrameWork_t *menu, void *ptr)
{
    menu->items = Z_Realloc(menu->items, (menu->nitems + 1) * sizeof(void *));
    menu->items[menu->nitems++] = ptr;
    ((menuCommon_t *)ptr)->parent = menu;
}
void Menu_Layout(menuFrameWork_t *menu)
{
    for (int i = 0; i < menu->nitems; i++) {
        menuCommon_t *item = menu->items[i];
        if (item->type == MTYPE_SPINCONTROL) {
            menuSpinControl_t *row = (void *)item;
            row->numItems = 0;
            assert(row->itemnames);
            while (row->itemnames[row->numItems]) row->numItems++;
        }
    }
}
void Menu_Init(menuFrameWork_t *menu) { Menu_Layout(menu); }
menuFrameWork_t *UI_FindMenu(const char *name)
{
    menuFrameWork_t *menu;
    LIST_FOR_EACH(menuFrameWork_t, menu, &ui_menus, entry)
        if (!strcmp(menu->name, name)) return menu;
    return NULL;
}
void UI_PushMenu(menuFrameWork_t *menu)
{
    assert(menu);
    for (int i = 0; i < uis.menuDepth; i++)
        if (uis.layers[i] == menu) { uis.menuDepth = i; break; }
    assert(uis.menuDepth < MAX_MENU_DEPTH);
    uis.layers[uis.menuDepth++] = menu;
    if (menu->push) assert(menu->push(menu));
    Menu_Init(menu);
    uis.activeMenu = menu;
}
void UI_PopMenu(void)
{
    assert(uis.menuDepth > 0);
    menuFrameWork_t *menu = uis.layers[--uis.menuDepth];
    if (menu->pop) menu->pop(menu);
    uis.activeMenu = uis.menuDepth ? uis.layers[uis.menuDepth - 1] : NULL;
}
static void FreeMenus(void)
{
    while (uis.menuDepth) UI_PopMenu();
    menuFrameWork_t *menu, *next;
    LIST_FOR_EACH_SAFE(menuFrameWork_t, menu, next, &ui_menus, entry)
        if (menu->free) menu->free(menu);
    List_Init(&ui_menus);
}
static void Reset(void)
{
    FreeMenus();
    assert(!allocations);
    for (int i = 0; i < num_variables; i++) {
        free(variables[i].name);
        free(variables[i].string);
        free(variables[i].default_string);
    }
    num_variables = 0;
    memset(&cls, 0, sizeof(cls));
    cvar_modified = 0;
    developer = Cvar_Get("developer", "0", 0);
    memset(&display_trial, 0, sizeof(display_trial));
    vid = &mock_driver;
    vid_fullscreen = Cvar_Get("vid_fullscreen", "0", CVAR_ARCHIVE);
    _vid_fullscreen = Cvar_Get("_vid_fullscreen", "2", CVAR_ARCHIVE);
    vid_modelist = Cvar_Get("vid_modelist", "desktop 1920x1080@144", 0);
    vid_geometry = Cvar_Get("vid_geometry", "800x600+100+200", CVAR_ARCHIVE);
    Cvar_Get("vid_monitor", "DISPLAY1", CVAR_ARCHIVE);
    Cvar_Get("_vid_fullscreen_borderless", "", CVAR_ARCHIVE);
    Cvar_Get("vid_noborder", "0", CVAR_ARCHIVE);
    Cvar_Get("gl_swapinterval", "0", CVAR_ARCHIVE);
    Cvar_Get("win_notitle", "0", 0);
    Cvar_Get("win_noresize", "0", 0);
    connected[0] = connected[1] = true;
    current = (vid_display_settings_t){
        .mode = VID_DISPLAY_WINDOWED, .display = "DISPLAY1",
        .window = { 100, 200, 800, 600 }, .width = 1920, .height = 1080, .refresh = 144
    };
    swap_value = 0;
    wrong_readback = false;
    now = 1000;
    apply_calls = fail_calls = activation_calls = 0;
}
static vid_display_settings_t Exclusive(void)
{
    vid_display_settings_t desired = current;
    desired.mode = VID_DISPLAY_EXCLUSIVE;
    strcpy(desired.display, "DISPLAY2");
    desired.width = 2560;
    desired.height = 1440;
    desired.refresh = 120;
    return desired;
}
static void Parser(void)
{
    Reset();
    vrect_t rect;
    int refresh, depth;
    assert(VID_GetFullscreenMode(1, &rect, &refresh, &depth));
    assert(rect.width == 0 && rect.height == 0 && refresh == 0);
    Cvar_SetInteger(vid_fullscreen, 1, FROM_CODE);
    assert(!VID_GetFullscreen(&rect, &refresh, NULL)); // preserve SDL desktop path
    assert(VID_GetFullscreenMode(2, &rect, &refresh, &depth));
    assert(rect.width == 1920 && refresh == 144);
    assert(!VID_GetFullscreenMode(3, &rect, &refresh, &depth));
    Cvar_Set("vid_modelist", "640x480:32@75 1920x1080@144:32 bad");
    assert(VID_GetFullscreenMode(1, &rect, &refresh, &depth) && refresh == 75 && depth == 32);
    assert(VID_GetFullscreenMode(2, &rect, &refresh, &depth) && refresh == 144 && depth == 32);
    assert(!VID_GetFullscreenMode(3, &rect, &refresh, &depth));
    Cvar_Set("vid_modelist", "0x0@9999:99");
    assert(!VID_GetFullscreenMode(1, &rect, &refresh, &depth));
}
static void ApplyKeepAndConfig(void)
{
    Reset();
    assert(VID_ApplyDisplaySettings(&current) && apply_calls == 0);
    vid_display_settings_t desired = Exclusive();
    assert(VID_ApplyDisplaySettings(&desired));
    assert(VID_DisplaySecondsLeft() == 15);
    assert(vid_fullscreen->integer == 0);
    assert(!strcmp(Cvar_VariableString("vid_monitor"), "DISPLAY1"));
    assert(!strcmp(vid_geometry->string, "800x600+100+200"));
    assert(!strcmp(vid_modelist->string, "desktop 1920x1080@144"));
    assert(!VID_ApplyDisplaySettings(&desired)); // no overlapping trials
    VID_KeepDisplaySettings();
    assert(!VID_PendingDisplaySettings());
    assert(vid_fullscreen->integer == 3 && _vid_fullscreen->integer == 3);
    assert(!strcmp(Cvar_VariableString("vid_monitor"), "DISPLAY2"));
    assert(vid_modelist->flags & CVAR_ARCHIVE);
    vrect_t rect; int refresh;
    assert(VID_GetFullscreen(&rect, &refresh, NULL) && rect.width == 2560 && refresh == 120);
    int calls = apply_calls;
    VID_KeepDisplaySettings();
    assert(calls == apply_calls);
}
static void RollbackAndClock(void)
{
    Reset();
    vid_display_settings_t original = current, desired = Exclusive();
    assert(VID_ApplyDisplaySettings(&desired));
    now += 14999;
    VID_DisplayFrame();
    assert(VID_DisplaySecondsLeft() == 1);
    now++;
    cls.ref_initialized = true;
    CL_RunRefresh();
    assert(!VID_PendingDisplaySettings());
    assert(VID_DisplaySettingsEqual(&current, &original));
    assert(vid_fullscreen->integer == 0);
    VID_RevertDisplaySettings(); // idempotent
    assert(apply_calls == 2);
    now = UINT_MAX - 5000;
    assert(VID_ApplyDisplaySettings(&desired));
    now += 15000;
    VID_DisplayFrame();
    assert(!VID_PendingDisplaySettings());
    assert(VID_DisplaySettingsEqual(&current, &original));
}
static void FailureAndHotplug(void)
{
    Reset();
    vid_display_settings_t original = current, desired = Exclusive();
    fail_calls = 1;
    assert(!VID_ApplyDisplaySettings(&desired));
    assert(!VID_PendingDisplaySettings());
    assert(VID_DisplaySettingsEqual(&current, &original));
    assert(strstr(VID_DisplayMessage(), "failed"));
    assert(VID_ApplyDisplaySettings(&desired));
    connected[1] = false;
    now += 250;
    VID_DisplayFrame();
    assert(!VID_PendingDisplaySettings() && current.mode == VID_DISPLAY_WINDOWED);

    connected[1] = true;
    assert(VID_ApplyDisplaySettings(&desired));
    connected[0] = false;
    VID_RevertDisplaySettings();
    assert(current.mode == VID_DISPLAY_WINDOWED && !strcmp(current.display, "DISPLAY2"));
    assert(current.window.width == 640 && current.window_flags == 0);
    assert(strstr(VID_DisplayMessage(), "unavailable"));
}
static void ReadbackAndWindowMemory(void)
{
    Reset();
    vid_display_settings_t desired = Exclusive();
    wrong_readback = true;
    assert(!VID_ApplyDisplaySettings(&desired));
    assert(!VID_PendingDisplaySettings() && vid_fullscreen->integer == 0);
    assert(strstr(VID_DisplayMessage(), "requested mode"));
    wrong_readback = false;
    current.window = (vrect_t){ 250, 275, 1024, 768 };
    assert(VID_ApplyDisplaySettings(&desired));
    VID_KeepDisplaySettings();
    assert(!strcmp(vid_geometry->string, "1024x768+250+275"));
    assert(VID_ToggleDisplay());
    VID_KeepDisplaySettings();
    assert(current.window.width == 1024 && current.window.x == 250);
    desired = Exclusive();
    assert(VID_ApplyDisplaySettings(&desired));
    current.mode = VID_DISPLAY_WINDOWED; // unexpected driver/state change before Keep
    VID_KeepDisplaySettings();
    assert(!VID_PendingDisplaySettings() && vid_fullscreen->integer == 0);
}

static void VsyncAndToggle(void)
{
    Reset();
    vid_display_settings_t desired = current;
    desired.vsync = 1;
    assert(VID_ApplyDisplaySettings(&desired));
    assert(!VID_PendingDisplaySettings() && swap_value == 1);
    assert(Cvar_VariableInteger("gl_swapinterval") == 1);
    assert(VID_ToggleDisplay());
    assert(current.mode == VID_DISPLAY_EXCLUSIVE);
    assert(current.width == 1920 && current.refresh == 144);
    assert(VID_ToggleDisplay()); // another Alt-Enter cancels the trial
    assert(current.mode == VID_DISPLAY_WINDOWED && !VID_PendingDisplaySettings());

    desired = current;
    desired.mode = VID_DISPLAY_BORDERLESS;
    assert(VID_ApplyDisplaySettings(&desired));
    VID_KeepDisplaySettings();
    assert(VID_ToggleDisplay());
    VID_KeepDisplaySettings();
    assert(current.mode == VID_DISPLAY_WINDOWED && current.window_flags == 0);
    assert(VID_ToggleDisplay());
    assert(current.mode == VID_DISPLAY_BORDERLESS);
    VID_RevertDisplaySettings();
}
static void MenuAndRestart(void)
{
    Reset();
    M_Menu_Video();
    UI_PushMenu(UI_FindMenu("video"));
    assert(video_menu.menu.nitems == 8);
    assert(video_menu.apply.generic.flags & QMF_GRAYED);
    video_menu.rows[DISPLAY_MODE].curvalue = VID_DISPLAY_BORDERLESS;
    Change(&video_menu.rows[DISPLAY_MODE].generic);
    assert(video_menu.rows[DISPLAY_SIZE].generic.flags & QMF_GRAYED);
    assert(video_menu.rows[DISPLAY_REFRESH].generic.flags & QMF_GRAYED);
    assert(strstr(video_menu.rows[DISPLAY_SIZE].itemnames[0], "Desktop"));
    assert(!apply_calls && vid_fullscreen->integer == 0);
    Action(&video_menu.back.generic);
    UI_PushMenu(UI_FindMenu("video"));
    assert(video_menu.draft.mode == VID_DISPLAY_WINDOWED); // Back discarded draft

    video_menu.rows[DISPLAY_MODE].curvalue = VID_DISPLAY_EXCLUSIVE;
    Change(&video_menu.rows[DISPLAY_MODE].generic);
    video_menu.rows[DISPLAY_MONITOR].curvalue = 1;
    Change(&video_menu.rows[DISPLAY_MONITOR].generic);
    assert(video_menu.num_rates == 1 && video_menu.rates[0] == 120);
    assert(!strcmp(video_menu.draft.display, "DISPLAY2"));
    Action(&video_menu.apply.generic);
    M_VideoFrame();
    assert(uis.activeMenu == &video_menu.confirm);
    assert(strstr(video_menu.countdown_text, "15"));

    FreeMenus(); // renderer/UI restart frees all menu allocations, preserving the trial
    assert(!allocations && VID_PendingDisplaySettings());
    now += 5000;
#ifdef _WIN32
    cls.ref_initialized = true;
    CL_ShutdownRefresh();
    CL_InitRefresh();
    assert(swap_value == VID_PendingDisplaySettings()->vsync);
#else
    M_Menu_Video();
#endif
    M_VideoFrame();
    assert(uis.activeMenu == &video_menu.confirm);
    assert(strstr(video_menu.countdown_text, "10"));
    ConfirmKey(&video_menu.confirm, K_ESCAPE);
    M_VideoFrame();
    assert(!VID_PendingDisplaySettings() && current.mode == VID_DISPLAY_WINDOWED);
    FreeMenus();
    assert(!allocations);
}

static void WindowSizePolling(void)
{
    for (int custom = 0; custom < 2; custom++) {
        Reset();
        if (custom) {
            current.window.width = 933;
            current.window.height = 701;
        }
        M_Menu_Video();
        UI_PushMenu(UI_FindMenu("video"));
        menuSpinControl_t *row = &video_menu.rows[DISPLAY_SIZE];
        vid_display_resolution_t sizes[16];
        int count = video_menu.num_sizes;
        assert(count > 2 && count <= q_countof(sizes));
        memcpy(sizes, video_menu.sizes, count * sizeof(*sizes));

        for (int press = 0; press < count * 2; press++) {
            int selected = (row->curvalue + 1) % row->numItems;
            row->curvalue = selected;
            row->generic.change(&row->generic);
            now += 1100; // A monitor poll occurs between successive key presses.
            M_VideoFrame();
            assert(row->curvalue == selected && video_menu.num_sizes == count);
            assert(video_menu.draft.window.width == sizes[selected].width);
            assert(video_menu.draft.window.height == sizes[selected].height);
            for (int i = 0; i < count; i++) {
                assert(video_menu.sizes[i].width == sizes[i].width);
                assert(video_menu.sizes[i].height == sizes[i].height);
            }
        }
        assert(!apply_calls);
        FreeMenus();
        assert(!allocations);
    }
}

int main(void)
{
    List_Init(&ui_menus);
    Parser();
    ApplyKeepAndConfig();
    RollbackAndClock();
    FailureAndHotplug();
    ReadbackAndWindowMemory();
    VsyncAndToggle();
    MenuAndRestart();
    WindowSizePolling();
    Reset();
    puts("Display parser, transaction, config, timeout, hotplug, toggle and menu tests passed.");
    return 0;
}
