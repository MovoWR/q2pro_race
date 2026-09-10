/* Win32 display tests: every window/display mutation is replaced by a simulator. */
#include "../src/windows/client.h"

static BOOL WINAPI TestEnumMonitors(HDC, LPCRECT, MONITORENUMPROC, LPARAM);
static BOOL WINAPI TestMonitorInfo(HMONITOR, LPMONITORINFO);
static HMONITOR WINAPI TestMonitorFromWindow(HWND, DWORD);
static HMONITOR WINAPI TestMonitorFromRect(LPCRECT, DWORD);
static BOOL WINAPI TestEnumSettings(LPCSTR, DWORD, DEVMODEA *, DWORD);
static BOOL WINAPI TestEnumDevices(LPCSTR, DWORD, PDISPLAY_DEVICEA, DWORD);
static LONG WINAPI TestChangeSettings(LPCSTR, DEVMODEA *, HWND, DWORD, LPVOID);
static LONG_PTR WINAPI TestGetStyle(HWND, int);
static LONG_PTR WINAPI TestSetStyle(HWND, int, LONG_PTR);
static BOOL WINAPI TestAdjustRect(LPRECT, DWORD, BOOL);
static BOOL WINAPI TestSetPosition(HWND, HWND, int, int, int, int, UINT);
static BOOL WINAPI TestWindowRect(HWND, LPRECT);
static BOOL WINAPI TestClientRect(HWND, LPRECT);
static int WINAPI TestMapPoints(HWND, HWND, LPPOINT, UINT);
static BOOL WINAPI TestWindowAction(HWND);
static HWND WINAPI TestFocus(HWND);
static BOOL WINAPI TestShowWindow(HWND, int);
static BOOL WINAPI TestPeekMessage(LPMSG, HWND, UINT, UINT, UINT);
static LRESULT WINAPI TestDefProc(HWND, UINT, WPARAM, LPARAM);
static UINT WINAPI TestDpi(HWND);

#define EnumDisplayMonitors TestEnumMonitors
#define GetMonitorInfoA TestMonitorInfo
#define MonitorFromWindow TestMonitorFromWindow
#define MonitorFromRect TestMonitorFromRect
#define EnumDisplaySettingsExA TestEnumSettings
#define EnumDisplayDevicesA TestEnumDevices
#define ChangeDisplaySettingsExA TestChangeSettings
#undef GetWindowLongPtr
#undef SetWindowLongPtr
#define GetWindowLongPtr TestGetStyle
#define SetWindowLongPtr TestSetStyle
#define AdjustWindowRect TestAdjustRect
#define SetWindowPos TestSetPosition
#define GetWindowRect TestWindowRect
#define GetClientRect TestClientRect
#define MapWindowPoints TestMapPoints
#define UpdateWindow TestWindowAction
#define SetForegroundWindow TestWindowAction
#define SetFocus TestFocus
#define ShowWindow TestShowWindow
#undef PeekMessage
#define PeekMessage TestPeekMessage
#undef DefWindowProc
#define DefWindowProc TestDefProc

#include "../src/windows/client.c"
#define main controller_test_main
#include "display_settings_test.c"
#undef main

HINSTANCE hGlobalInstance;
static DEVMODEA desktop_modes[2], active_modes[2];
static int change_count[2], restore_failures;
static LONG_PTR window_style;
static RECT outer_rect;
static int selected_monitor;
static bool position_fails, current_mode_unavailable;

static int MonitorIndex(const char *id)
{
    for (int i = 0; i < 2; i++)
        if (connected[i] && id && !strcmp(id, monitors[i].id))
            return i;
    return -1;
}
static BOOL WINAPI TestEnumMonitors(HDC dc, LPCRECT rect, MONITORENUMPROC callback, LPARAM data)
{
    for (int i = 0; i < 2; i++)
        if (connected[i] && !callback((HMONITOR)(intptr_t)(i + 1), NULL, NULL, data))
            break;
    return TRUE;
}
static BOOL WINAPI TestMonitorInfo(HMONITOR handle, LPMONITORINFO output)
{
    int i = (int)(intptr_t)handle - 1;
    if (i < 0 || i > 1 || !connected[i]) return FALSE;
    output->rcMonitor = (RECT){ monitors[i].desktop.x, monitors[i].desktop.y,
        monitors[i].desktop.x + active_modes[i].dmPelsWidth,
        monitors[i].desktop.y + active_modes[i].dmPelsHeight };
    output->rcWork = output->rcMonitor;
    output->rcWork.bottom -= 40;
    output->dwFlags = i == 0 ? MONITORINFOF_PRIMARY : 0;
    if (output->cbSize == sizeof(MONITORINFOEXA))
        strcpy(((MONITORINFOEXA *)output)->szDevice, monitors[i].id);
    return TRUE;
}
static HMONITOR WINAPI TestMonitorFromWindow(HWND wnd, DWORD flags)
{
    int i = connected[selected_monitor] ? selected_monitor : (connected[0] ? 0 : 1);
    return (HMONITOR)(intptr_t)(i + 1);
}
static HMONITOR WINAPI TestMonitorFromRect(LPCRECT rect, DWORD flags)
{
    int i = rect->left < 0 && connected[1] ? 1 : (connected[0] ? 0 : 1);
    return (HMONITOR)(intptr_t)(i + 1);
}
static BOOL WINAPI TestEnumSettings(LPCSTR id, DWORD index, DEVMODEA *mode, DWORD flags)
{
    int i = MonitorIndex(id);
    if (i < 0) return FALSE;
    if (index == ENUM_CURRENT_SETTINGS && current_mode_unavailable) return FALSE;
    if (index == ENUM_CURRENT_SETTINGS) { *mode = active_modes[i]; return TRUE; }
    if (index == ENUM_REGISTRY_SETTINGS) { *mode = desktop_modes[i]; return TRUE; }
    int count;
    vid_display_resolution_t *modes = GetModes(id, &count);
    bool found = index < count;
    if (found) {
        *mode = desktop_modes[i];
        mode->dmPelsWidth = modes[index].width;
        mode->dmPelsHeight = modes[index].height;
        mode->dmDisplayFrequency = modes[index].refresh;
    }
    Z_Free(modes);
    return found;
}
static BOOL WINAPI TestEnumDevices(LPCSTR id, DWORD index, PDISPLAY_DEVICEA device, DWORD flags)
{
    int i = MonitorIndex(id);
    if (i < 0 || index) return FALSE;
    strcpy(device->DeviceString, i ? "Second" : "First");
    return TRUE;
}
static LONG WINAPI TestChangeSettings(LPCSTR id, DEVMODEA *mode, HWND wnd, DWORD flags, LPVOID data)
{
    int i = MonitorIndex(id);
    if (i < 0 || !mode) return DISP_CHANGE_BADPARAM;
    if (!flags && restore_failures) {
        restore_failures--;
        return DISP_CHANGE_FAILED;
    }
    int count;
    vid_display_resolution_t *modes = GetModes(id, &count);
    bool supported = false;
    for (int j = 0; j < count; j++)
        if (modes[j].width == mode->dmPelsWidth && modes[j].height == mode->dmPelsHeight &&
            (!(mode->dmFields & DM_DISPLAYFREQUENCY) || modes[j].refresh == mode->dmDisplayFrequency))
            supported = true;
    Z_Free(modes);
    if (!supported) return DISP_CHANGE_BADMODE;
    if (!(flags & CDS_TEST)) {
        change_count[i]++;
        active_modes[i].dmPelsWidth = mode->dmPelsWidth;
        active_modes[i].dmPelsHeight = mode->dmPelsHeight;
        if (mode->dmFields & DM_BITSPERPEL)
            active_modes[i].dmBitsPerPel = mode->dmBitsPerPel;
        if (mode->dmFields & DM_DISPLAYFREQUENCY)
            active_modes[i].dmDisplayFrequency = mode->dmDisplayFrequency;
    }
    return DISP_CHANGE_SUCCESSFUL;
}
static LONG_PTR WINAPI TestGetStyle(HWND wnd, int index) { return window_style; }
static LONG_PTR WINAPI TestSetStyle(HWND wnd, int index, LONG_PTR value)
{
    LONG_PTR old = window_style; window_style = value; return old;
}
static BOOL WINAPI TestAdjustRect(LPRECT rect, DWORD style, BOOL menu)
{
    if (style & WS_CAPTION) { rect->left -= 8; rect->top -= 30; rect->right += 8; rect->bottom += 8; }
    return TRUE;
}
static BOOL WINAPI TestSetPosition(HWND wnd, HWND after, int x, int y, int width, int height, UINT flags)
{
    if (position_fails) return FALSE;
    outer_rect = (RECT){ x, y, x + width, y + height };
    selected_monitor = x < 0 ? 1 : 0;
    win.rc = (vrect_t){ x, y, width, height };
    if (window_style & WS_CAPTION) {
        int dpi = win.GetDpiForWindow ? win.GetDpiForWindow(wnd) : 96;
        win.rc.width -= 16 * dpi / 96;
        win.rc.height -= 38 * dpi / 96;
    }
    return TRUE;
}
static BOOL WINAPI TestWindowRect(HWND wnd, LPRECT rect) { *rect = outer_rect; return TRUE; }
static BOOL WINAPI TestClientRect(HWND wnd, LPRECT rect)
{
    *rect = (RECT){ 0, 0, win.rc.width, win.rc.height }; return TRUE;
}
static int WINAPI TestMapPoints(HWND from, HWND to, LPPOINT points, UINT count)
{
    for (UINT i = 0; i < count; i++) { points[i].x += win.rc.x; points[i].y += win.rc.y; }
    return 0;
}
static BOOL WINAPI TestWindowAction(HWND wnd) { return TRUE; }
static HWND WINAPI TestFocus(HWND wnd) { return wnd; }
static BOOL WINAPI TestShowWindow(HWND wnd, int command) { return TRUE; }
static BOOL WINAPI TestPeekMessage(LPMSG message, HWND wnd, UINT first, UINT last, UINT flags) { return FALSE; }
static LRESULT WINAPI TestDefProc(HWND wnd, UINT message, WPARAM wparam, LPARAM lparam) { return 0; }
static UINT WINAPI TestDpi(HWND wnd) { return selected_monitor == 1 ? 144 : 96; }
static BOOL WINAPI TestAdjustDpi(LPRECT rect, DWORD style, BOOL menu, DWORD ex_style, UINT dpi)
{
    if (style & WS_CAPTION) {
        rect->left -= 8 * dpi / 96;
        rect->top -= 30 * dpi / 96;
        rect->right += 8 * dpi / 96;
        rect->bottom += 8 * dpi / 96;
    }
    return TRUE;
}

// Unused input/lifecycle boundaries cannot launch the engine in this fixture.
void CL_Activate(active_t active) { }
void SCR_ModeChanged(void) { }
void R_ModeChanged(int width, int height, vidFlags_t flags) { }
void Key_Event(unsigned key, bool down, unsigned time) { }
void Key_Event2(unsigned key, bool down, unsigned time) { }
bool Key_IsWaitingForKey(void) { return false; }
int Key_IsDown(int key) { return 0; }
keydest_t Key_GetDest(void) { return KEY_MENU; }
void Key_CharEvent(int key) { }
void UI_MouseEvent(int x, int y) { }
void Com_Quit(const char *reason, error_type_t type) { abort(); }

static const vid_driver_t native_mock_driver = {
    .get_displays = Win_GetDisplays, .get_display_modes = Win_GetDisplayModes,
    .get_display_settings = Win_GetDisplaySettings, .apply_display_settings = Win_ApplyDisplaySettings,
    .swap_interval = SwapInterval, .pump_events = Win_PumpEvents
};

static void ResetWindows(void)
{
    Reset();
    memset(&win, 0, sizeof(win));
    memset(change_count, 0, sizeof(change_count));
    for (int i = 0; i < 2; i++) {
        desktop_modes[i] = (DEVMODEA){
            .dmSize = sizeof(DEVMODEA),
            .dmFields = DM_BITSPERPEL | DM_PELSWIDTH | DM_PELSHEIGHT | DM_DISPLAYFLAGS | DM_DISPLAYFREQUENCY | DM_POSITION,
            .dmBitsPerPel = 32, .dmPelsWidth = monitors[i].desktop.width,
            .dmPelsHeight = monitors[i].desktop.height, .dmDisplayFrequency = monitors[i].refresh
        };
        desktop_modes[i].dmPosition.x = monitors[i].desktop.x;
        desktop_modes[i].dmPosition.y = monitors[i].desktop.y;
        active_modes[i] = desktop_modes[i];
    }
    vid = &native_mock_driver;
    win.wnd = (HWND)1;
    vid_noborder = Cvar_FindVar("vid_noborder");
    win_notitle = Cvar_FindVar("win_notitle");
    win_noresize = Cvar_FindVar("win_noresize");
    win_alwaysontop = Cvar_Get("win_alwaysontop", "0", 0);
    win_noalttab = Cvar_Get("win_noalttab", "0", 0);
    win_menu_cursor = Cvar_Get("win_menu_cursor", "arrow", 0);
    vid_flip_on_switch = Cvar_Get("vid_flip_on_switch", "0", 0);
    restore_failures = selected_monitor = 0;
    position_fails = current_mode_unavailable = false;
    assert(Win_ApplyDisplaySettings(&current));
}
static void MonitorModesAndOwnership(void)
{
    ResetWindows();
    int count;
    vid_display_t *displays = Win_GetDisplays(&count);
    assert(count == 2 && displays[1].desktop.x == -2560 && displays[0].primary);
    Z_Free(displays);
    vid_display_resolution_t *modes = Win_GetDisplayModes("DISPLAY2", &count);
    assert(count == 2 && modes[0].refresh == 120);
    Z_Free(modes);

    vid_display_settings_t settings = Exclusive();
    settings.width = 1920; settings.height = 1080;
    assert(Win_ApplyDisplaySettings(&settings));
    assert(win.cds_fullscreen && win.rc.x == -2560 && win.rc.y == -200);
    assert(change_count[0] == 0 && change_count[1] == 1);
    displays = Win_GetDisplays(&count);
    assert(displays[1].desktop.width == 2560); // desktop, not temporary exclusive size
    Z_Free(displays);
    settings.mode = VID_DISPLAY_BORDERLESS;
    strcpy(settings.display, "DISPLAY1");
    assert(Win_ApplyDisplaySettings(&settings));
    assert(!win.cds_fullscreen && win.rc.width == 1920);
    assert(active_modes[1].dmPelsWidth == 2560 && change_count[0] == 0);
    assert(Win_UsesSystemCursor());

    settings.mode = VID_DISPLAY_WINDOWED;
    strcpy(settings.display, "DISPLAY2");
    assert(Win_ApplyDisplaySettings(&settings));
    assert(window_style & WS_CAPTION);
    assert(window_style & WS_THICKFRAME);
    assert(win.rc.x < 0 && win.rc.y >= -200);
    assert(win.rc.width == 800 && win.rc.height == 600);
}
static void NativeRollbackAndFailure(void)
{
    ResetWindows();
    vid_display_settings_t settings = Exclusive();
    settings.width = 1920; settings.height = 1080;
    assert(VID_ApplyDisplaySettings(&settings));
    assert(!Win_UsesSystemCursor());
    now += 15000;
    VID_DisplayFrame();
    assert(!VID_PendingDisplaySettings());
    assert(win.display_mode == VID_DISPLAY_WINDOWED && !win.cds_fullscreen);
    assert(active_modes[1].dmPelsWidth == 2560);
    assert(vid_fullscreen->integer == 0);

    settings = Exclusive();
    strcpy(settings.display, "DISPLAY1"); // first monitor does not offer 2560 x 1440
    assert(!VID_ApplyDisplaySettings(&settings));
    assert(win.display_mode == VID_DISPLAY_WINDOWED);
    assert(change_count[0] == 0);
    position_fails = true;
    settings = current;
    assert(!Win_ApplyDisplaySettings(&settings));
    position_fails = false;

    settings = Exclusive();
    settings.width = 1920; settings.height = 1080;
    assert(Win_ApplyDisplaySettings(&settings));
    restore_failures = 1;
    settings.mode = VID_DISPLAY_BORDERLESS;
    assert(!Win_ApplyDisplaySettings(&settings));
    assert(win.cds_fullscreen); // failed restoration retains ownership for retry
    assert(Win_ApplyDisplaySettings(&settings));
    assert(!win.cds_fullscreen);
}
static void DesktopMoveAndDisconnect(void)
{
    ResetWindows();
    vid_display_settings_t settings = current;
    settings.mode = VID_DISPLAY_BORDERLESS;
    assert(Win_ApplyDisplaySettings(&settings));
    selected_monitor = 1;
    outer_rect.left = -2500;
    pos_changed_event(win.wnd, NULL);
    assert(win.mode_changed & MODE_DISPLAY);
    Win_PumpEvents();
    assert(!strcmp(win.display_device, "DISPLAY2") && win.rc.width == 2560);
    assert(win.rc.x == -2560);
    connected[1] = false;
    win.mode_changed |= MODE_DISPLAY;
    Win_PumpEvents();
    assert(win.display_mode == VID_DISPLAY_WINDOWED);
    assert(window_style & WS_CAPTION);
    assert(win.rc.x >= 0);
}
static void DpiFocusAndLegacyDepth(void)
{
    ResetWindows();
    win.GetDpiForWindow = TestDpi;
    win.AdjustWindowRectExForDpi = TestAdjustDpi;
    vid_display_settings_t settings = current;
    strcpy(settings.display, "DISPLAY2");
    assert(Win_ApplyDisplaySettings(&settings));
    assert(win.rc.width == 800 && win.rc.height == 600);
    assert(outer_rect.right - outer_rect.left == 824);
    assert(Win_GetDpiScale() == 2);
    RECT suggested = { 50, 60, 866, 698 };
    Win_MainWndProc(win.wnd, WM_DPICHANGED, MAKELONG(96, 96), (LPARAM)&suggested);
    assert(win.rc.x == 50 && win.rc.y == 60 && win.rc.width == 800 && win.rc.height == 600);
    assert(win.mode_changed & MODE_SIZE);

    settings = Exclusive();
    settings.width = 1920; settings.height = 1080; settings.depth = 16;
    assert(Win_ApplyDisplaySettings(&settings));
    assert(active_modes[1].dmBitsPerPel == 16);
    Cvar_SetInteger(vid_flip_on_switch, 1, FROM_CODE);
    Win_Activate(WA_INACTIVE);
    assert(active_modes[1].dmPelsWidth == 2560 && active_modes[1].dmBitsPerPel == 32);
    Win_Activate(WA_ACTIVE);
    assert(active_modes[1].dmPelsWidth == 1920 && active_modes[1].dmBitsPerPel == 16);
    assert(change_count[0] == 0);
    settings.mode = VID_DISPLAY_WINDOWED;
    assert(Win_ApplyDisplaySettings(&settings));
    assert(active_modes[1].dmBitsPerPel == 32);
}

static void LegacyDesktopFallback(void)
{
    ResetWindows();
    Cvar_Set("vid_modelist", "bad");
    Cvar_SetInteger(vid_fullscreen, 1, FROM_CODE);
    Win_SetMode();
    assert(win.display_mode == VID_DISPLAY_EXCLUSIVE);
    assert(win.rc.width == 1920 && win.rc.height == 1080);
    assert(change_count[0] == 0 && change_count[1] == 0);
    assert(Win_RestoreDesktop());
}

static void NativeReadbackValidation(void)
{
    ResetWindows();
    vid_display_settings_t settings = Exclusive();
    settings.width = 1920;
    settings.height = 1080;
    assert(VID_ApplyDisplaySettings(&settings));

    // A desktop operation changes resolution without changing refresh rate.
    active_modes[1] = desktop_modes[1];
    Win_MainWndProc(win.wnd, WM_DISPLAYCHANGE, active_modes[1].dmBitsPerPel,
        MAKELPARAM(active_modes[1].dmPelsWidth, active_modes[1].dmPelsHeight));
    Win_PumpEvents();
    vid_display_settings_t actual;
    assert(Win_GetDisplaySettings(&actual));
    assert(actual.width == 2560 && actual.height == 1440);
    assert(actual.refresh == settings.refresh);
    VID_KeepDisplaySettings();
    assert(!VID_PendingDisplaySettings());
    assert(win.display_mode == VID_DISPLAY_WINDOWED && !win.cds_fullscreen);
    assert(vid_fullscreen->integer == 0);
    assert(!strcmp(vid_modelist->string, "desktop 1920x1080@144"));

    current_mode_unavailable = true;
    assert(!Win_GetDisplaySettings(&actual));
    current_mode_unavailable = false;
}

int main(void)
{
    List_Init(&ui_menus);
    MonitorModesAndOwnership();
    NativeRollbackAndFailure();
    DesktopMoveAndDisconnect();
    DpiFocusAndLegacyDepth();
    LegacyDesktopFallback();
    NativeReadbackValidation();
    assert(!allocations);
    puts("Win32 simulated monitor selection, modes, placement, rollback and hotplug passed.");
    return 0;
}
