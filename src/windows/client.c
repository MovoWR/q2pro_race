/*
Copyright (C) 1997-2001 Id Software, Inc.

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License along
with this program; if not, write to the Free Software Foundation, Inc.,
51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
*/

#include "client.h"
#include <hidusage.h>

win_state_t     win;

static cvar_t   *vid_flip_on_switch;
static cvar_t   *vid_hwgamma;
static cvar_t   *win_noalttab;
static cvar_t   *win_disablewinkey;
static cvar_t   *win_noresize;
static cvar_t   *win_notitle;
static cvar_t   *win_alwaysontop;
static cvar_t   *vid_noborder;
static cvar_t   *win_menu_cursor;
static HCURSOR  win_menu_cursor_handle;

typedef enum {
    WIN_MODE_WINDOWED,
    WIN_MODE_BORDERLESS_FULLSCREEN,
    WIN_MODE_EXCLUSIVE_FULLSCREEN
} win_video_mode_t;


static void     Win_ClipCursor(void);
static bool Win_FindDisplay(const char *id, MONITORINFOEXA *info);

static win_video_mode_t Win_TargetMode(void)
{
    if (!vid_fullscreen || vid_fullscreen->integer <= 0) {
        return WIN_MODE_WINDOWED;
    }
    if (vid_noborder && vid_noborder->integer) {
        return WIN_MODE_BORDERLESS_FULLSCREEN;
    }
    return WIN_MODE_EXCLUSIVE_FULLSCREEN;
}

static bool Win_MenuCursorEnabled(void)
{
    if (!win_menu_cursor) {
        return true;
    }

    if (!_stricmp(win_menu_cursor->string, "0") ||
        !_stricmp(win_menu_cursor->string, "off") ||
        !_stricmp(win_menu_cursor->string, "none")) {
        return false;
    }

    return true;
}

static bool Win_MenuCursorActive(void)
{
    return win.display_mode != VID_DISPLAY_EXCLUSIVE &&
        Win_MenuCursorEnabled() && (Key_GetDest() & (KEY_MENU | KEY_CONSOLE));
}

static bool Win_ShouldGrabMouse(bool requested)
{
    if (!requested) {
        return false;
    }
    if (Win_MenuCursorActive()) {
        return false;
    }
    return true;
}

/*
===============================================================================

COMMON WIN32 VIDEO RELATED ROUTINES

===============================================================================
*/

static void Win_SetPosition(void)
{
    LONG_PTR style = GetWindowLongPtr(win.wnd, GWL_STYLE);
    style &= ~(WS_OVERLAPPEDWINDOW | WS_POPUP | WS_DLGFRAME);
    HWND after = win_alwaysontop->integer || win.display_mode == VID_DISPLAY_EXCLUSIVE
        ? HWND_TOPMOST : HWND_NOTOPMOST;

    if (win.flags & QVF_FULLSCREEN) {
        style |= WS_POPUP;
    } else if (win.window_flags & VID_WINDOW_BORDERLESS) {
        style |= WS_POPUP | WS_MINIMIZEBOX | WS_MAXIMIZEBOX;
    } else if (win.window_flags & VID_WINDOW_NOTITLE) {
        style |= win.window_flags & VID_WINDOW_NORESIZE ? WS_DLGFRAME : WS_THICKFRAME;
    } else {
        style |= WS_OVERLAPPEDWINDOW;
        if (win.window_flags & VID_WINDOW_NORESIZE)
            style &= ~WS_THICKFRAME;
    }

    RECT rect = { 0, 0, win.rc.width, win.rc.height };
    AdjustWindowRect(&rect, (DWORD)style, FALSE);
    int x = win.rc.x, y = win.rc.y;
    int width = rect.right - rect.left, height = rect.bottom - rect.top;
    if (!(win.flags & QVF_FULLSCREEN)) {
        MONITORINFOEXA monitor = { .cbSize = sizeof(monitor) };
        if (Win_FindDisplay(win.display_device, &monitor)) {
            const RECT *work = &monitor.rcWork;
            width = min(width, work->right - work->left);
            height = min(height, work->bottom - work->top);
            x = max(work->left, min(work->right - width, x));
            y = max(work->top, min(work->bottom - height, y));
        }
    }

    SetWindowLongPtr(win.wnd, GWL_STYLE, style);
    SetWindowPos(win.wnd, after, x, y, width, height, SWP_FRAMECHANGED | SWP_SHOWWINDOW);
    UpdateWindow(win.wnd);
    SetForegroundWindow(win.wnd);
    SetFocus(win.wnd);
    if (win.mouse.grabbed)
        Win_ClipCursor();
}

/*
============
Win_ModeChanged
============
*/
static void Win_ModeChanged(void)
{
    R_ModeChanged(win.rc.width, win.rc.height, win.flags);
    SCR_ModeChanged();
}


static bool mode_is_sane(const DEVMODE *dm)
{
    // should have all these flags set
    if (~dm->dmFields & (DM_BITSPERPEL | DM_PELSWIDTH | DM_PELSHEIGHT | DM_DISPLAYFLAGS | DM_DISPLAYFREQUENCY))
        return false;

    // grayscale and interlaced modes are not supported
    if (dm->dmDisplayFlags & (DM_GRAYSCALE | DM_INTERLACED))
        return false;

    // according to MSDN, frequency can be 0 or 1 for some weird hardware
    if (dm->dmDisplayFrequency == 0 || dm->dmDisplayFrequency == 1)
        return false;

    return true;
}

static bool modes_are_equal(const DEVMODE *base, const DEVMODE *compare)
{
    if (!mode_is_sane(base))
        return false;

    if ((compare->dmFields & DM_PELSWIDTH) && base->dmPelsWidth != compare->dmPelsWidth)
        return false;

    if ((compare->dmFields & DM_PELSHEIGHT) && base->dmPelsHeight != compare->dmPelsHeight)
        return false;

    if ((compare->dmFields & DM_BITSPERPEL) && base->dmBitsPerPel != compare->dmBitsPerPel)
        return false;

    if ((compare->dmFields & DM_DISPLAYFREQUENCY) && base->dmDisplayFrequency != compare->dmDisplayFrequency)
        return false;

    return true;
}

/*
============
Win_GetModeList
============
*/
typedef struct {
    const char *id;
    MONITORINFOEXA *info;
    bool found;
} win_find_display_t;

static BOOL CALLBACK Win_FindDisplayCallback(HMONITOR monitor, HDC dc, LPRECT rect, LPARAM data)
{
    win_find_display_t *find = (void *)data;
    MONITORINFOEXA info = { .cbSize = sizeof(info) };
    if (GetMonitorInfoA(monitor, (MONITORINFO *)&info) && !strcmp(info.szDevice, find->id)) {
        *find->info = info;
        find->found = true;
        return FALSE;
    }
    return TRUE;
}

static bool Win_FindDisplay(const char *id, MONITORINFOEXA *info)
{
    if (!id || !*id)
        return GetMonitorInfoA(MonitorFromWindow(win.wnd, MONITOR_DEFAULTTOPRIMARY), (MONITORINFO *)info) != 0;
    win_find_display_t find = { id, info, false };
    EnumDisplayMonitors(NULL, NULL, Win_FindDisplayCallback, (LPARAM)&find);
    return find.found;
}

typedef struct {
    vid_display_t *items;
    int count;
} win_display_list_t;

static BOOL CALLBACK Win_ListDisplayCallback(HMONITOR monitor, HDC dc, LPRECT rect, LPARAM data)
{
    win_display_list_t *list = (void *)data;
    MONITORINFOEXA info = { .cbSize = sizeof(info) };
    DEVMODEA desktop = { .dmSize = sizeof(desktop) };
    if (!GetMonitorInfoA(monitor, (MONITORINFO *)&info) ||
        !EnumDisplaySettingsExA(info.szDevice, ENUM_CURRENT_SETTINGS, &desktop, 0))
        return TRUE;
    if (win.cds_fullscreen && !strcmp(win.display_device, info.szDevice))
        desktop = win.desktop_dm;
    list->items = Z_Realloc(list->items, (list->count + 1) * sizeof(*list->items));
    vid_display_t *item = &list->items[list->count++];
    memset(item, 0, sizeof(*item));
    Q_strlcpy(item->id, info.szDevice, sizeof(item->id));
    DISPLAY_DEVICEA device = { .cb = sizeof(device) };
    EnumDisplayDevicesA(info.szDevice, 0, &device, 0);
    Q_snprintf(item->name, sizeof(item->name), "%s%s",
               device.DeviceString[0] ? device.DeviceString : info.szDevice,
               info.dwFlags & MONITORINFOF_PRIMARY ? " (Primary)" : "");
    item->primary = (info.dwFlags & MONITORINFOF_PRIMARY) != 0;
    item->desktop = (vrect_t){ desktop.dmPosition.x, desktop.dmPosition.y,
                              desktop.dmPelsWidth, desktop.dmPelsHeight };
    item->refresh = desktop.dmDisplayFrequency;
    return TRUE;
}

vid_display_t *Win_GetDisplays(int *count)
{
    win_display_list_t list = { 0 };
    EnumDisplayMonitors(NULL, NULL, Win_ListDisplayCallback, (LPARAM)&list);
    *count = list.count;
    return list.items;
}

vid_display_resolution_t *Win_GetDisplayModes(const char *id, int *count)
{
    MONITORINFOEXA info = { .cbSize = sizeof(info) };
    DEVMODEA desktop = { .dmSize = sizeof(desktop) }, mode;
    vid_display_resolution_t *modes = NULL;
    *count = 0;
    if (!Win_FindDisplay(id, &info) ||
        !EnumDisplaySettingsExA(info.szDevice, ENUM_CURRENT_SETTINGS, &desktop, 0))
        return NULL;
    for (int i = 0; i < 4096; i++) {
        memset(&mode, 0, sizeof(mode));
        mode.dmSize = sizeof(mode);
        if (!EnumDisplaySettingsExA(info.szDevice, i, &mode, 0))
            break;
        if (!mode_is_sane(&mode) || mode.dmBitsPerPel != desktop.dmBitsPerPel ||
            mode.dmPelsWidth < 320 || mode.dmPelsHeight < 240 ||
            mode.dmPelsWidth > 8192 || mode.dmPelsHeight > 8192)
            continue;
        int j;
        for (j = 0; j < *count; j++)
            if (modes[j].width == mode.dmPelsWidth && modes[j].height == mode.dmPelsHeight &&
                modes[j].refresh == mode.dmDisplayFrequency)
                break;
        if (j != *count)
            continue;
        modes = Z_Realloc(modes, (*count + 1) * sizeof(*modes));
        modes[(*count)++] = (vid_display_resolution_t){ mode.dmPelsWidth, mode.dmPelsHeight, mode.dmDisplayFrequency };
    }
    return modes;
}

static void Win_RestoreDesktop(void)
{
    if (win.cds_fullscreen) {
        ChangeDisplaySettingsExA(win.display_device, &win.desktop_dm, NULL, 0, NULL);
        win.cds_fullscreen = false;
    }
}

bool Win_GetDisplaySettings(vid_display_settings_t *settings)
{
    if (!win.wnd)
        return false;
    memset(settings, 0, sizeof(*settings));
    MONITORINFOEXA info = { .cbSize = sizeof(info) };
    if (!Win_FindDisplay(win.flags & QVF_FULLSCREEN ? win.display_device : "", &info))
        return false;
    settings->mode = win.display_mode;
    Q_strlcpy(settings->display, info.szDevice, sizeof(settings->display));
    if (win.flags & QVF_FULLSCREEN)
        VID_GetGeometry(&settings->window);
    else
        settings->window = win.rc;
    settings->width = win.rc.width;
    settings->height = win.rc.height;
    DEVMODEA mode = { .dmSize = sizeof(mode) };
    if (EnumDisplaySettingsExA(info.szDevice, ENUM_CURRENT_SETTINGS, &mode, 0))
        settings->refresh = mode.dmDisplayFrequency;
    settings->window_flags = win.window_flags;
    settings->vsync = Cvar_VariableInteger("gl_swapinterval");
    return true;
}

char *Win_GetModeList(void)
{
    int count;
    vid_display_resolution_t *modes = Win_GetDisplayModes(Cvar_VariableString("vid_monitor"), &count);
    if (!count) {
        Z_Free(modes);
        return Z_CopyString(VID_MODELIST);
    }
    size_t size = 8 + count * 32 + 1;
    char *buffer = Z_Malloc(size);
    size_t length = Q_strlcpy(buffer, "desktop", size);
    for (int i = 0; i < count; i++)
        length += Q_scnprintf(buffer + length, size - length, " %dx%d@%d",
                             modes[i].width, modes[i].height, modes[i].refresh);
    Z_Free(modes);
    return buffer;
}

// Compare against the display owned by this window, including non-primary displays.
static bool mode_is_current(const DEVMODE *dm)
{
    DEVMODEA current = { .dmSize = sizeof(current) };
    return EnumDisplaySettingsExA(win.display_device, ENUM_CURRENT_SETTINGS, &current, 0) &&
        modes_are_equal(&current, dm);
}

static LONG set_fullscreen_mode(const vid_display_settings_t *settings)
{
    MONITORINFOEXA info = { .cbSize = sizeof(info) };
    DEVMODEA desktop = { .dmSize = sizeof(desktop) };
    if (!Win_FindDisplay(settings->display, &info))
        return DISP_CHANGE_BADMODE;
    if (win.cds_fullscreen && strcmp(win.display_device, info.szDevice))
        Win_RestoreDesktop();
    if (win.cds_fullscreen) {
        desktop = win.desktop_dm;
    } else if (!EnumDisplaySettingsExA(info.szDevice, ENUM_CURRENT_SETTINGS, &desktop, 0)) {
        return DISP_CHANGE_FAILED;
    }
    DEVMODEA mode = { .dmSize = sizeof(mode) };
    mode.dmFields = DM_PELSWIDTH | DM_PELSHEIGHT | DM_BITSPERPEL;
    mode.dmPelsWidth = settings->width ? settings->width : desktop.dmPelsWidth;
    mode.dmPelsHeight = settings->height ? settings->height : desktop.dmPelsHeight;
    mode.dmBitsPerPel = desktop.dmBitsPerPel;
    if (settings->refresh) {
        mode.dmFields |= DM_DISPLAYFREQUENCY;
        mode.dmDisplayFrequency = settings->refresh;
    }
    LONG result = ChangeDisplaySettingsExA(info.szDevice, &mode, NULL, CDS_TEST, NULL);
    if (result != DISP_CHANGE_SUCCESSFUL)
        return result;
    Q_strlcpy(win.display_device, info.szDevice, sizeof(win.display_device));
    if (!mode_is_current(&mode))
        result = ChangeDisplaySettingsExA(info.szDevice, &mode, NULL, CDS_FULLSCREEN, NULL);
    if (result != DISP_CHANGE_SUCCESSFUL)
        return result;
    win.desktop_dm = desktop;
    win.dm = mode;
    win.cds_fullscreen = true;
    win.display_mode = VID_DISPLAY_EXCLUSIVE;
    win.flags |= QVF_FULLSCREEN;
    win.rc = (vrect_t){ info.rcMonitor.left, info.rcMonitor.top, mode.dmPelsWidth, mode.dmPelsHeight };
    return DISP_CHANGE_SUCCESSFUL;
}

static bool set_borderless_fullscreen_mode(const vid_display_settings_t *settings)
{
    Win_RestoreDesktop();
    MONITORINFOEXA info = { .cbSize = sizeof(info) };
    if (!Win_FindDisplay(settings->display, &info))
        return false;
    Q_strlcpy(win.display_device, info.szDevice, sizeof(win.display_device));
    win.rc = (vrect_t){ info.rcMonitor.left, info.rcMonitor.top,
        info.rcMonitor.right - info.rcMonitor.left, info.rcMonitor.bottom - info.rcMonitor.top };
    win.display_mode = VID_DISPLAY_BORDERLESS;
    win.flags |= QVF_FULLSCREEN;
    memset(&win.dm, 0, sizeof(win.dm));
    return true;
}

bool Win_ApplyDisplaySettings(const vid_display_settings_t *settings)
{
    if (settings->mode < VID_DISPLAY_WINDOWED || settings->mode > VID_DISPLAY_EXCLUSIVE)
        return false;
    MONITORINFOEXA info = { .cbSize = sizeof(info) };
    if (!Win_FindDisplay(settings->display, &info))
        return false;
    win.applying_display = true;
    bool ok = true;
    if (settings->mode == VID_DISPLAY_EXCLUSIVE) {
        ok = set_fullscreen_mode(settings) == DISP_CHANGE_SUCCESSFUL;
    } else if (settings->mode == VID_DISPLAY_BORDERLESS) {
        ok = set_borderless_fullscreen_mode(settings);
    } else {
        if (settings->window.width < 320 || settings->window.height < 240 ||
            settings->window.width > 8192 || settings->window.height > 8192) {
            win.applying_display = false;
            return false;
        }
        Win_RestoreDesktop();
        win.display_mode = VID_DISPLAY_WINDOWED;
        win.flags &= ~QVF_FULLSCREEN;
        win.rc = settings->window;
        win.window_flags = settings->window_flags;
        Q_strlcpy(win.display_device, info.szDevice, sizeof(win.display_device));
        memset(&win.dm, 0, sizeof(win.dm));
    }
    if (ok) {
        Win_SetPosition();
        Win_ModeChanged();
        win.mode_changed = 0;
    }
    win.applying_display = false;
    return ok;
}


int Win_GetDpiScale(void)
{
    if (win.GetDpiForWindow) {
        int dpi = win.GetDpiForWindow(win.wnd);
        if (dpi) {
            int scale = (dpi + USER_DEFAULT_SCREEN_DPI / 2) / USER_DEFAULT_SCREEN_DPI;
            return Q_clip(scale, 1, 10);
        }
    }
    return 1;
}

/*
============
Win_SetMode
============
*/
void Win_SetMode(void)
{
    const vid_display_settings_t *pending = VID_PendingDisplaySettings();
    if (pending) {
        if (!Win_ApplyDisplaySettings(pending))
            VID_RevertDisplaySettings();
        else
            vid->swap_interval(pending->vsync);
        return;
    }

    vid_display_settings_t settings = { 0 };
    settings.mode = (vid_display_mode_t)Win_TargetMode();
    Q_strlcpy(settings.display, Cvar_VariableString("vid_monitor"), sizeof(settings.display));
    VID_GetGeometry(&settings.window);
    vrect_t fullscreen;
    VID_GetFullscreen(&fullscreen, &settings.refresh, NULL);
    settings.width = fullscreen.width;
    settings.height = fullscreen.height;
    settings.window_flags = (vid_noborder->integer ? VID_WINDOW_BORDERLESS : 0) |
        (win_notitle->integer ? VID_WINDOW_NOTITLE : 0) |
        (win_noresize->integer ? VID_WINDOW_NORESIZE : 0);
    if (Win_ApplyDisplaySettings(&settings))
        return;

    Com_WPrintf("Display settings unavailable; restoring a window on an available monitor.\n");
    settings.mode = VID_DISPLAY_WINDOWED;
    settings.display[0] = 0;
    settings.window_flags = 0;
    Cvar_SetInteger(vid_fullscreen, 0, FROM_CODE);
    Win_ApplyDisplaySettings(&settings);
}

/*
============
Win_UpdateGamma
============
*/
void Win_UpdateGamma(const byte *table)
{
    WORD v;
    int i;

    if (win.flags & QVF_GAMMARAMP) {
        for (i = 0; i < 256; i++) {
            v = table[i] << 8;
            win.gamma_cust[0][i] = v;
            win.gamma_cust[1][i] = v;
            win.gamma_cust[2][i] = v;
        }

        SetDeviceGammaRamp(win.dc, win.gamma_cust);
    }
}

static void Win_DisableAltTab(void)
{
    if (!win.alttab_disabled) {
        RegisterHotKey(0, 0, MOD_ALT, VK_TAB);
        RegisterHotKey(0, 1, MOD_ALT, VK_RETURN);
        win.alttab_disabled = true;
    }
}

static void Win_EnableAltTab(void)
{
    if (win.alttab_disabled) {
        UnregisterHotKey(0, 0);
        UnregisterHotKey(0, 1);
        win.alttab_disabled = false;
    }
}

static void win_noalttab_changed(cvar_t *self)
{
    if (self->integer) {
        Win_DisableAltTab();
    } else {
        Win_EnableAltTab();
    }
}

static void Win_Activate(WPARAM wParam)
{
    active_t active;

    if (HIWORD(wParam)) {
        // we don't want to act like we're active if we're minimized
        active = ACT_MINIMIZED;
    } else if (LOWORD(wParam)) {
        active = ACT_ACTIVATED;
    } else {
        active = ACT_RESTORED;
    }

    CL_Activate(active);

    if (win_noalttab->integer) {
        if (active == ACT_ACTIVATED) {
            Win_EnableAltTab();
        } else {
            Win_DisableAltTab();
        }
    }

    if (win.flags & QVF_GAMMARAMP) {
        if (active == ACT_ACTIVATED) {
            SetDeviceGammaRamp(win.dc, win.gamma_cust);
        } else {
            SetDeviceGammaRamp(win.dc, win.gamma_orig);
        }
    }

    if (win.flags & QVF_FULLSCREEN) {
        if (active == ACT_ACTIVATED) {
            ShowWindow(win.wnd, SW_RESTORE);
        }

        if (win.cds_fullscreen && vid_flip_on_switch->integer) {
            if (active == ACT_ACTIVATED) {
                if (!mode_is_current(&win.dm)) {
                    ChangeDisplaySettingsExA(win.display_device, &win.dm, NULL, CDS_FULLSCREEN, NULL);
                }
            } else {
                ChangeDisplaySettingsExA(win.display_device, &win.desktop_dm, NULL, 0, NULL);
            }
        }
    }

    if (active == ACT_ACTIVATED) {
        SetForegroundWindow(win.wnd);
    }
}

static LRESULT CALLBACK LowLevelKeyboardProc(int nCode, WPARAM wParam, LPARAM lParam)
{
    PKBDLLHOOKSTRUCT kb = (PKBDLLHOOKSTRUCT)lParam;
    unsigned key;

    if (nCode != HC_ACTION) {
        goto ignore;
    }

    switch (kb->vkCode) {
    case VK_LWIN:
        key = K_LWINKEY;
        break;
    case VK_RWIN:
        key = K_RWINKEY;
        break;
    default:
        goto ignore;
    }

    switch (wParam) {
    case WM_KEYDOWN:
        Key_Event(key, true, kb->time);
        return TRUE;
    case WM_KEYUP:
        Key_Event(key, false, kb->time);
        return TRUE;
    default:
        break;
    }

ignore:
    return CallNextHookEx(NULL, nCode, wParam, lParam);
}

static void win_disablewinkey_changed(cvar_t *self)
{
    if (self->integer) {
        win.kbdHook = SetWindowsHookEx(WH_KEYBOARD_LL, LowLevelKeyboardProc, hGlobalInstance, 0);
        if (!win.kbdHook) {
            Com_EPrintf("Couldn't set low-level keyboard hook, error %#lX\n", GetLastError());
            Cvar_Set("win_disablewinkey", "0");
        }
    } else {
        if (win.kbdHook) {
            UnhookWindowsHookEx(win.kbdHook);
            win.kbdHook = NULL;
        }
    }
}

static const byte scantokey[2][96] = {
    {
//      0               1           2           3               4           5               6           7
//      8               9           A           B               C           D               E           F
        0,              K_ESCAPE,   '1',        '2',            '3',        '4',            '5',        '6',
        '7',            '8',        '9',        '0',            '-',        '=',            K_BACKSPACE,K_TAB,      // 0
        'q',            'w',        'e',        'r',            't',        'y',            'u',        'i',
        'o',            'p',        '[',        ']',            K_ENTER,    K_LCTRL,        'a',        's',        // 1
        'd',            'f',        'g',        'h',            'j',        'k',            'l',        ';',
        '\'',           '`',        K_LSHIFT,   '\\',           'z',        'x',            'c',        'v',        // 2
        'b',            'n',        'm',        ',',            '.',        '/',            K_RSHIFT,   K_KP_MULTIPLY,
        K_LALT,         K_SPACE,    K_CAPSLOCK, K_F1,           K_F2,       K_F3,           K_F4,       K_F5,       // 3
        K_F6,           K_F7,       K_F8,       K_F9,           K_F10,      K_PAUSE,        K_SCROLLOCK,K_KP_HOME,
        K_KP_UPARROW,   K_KP_PGUP,  K_KP_MINUS, K_KP_LEFTARROW, K_KP_5,     K_KP_RIGHTARROW,K_KP_PLUS,  K_KP_END,   // 4
        K_KP_DOWNARROW, K_KP_PGDN,  K_KP_INS,   K_KP_DEL,       0,          0,              K_102ND,    K_F11,
        K_F12,          0,          0,          0,              0,          0,              0,          0,          // 5
    },
    {
        0,              0,          0,          0,              0,          0,              0,          0,
        0,              0,          0,          0,              0,          0,              0,          0,          // 0
        0,              0,          0,          0,              0,          0,              0,          0,
        0,              0,          0,          0,              K_KP_ENTER, K_RCTRL,        0,          0,          // 1
        0,              0,          0,          0,              0,          0,              0,          0,
        0,              0,          0,          0,              0,          0,              0,          0,          // 2
        0,              0,          0,          0,              0,          K_KP_SLASH,     0,          K_PRINTSCREEN,
        K_RALT,         0,          0,          0,              0,          0,              0,          0,          // 3
        0,              0,          0,          0,              0,          K_NUMLOCK,      0,          K_HOME,
        K_UPARROW,      K_PGUP,     0,          K_LEFTARROW,    0,          K_RIGHTARROW,   0,          K_END,      // 4
        K_DOWNARROW,    K_PGDN,     K_INS,      K_DEL,          0,          0,              0,          0,
        0,              0,          0,          K_LWINKEY,      K_RWINKEY,  K_MENU,         0,          0,          // 5
    }
};

static int key_from_lparam(LPARAM lParam)
{
    int scancode = (lParam >> 16) & 255;
    int extended = (lParam >> 24) & 1;

    if ((unsigned)scancode >= q_countof(scantokey[0]))
        return 0;

    return scantokey[extended][scancode];
}

// scantokey[0] read backwards: canonical keynum -> the scancode it came from.
// only non-extended scancodes matter, since those are the printable keys whose
// labels move around between layouts.
static byte keytoscan[256];

// cache of canonical keynum -> character printed on that physical key under the
// active keyboard layout. rebuilt lazily; dropped on WM_INPUTLANGCHANGE.
static byte keylabels[256];
static bool keylabels_valid;

static void build_key_labels(void)
{
    memset(keytoscan, 0, sizeof(keytoscan));
    memset(keylabels, 0, sizeof(keylabels));

    // invert scantokey so labels can be looked up per keynum
    for (int sc = 0; sc < 96; sc++) {
        int key = scantokey[0][sc];
        if (key && !keytoscan[key])
            keytoscan[key] = sc;
    }

    for (int key = 0; key < 256; key++) {
        if (!keytoscan[key])
            continue;

        UINT vk = MapVirtualKey(keytoscan[key], MAPVK_VSC_TO_VK);
        if (!vk)
            continue;

        UINT ch = MapVirtualKey(vk, MAPVK_VK_TO_CHAR);

        // high bit flags a dead key (accents on many European layouts); those
        // have no standalone label worth showing, so leave them canonical.
        if (ch & 0x80000000)
            continue;

        ch = Q_tolower(ch & 0xffff);

        // only relabel to printable ASCII. anything else -- umlauts, ß, and the
        // rest of the non-ASCII layout keys -- has no keynum of its own and no
        // guaranteed glyph in the conchars font, so it stays canonical.
        if (ch > 32 && ch < 127)
            keylabels[key] = ch;
    }

    keylabels_valid = true;
}

int Win_GetKeyLabel(int keynum)
{
    if (keynum < 0 || keynum > 255)
        return 0;

    if (!keylabels_valid)
        build_key_labels();

    // no point reporting a label identical to the canonical name
    if (keylabels[keynum] == keynum)
        return 0;

    return keylabels[keynum];
}

// Map from windows to quake keynums
static void legacy_key_event(WPARAM wParam, LPARAM lParam, bool down)
{
    int scancode = (lParam >> 16) & 255;
    int extended = (lParam >> 24) & 1;
    int result = key_from_lparam(lParam);

    if (!result) {
        Com_DPrintf("%s: unknown %sscancode %d\n",
                    __func__, extended ? "extended " : "", scancode);
        return;
    }

    Key_Event2(result, down, win.lastMsgTime);
}

static bool should_translate_message(const MSG *msg)
{
    if (msg->hwnd != win.wnd)
        return true;

    if (msg->message != WM_KEYDOWN && msg->message != WM_SYSKEYDOWN)
        return true;

    // Character translation is useful only for console, chat, and menu fields.
    // In particular, do not let the key that opens a text destination type into
    // the destination it just opened.
    if (!(Key_GetDest() & (KEY_CONSOLE | KEY_MESSAGE | KEY_MENU)))
        return false;

    // A captured binding is a physical key, not text. Translating it would
    // enqueue a character after the binding callback has already completed.
    if (Key_IsWaitingForKey())
        return false;

    // The unshifted physical console key is consumed by Key_Event. On layouts
    // where that key is a dead accent, translating it would also poison the
    // next character entered after the console opens.
    int key = key_from_lparam(msg->lParam);
    if (!Key_IsDown(K_SHIFT) && (key == '`' || key == '~'))
        return false;

    return true;
}

static void char_event(WPARAM wParam, LPARAM lParam)
{
    unsigned repeat = LOWORD(lParam);

    if (!repeat)
        repeat = 1;

    while (repeat--)
        Key_CharEvent((int)wParam);
}

static void mouse_wheel_event(int delta)
{
    UINT lines, key;

    // FIXME: handle WHEEL_DELTA and partial scrolls...
    if (delta > 0) {
        key = K_MWHEELUP;
    } else if (delta < 0) {
        key = K_MWHEELDOWN;
    } else {
        return;
    }

    if (Key_GetDest() & KEY_CONSOLE) {
        SystemParametersInfo(SPI_GETWHEELSCROLLLINES, 0, &lines, 0);
        lines = Q_clip(lines, 1, 9);
    } else {
        lines = 1;
    }

    do {
        Key_Event(key, true, win.lastMsgTime);
        Key_Event(key, false, win.lastMsgTime);
    } while (--lines);
}

static void mouse_hwheel_event(int delta)
{
    UINT key;

    // FIXME: handle WHEEL_DELTA and partial scrolls...
    if (delta > 0) {
        key = K_MWHEELRIGHT;
    } else if (delta < 0) {
        key = K_MWHEELLEFT;
    } else {
        return;
    }

    Key_Event(key, true, win.lastMsgTime);
    Key_Event(key, false, win.lastMsgTime);
}

// returns TRUE if mouse cursor inside client area
static BOOL check_cursor_pos(void)
{
    POINT pt;

    if (win.mouse.grabbed)
        return TRUE;

    if (!GetCursorPos(&pt))
        return FALSE;

    return PtInRect(&win.screen_rc, pt);
}

#define BTN_DN(i)   BIT((i) * 2 + 0)
#define BTN_UP(i)   BIT((i) * 2 + 1)

static void raw_mouse_event(const RAWMOUSE *rm)
{
    int i;

    if (!check_cursor_pos()) {
        // cursor is over non-client area
        // perform just button up actions
        for (i = 0; i < MOUSE_BUTTONS; i++) {
            if (rm->usButtonFlags & BTN_UP(i)) {
                Key_Event(K_MOUSE1 + i, false, win.lastMsgTime);
            }
        }
        return;
    }

    if (rm->usButtonFlags) {
        // perform button actions
        for (i = 0; i < MOUSE_BUTTONS; i++) {
            if (rm->usButtonFlags & BTN_DN(i)) {
                Key_Event(K_MOUSE1 + i, true, win.lastMsgTime);
            }
            if (rm->usButtonFlags & BTN_UP(i)) {
                Key_Event(K_MOUSE1 + i, false, win.lastMsgTime);
            }
        }

        if (rm->usButtonFlags & RI_MOUSE_WHEEL) {
            mouse_wheel_event((short)rm->usButtonData);
        }

        if (rm->usButtonFlags & RI_MOUSE_HWHEEL) {
            mouse_hwheel_event((short)rm->usButtonData);
        }
    }

    if ((rm->usFlags & (MOUSE_MOVE_RELATIVE | MOUSE_MOVE_ABSOLUTE)) == MOUSE_MOVE_RELATIVE) {
        win.mouse.mx += rm->lLastX;
        win.mouse.my += rm->lLastY;
    }
}

static void raw_input_event(HRAWINPUT handle)
{
    BYTE buffer[64];
    UINT len, ret;
    PRAWINPUT ri;

    len = sizeof(buffer);
    ret = GetRawInputData(handle, RID_INPUT, buffer, &len, sizeof(RAWINPUTHEADER));
    if (ret == (UINT)-1) {
        Com_EPrintf("GetRawInputData failed with error %#lx\n", GetLastError());
        return;
    }

    ri = (PRAWINPUT)buffer;
    if (ri->header.dwType == RIM_TYPEMOUSE) {
        raw_mouse_event(&ri->data.mouse);
    }
}

static int get_window_dpi(void)
{
    if (win.GetDpiForWindow) {
        int dpi = win.GetDpiForWindow(win.wnd);
        if (dpi)
            return dpi;
    }
    return USER_DEFAULT_SCREEN_DPI;
}

static void pos_changing_event(HWND wnd, WINDOWPOS *pos)
{
    LONG_PTR style;
    RECT rc;
    int dpi;

    if (win.flags & QVF_FULLSCREEN)
        return;

    if (pos->flags & SWP_NOSIZE)
        return;

    style = GetWindowLongPtr(wnd, GWL_STYLE);
    dpi = get_window_dpi();

    // calculate size of non-client area
    rc.left = 0;
    rc.top = 0;
    rc.right = MulDiv(320, dpi, USER_DEFAULT_SCREEN_DPI);
    rc.bottom = MulDiv(240, dpi, USER_DEFAULT_SCREEN_DPI);

    AdjustWindowRect(&rc, (DWORD)style, FALSE);

    // don't allow too small size
    pos->cx = max(pos->cx, rc.right - rc.left);
    pos->cy = max(pos->cy, rc.bottom - rc.top);
}

static void pos_changed_event(HWND wnd, const WINDOWPOS *pos)
{
    RECT rc;

    // get window position
    GetWindowRect(wnd, &rc);
    win.rc.x = rc.left;
    win.rc.y = rc.top;

    // get size of client area
    GetClientRect(wnd, &rc);
    win.rc.width = rc.right - rc.left;
    win.rc.height = rc.bottom - rc.top;

    // get rectangle of client area in screen coordinates
    MapWindowPoints(wnd, NULL, (POINT *)&rc, 2);
    win.screen_rc = rc;
    win.center_x = (rc.right + rc.left) / 2;
    win.center_y = (rc.top + rc.bottom) / 2;

    // set mode_changed flags unless in full screen
    if (win.flags & QVF_FULLSCREEN)
        return;

    if (!pos) {
        win.mode_changed |= MODE_STYLE;
        return;
    }

    if (!(pos->flags & SWP_NOSIZE))
        win.mode_changed |= MODE_SIZE;

    if (!(pos->flags & SWP_NOMOVE))
        win.mode_changed |= MODE_POS;
}

// main window procedure
static LRESULT WINAPI Win_MainWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg) {
    case WM_MOUSEMOVE:
        if (win.mouse.initialized)
            UI_MouseEvent((short)LOWORD(lParam), (short)HIWORD(lParam));
        break;

    case WM_SETCURSOR:
        if (LOWORD(lParam) == HTCLIENT) {
            if (!win.mouse.grabbed && Win_MenuCursorActive() && win_menu_cursor_handle) {
                SetCursor(win_menu_cursor_handle);
                return TRUE;
            }
        }
        break;

    case WM_HOTKEY:
        return FALSE;

    case WM_INPUT:
        if (wParam == RIM_INPUT && win.mouse.initialized)
            raw_input_event((HRAWINPUT)lParam);
        break;

    case WM_CLOSE:
        PostQuitMessage(0);
        return FALSE;

    case WM_ACTIVATE:
        Win_Activate(wParam);
        break;

    case WM_WINDOWPOSCHANGING:
        pos_changing_event(hWnd, (WINDOWPOS *)lParam);
        break;

    case WM_WINDOWPOSCHANGED:
        pos_changed_event(hWnd, (WINDOWPOS *)lParam);
        return FALSE;

    case WM_STYLECHANGED:
    case WM_THEMECHANGED:
        pos_changed_event(hWnd, NULL);
        break;

    case WM_SYSCOMMAND:
        switch (wParam & 0xFFF0) {
        case SC_SCREENSAVE:
            return FALSE;
        case SC_MAXIMIZE:
            break; // Windowed mode retains normal desktop maximize behavior.
        }
        break;

    case WM_KEYDOWN:
    case WM_SYSKEYDOWN:
        legacy_key_event(wParam, lParam, true);
        return FALSE;

    case WM_KEYUP:
    case WM_SYSKEYUP:
        legacy_key_event(wParam, lParam, false);
        return FALSE;

    case WM_CHAR:
        char_event(wParam, lParam);
        return FALSE;

    case WM_SYSCHAR:
        // Windows reports AltGr as synthetic left Ctrl plus right Alt. Accept
        // that combination, but keep ordinary Alt shortcuts from typing.
        if (Key_IsDown(K_LCTRL) && Key_IsDown(K_RALT))
            char_event(wParam, lParam);
        return FALSE;

    case WM_DEADCHAR:
    case WM_SYSDEADCHAR:
        return FALSE;

    case WM_INPUTLANGCHANGE:
        // user switched keyboard layout; key labels are stale now
        keylabels_valid = false;
        break;

    case WM_ERASEBKGND:
        if (win.flags & QVF_FULLSCREEN)
            return FALSE;
        break;

    case WM_DISPLAYCHANGE:
        win.mode_changed |= MODE_DISPLAY;
        break;

    case WM_DPICHANGED:
        if (!win.applying_display && !(win.flags & QVF_FULLSCREEN)) {
            const RECT *suggested = (const RECT *)lParam;
            SetWindowPos(hWnd, NULL, suggested->left, suggested->top,
                         suggested->right - suggested->left, suggested->bottom - suggested->top,
                         SWP_NOZORDER | SWP_NOACTIVATE);
        }
        win.mode_changed |= MODE_SIZE;
        break;

    default:
        break;
    }

    // pass all unhandled messages to DefWindowProc
    return DefWindowProc(hWnd, uMsg, wParam, lParam);
}

/*
============
Win_PumpEvents
============
*/
void Win_PumpEvents(void)
{
    MSG        msg;

    win.lastMsgTime = Sys_Milliseconds();
    while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
        if (msg.message == WM_QUIT) {
            Com_Quit(NULL, ERR_DISCONNECT);
            break;
        }
        if (should_translate_message(&msg))
            TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    if (win.mode_changed & MODE_DISPLAY) {
        MONITORINFOEXA info = { .cbSize = sizeof(info) };
        if (!Win_FindDisplay(win.display_device, &info)) {
            if (VID_PendingDisplaySettings()) {
                VID_RevertDisplaySettings();
            } else {
                vid_display_settings_t fallback = { 0 };
                fallback.window = (vrect_t){ 0, 0, 640, 480 };
                Win_ApplyDisplaySettings(&fallback);
            }
        } else if (win.display_mode == VID_DISPLAY_BORDERLESS) {
            vid_display_settings_t settings;
            if (Win_GetDisplaySettings(&settings))
                Win_ApplyDisplaySettings(&settings);
        }
    }
    if (win.mode_changed) {
        if (win.mode_changed & MODE_REPOSITION) {
            Win_SetPosition();
        }
        if (win.mode_changed & (MODE_SIZE | MODE_POS | MODE_STYLE)) {
            if (!(win.flags & QVF_FULLSCREEN) && !VID_PendingDisplaySettings())
                VID_SetGeometry(&win.rc);
            if (win.mouse.grabbed) {
                Win_ClipCursor();
            }
        }
        if (win.mode_changed & MODE_SIZE) {
            Win_ModeChanged();
        }
        win.mode_changed = 0;
    }
}

static void win_menu_cursor_changed(cvar_t *self)
{
    HCURSOR hNew = NULL;

    if (!Win_MenuCursorEnabled()) {
        hNew = NULL;
    } else if (!_stricmp(self->string, "arrow") ||
               !_stricmp(self->string, "1") ||
               !_stricmp(self->string, "on") ||
               !_stricmp(self->string, "true")) {
        hNew = LoadCursor(NULL, IDC_ARROW);
    } else if (!_stricmp(self->string, "cross")) {
        hNew = LoadCursor(NULL, IDC_CROSS);
    } else if (!_stricmp(self->string, "hand")) {
        hNew = LoadCursor(NULL, IDC_HAND);
    } else if (!_stricmp(self->string, "ibeam")) {
        hNew = LoadCursor(NULL, IDC_IBEAM);
    } else if (!_stricmp(self->string, "wait")) {
        hNew = LoadCursor(NULL, IDC_WAIT);
    } else if (!_stricmp(self->string, "no")) {
        hNew = LoadCursor(NULL, IDC_NO);
    } else if (!_stricmp(self->string, "size")) {
        hNew = LoadCursor(NULL, IDC_SIZEALL);
    } else {
        hNew = LoadCursor(NULL, IDC_ARROW);
    }

    win_menu_cursor_handle = hNew;

    IN_Activate();

    if (win.wnd) {
        POINT pt;
        GetCursorPos(&pt);
        ScreenToClient(win.wnd, &pt);
        RECT rc;
        GetClientRect(win.wnd, &rc);
        if (PtInRect(&rc, pt)) {
            PostMessage(win.wnd, WM_SETCURSOR, (WPARAM)win.wnd, MAKELONG(HTCLIENT, WM_MOUSEMOVE));
        }
    }
}

static void win_style_changed(cvar_t *self)
{
    if (VID_DisplayCommitting() || VID_PendingDisplaySettings())
        return;
    win.window_flags = (vid_noborder && vid_noborder->integer ? VID_WINDOW_BORDERLESS : 0) |
        (win_notitle && win_notitle->integer ? VID_WINDOW_NOTITLE : 0) |
        (win_noresize && win_noresize->integer ? VID_WINDOW_NORESIZE : 0);
    if (!win.wnd) {
        return;
    }
    if (self == vid_noborder && vid_fullscreen && vid_fullscreen->integer > 0) {
        Win_SetMode();
        IN_Activate();
        return;
    }
    if (!(win.flags & QVF_FULLSCREEN) || self == win_alwaysontop) {
        win.mode_changed |= MODE_REPOSITION;
    }
}

/*
============
Win_Init
============
*/
void Win_Init(void)
{
    WNDCLASSEXA wc;

    // register variables
    vid_flip_on_switch = Cvar_Get("vid_flip_on_switch", "0", 0);
    vid_hwgamma = Cvar_Get("vid_hwgamma", "0", CVAR_REFRESH);
    win_noalttab = Cvar_Get("win_noalttab", "0", CVAR_ARCHIVE);
    win_noalttab->changed = win_noalttab_changed;
    win_disablewinkey = Cvar_Get("win_disablewinkey", "0", 0);
    win_disablewinkey->changed = win_disablewinkey_changed;
    win_noresize = Cvar_Get("win_noresize", "0", 0);
    win_noresize->changed = win_style_changed;
    win_notitle = Cvar_Get("win_notitle", "0", 0);
    win_notitle->changed = win_style_changed;
    win_alwaysontop = Cvar_Get("win_alwaysontop", "0", 0);
    win_alwaysontop->changed = win_style_changed;
    vid_noborder = Cvar_Get("vid_noborder", "0", CVAR_ARCHIVE);
    vid_noborder->changed = win_style_changed;
    win_menu_cursor = Cvar_Get("win_menu_cursor", "arrow", CVAR_ARCHIVE);
    win_menu_cursor->changed = win_menu_cursor_changed;
    win_menu_cursor_changed(win_menu_cursor);

    win_disablewinkey_changed(win_disablewinkey);

    win.GetDpiForWindow = (PVOID)GetProcAddress(GetModuleHandle("user32"), "GetDpiForWindow");

    // register the frame class
    memset(&wc, 0, sizeof(wc));
    wc.cbSize = sizeof(wc);
    wc.lpfnWndProc = Win_MainWndProc;
    wc.hInstance = hGlobalInstance;
    wc.hIcon = LoadImage(hGlobalInstance, MAKEINTRESOURCE(IDI_APP),
                         IMAGE_ICON, 32, 32, LR_CREATEDIBSECTION);
    wc.hIconSm = LoadImage(hGlobalInstance, MAKEINTRESOURCE(IDI_APP),
                           IMAGE_ICON, 16, 16, LR_CREATEDIBSECTION);
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = GetStockObject(BLACK_BRUSH);
    wc.lpszClassName = WINDOW_CLASS_NAME;

    if (!RegisterClassExA(&wc)) {
        Com_Error(ERR_FATAL, "Couldn't register main window class");
    }

    // create the window
    win.wnd = CreateWindowA(WINDOW_CLASS_NAME, PRODUCT, 0, 0, 0, 0, 0, NULL,
                            NULL, hGlobalInstance, NULL);
    if (!win.wnd) {
        Com_Error(ERR_FATAL, "Couldn't create main window");
    }

    win.dc = GetDC(win.wnd);
    if (!win.dc) {
        Com_Error(ERR_FATAL, "Couldn't get DC of the main window");
    }

    // init gamma ramp
    if (vid_hwgamma->integer) {
        if (GetDeviceGammaRamp(win.dc, win.gamma_orig)) {
            Com_DPrintf("...enabling hardware gamma\n");
            win.flags |= QVF_GAMMARAMP;
            memcpy(win.gamma_cust, win.gamma_orig, sizeof(win.gamma_cust));
        } else {
            Com_DPrintf("...hardware gamma not supported\n");
            Cvar_Set("vid_hwgamma", "0");
        }
    }
}

/*
============
Win_Shutdown
============
*/
void Win_Shutdown(void)
{
    if (win.flags & QVF_GAMMARAMP) {
        SetDeviceGammaRamp(win.dc, win.gamma_orig);
    }

    if (win.dc) {
        ReleaseDC(win.wnd, win.dc);
    }

    if (win.wnd) {
        DestroyWindow(win.wnd);
    }

    UnregisterClassA(WINDOW_CLASS_NAME, hGlobalInstance);

    if (win.kbdHook) {
        UnhookWindowsHookEx(win.kbdHook);
    }

    if (win.cds_fullscreen) {
        ChangeDisplaySettingsExA(win.display_device, &win.desktop_dm, NULL, 0, NULL);
    }

    memset(&win, 0, sizeof(win));
}

/*
===============================================================================

MOUSE

===============================================================================
*/

// Called when the window gains focus or changes in some way
static void Win_ClipCursor(void)
{
    SetCursorPos(win.center_x, win.center_y);
    ClipCursor(&win.screen_rc);
}

// Called when the window gains focus
static void Win_AcquireMouse(void)
{
    Win_ClipCursor();
    SetCapture(win.wnd);

    while (ShowCursor(FALSE) >= 0)
        ;
}

// Called when the window loses focus
static void Win_DeAcquireMouse(void)
{
    if (win.mouse.restore_parms) {
        SystemParametersInfo(SPI_SETMOUSE, 0, win.mouse.original_parms, 0);
        win.mouse.restore_parms = false;
    }

    ClipCursor(NULL);
    ReleaseCapture();

    while (ShowCursor(TRUE) < 0)
        ;

    if (Win_MenuCursorActive() && win_menu_cursor_handle) {
        SetCursor(win_menu_cursor_handle);
    }
}

static bool Win_R1Q2Mouse(void)
{
    return Cvar_VariableInteger("m_r1q2") != 0;
}

static void Win_UpdateMouseParms(void)
{
    static int r1q2_mouseparms[3] = { 0, 0, 0 };

    if (!win.mouse.parms_valid) {
        return;
    }

    if (!win.mouse.grabbed) {
        if (win.mouse.restore_parms) {
            SystemParametersInfo(SPI_SETMOUSE, 0, win.mouse.original_parms, 0);
            win.mouse.restore_parms = false;
        }
        return;
    }

    if (Win_R1Q2Mouse()) {
        if (!win.mouse.restore_parms) {
            win.mouse.restore_parms =
                SystemParametersInfo(SPI_SETMOUSE, 0, r1q2_mouseparms, 0);
        }
    } else if (win.mouse.restore_parms) {
        SystemParametersInfo(SPI_SETMOUSE, 0, win.mouse.original_parms, 0);
        win.mouse.restore_parms = false;
    }
}

bool Win_GetMouseMotion(int *dx, int *dy)
{
    POINT current_pos;

    if (!win.mouse.grabbed) {
        return false;
    }

    Win_UpdateMouseParms();

    if (Win_R1Q2Mouse()) {
        win.mouse.mx = 0;
        win.mouse.my = 0;

        if (!GetCursorPos(&current_pos)) {
            return false;
        }

        *dx = current_pos.x - win.center_x;
        *dy = current_pos.y - win.center_y;

        if (*dx || *dy) {
            SetCursorPos(win.center_x, win.center_y);
        }
        return true;
    }

    *dx = win.mouse.mx;
    *dy = win.mouse.my;
    win.mouse.mx = 0;
    win.mouse.my = 0;
    return true;
}

static BOOL register_raw_mouse(bool enable)
{
    RAWINPUTDEVICE rid = {
        .usUsagePage = HID_USAGE_PAGE_GENERIC,
        .usUsage = HID_USAGE_GENERIC_MOUSE,
    };

    if (enable)
        rid.hwndTarget = win.wnd;
    else
        rid.dwFlags = RIDEV_REMOVE;

    return RegisterRawInputDevices(&rid, 1, sizeof(rid));
}

void Win_ShutdownMouse(void)
{
    if (!win.mouse.initialized) {
        return;
    }

    if (win.mouse.grabbed) {
        Win_DeAcquireMouse();
    }

    register_raw_mouse(false);

    memset(&win.mouse, 0, sizeof(win.mouse));
}

bool Win_InitMouse(void)
{
    if (!win.wnd) {
        return false;
    }

    if (!register_raw_mouse(true)) {
        Com_EPrintf("RegisterRawInputDevices failed with error %#lx\n", GetLastError());
        return false;
    }

    win.mouse.parms_valid =
        SystemParametersInfo(SPI_GETMOUSE, 0, win.mouse.original_parms, 0);

    Com_DPrintf("Raw mouse initialized.\n");
    win.mouse.initialized = true;
    return true;
}

// Called when the main window gains or loses focus.
void Win_GrabMouse(bool grab)
{
    if (!win.mouse.initialized) {
        return;
    }

    grab = Win_ShouldGrabMouse(grab);

    if (win.mouse.grabbed == grab) {
        win.mouse.mx = 0;
        win.mouse.my = 0;
        return;
    }

    if (grab) {
        Win_AcquireMouse();
    } else {
        Win_DeAcquireMouse();
    }

    win.mouse.grabbed = grab;
    Win_UpdateMouseParms();
    win.mouse.mx = 0;
    win.mouse.my = 0;
}

void Win_WarpMouse(int x, int y)
{
    SetCursorPos(win.screen_rc.left + x, win.screen_rc.top + y);
}

/*
================
Win_GetClipboardData
================
*/
char *Win_GetClipboardData(void)
{
    HANDLE clipdata;
    char *cliptext, *data;

    if (!OpenClipboard(NULL)) {
        Com_DPrintf("Couldn't open clipboard.\n");
        return NULL;
    }

    data = NULL;
    if ((clipdata = GetClipboardData(CF_TEXT)) != NULL) {
        if ((cliptext = GlobalLock(clipdata)) != NULL) {
            data = Z_CopyString(cliptext);
            GlobalUnlock(clipdata);
        }
    }

    CloseClipboard();
    return data;
}

/*
================
Win_SetClipboardData
================
*/
void Win_SetClipboardData(const char *data)
{
    HANDLE clipdata;
    char *cliptext;
    size_t length;

    if (!data || !*data) {
        return;
    }

    if (!OpenClipboard(NULL)) {
        Com_DPrintf("Couldn't open clipboard.\n");
        return;
    }

    EmptyClipboard();

    length = strlen(data) + 1;
    if ((clipdata = GlobalAlloc(GMEM_MOVEABLE | GMEM_DDESHARE, length)) != NULL) {
        if ((cliptext = GlobalLock(clipdata)) != NULL) {
            memcpy(cliptext, data, length);
            GlobalUnlock(clipdata);
            SetClipboardData(CF_TEXT, clipdata);
        }
    }

    CloseClipboard();
}
