/* Display trials leave archived cvars untouched until Keep. */
#include "client.h"

#define DISPLAY_CONFIRM_MSEC 15000u

static struct {
    bool active, committing;
    unsigned started, checked;
    vid_display_settings_t previous, requested;
    char message[160];
} display_trial;

bool VID_HasDisplaySettings(void)
{
    return vid && vid->get_displays && vid->get_display_modes &&
        vid->get_display_settings && vid->apply_display_settings;
}

bool VID_DisplaySettingsEqual(const vid_display_settings_t *a, const vid_display_settings_t *b)
{
    if (a->mode != b->mode || strcmp(a->display, b->display) || a->vsync != b->vsync)
        return false;
    if (a->mode == VID_DISPLAY_WINDOWED)
        return a->window.x == b->window.x && a->window.y == b->window.y &&
            a->window.width == b->window.width && a->window.height == b->window.height &&
            a->window_flags == b->window_flags;
    if (a->mode == VID_DISPLAY_EXCLUSIVE)
        return a->width == b->width && a->height == b->height && a->refresh == b->refresh;
    return true;
}

const vid_display_settings_t *VID_PendingDisplaySettings(void)
{
    return display_trial.active ? &display_trial.requested : NULL;
}

bool VID_DisplayCommitting(void)
{
    return display_trial.committing;
}

int VID_DisplaySecondsLeft(void)
{
    unsigned elapsed = Sys_Milliseconds() - display_trial.started;
    if (!display_trial.active || elapsed >= DISPLAY_CONFIRM_MSEC)
        return 0;
    return (DISPLAY_CONFIRM_MSEC - elapsed + 999) / 1000;
}

const char *VID_DisplayMessage(void)
{
    return display_trial.message;
}

static bool DisplayExists(const char *id)
{
    int count;
    bool found = false;
    vid_display_t *displays = vid->get_displays(&count);
    for (int i = 0; i < count; i++)
        if (!strcmp(id, displays[i].id))
            found = true;
    Z_Free(displays);
    return found;
}

void VID_RevertDisplaySettings(void)
{
    vid_display_settings_t previous;
    if (!display_trial.active || !VID_HasDisplaySettings())
        return;
    previous = display_trial.previous;
    display_trial.active = false;
    if (vid->apply_display_settings(&previous)) {
        Q_strlcpy(display_trial.message, "Previous display settings restored.", sizeof(display_trial.message));
    } else {
        // An empty ID selects an available monitor if the old one was unplugged.
        previous.mode = VID_DISPLAY_WINDOWED;
        previous.display[0] = 0;
        previous.window = (vrect_t){ 0, 0, 640, 480 };
        previous.window_flags = 0;
        if (vid->apply_display_settings(&previous))
            Q_strlcpy(display_trial.message, "Previous display unavailable. Restored a window on an available monitor.", sizeof(display_trial.message));
        else
            Q_strlcpy(display_trial.message, "Could not restore the display. Reconnect a monitor and retry.", sizeof(display_trial.message));
    }
    vid->swap_interval(previous.vsync);
    IN_Activate();
}

bool VID_ApplyDisplaySettings(const vid_display_settings_t *settings)
{
    if (!VID_HasDisplaySettings() || display_trial.active)
        return false;
    if (!vid->get_display_settings(&display_trial.previous))
        return false;
    if (VID_DisplaySettingsEqual(settings, &display_trial.previous))
        return true;
    display_trial.requested = *settings;
    display_trial.started = display_trial.checked = Sys_Milliseconds();
    display_trial.message[0] = 0;
    display_trial.active = true;
    if (!vid->apply_display_settings(settings)) {
        VID_RevertDisplaySettings();
        Q_strlcat(display_trial.message, " Requested settings failed.", sizeof(display_trial.message));
        return false;
    }
    vid_display_settings_t actual;
    if (!vid->get_display_settings(&actual) || actual.mode != settings->mode ||
        strcmp(actual.display, settings->display) ||
        (settings->mode == VID_DISPLAY_EXCLUSIVE &&
         ((settings->width && actual.width != settings->width) ||
          (settings->height && actual.height != settings->height) ||
          (settings->refresh && actual.refresh != settings->refresh)))) {
        VID_RevertDisplaySettings();
        Q_strlcat(display_trial.message, " Driver did not apply the requested mode.", sizeof(display_trial.message));
        return false;
    }
    display_trial.requested.window =
        settings->mode != VID_DISPLAY_WINDOWED && display_trial.previous.mode == VID_DISPLAY_WINDOWED
        ? display_trial.previous.window : actual.window;
    display_trial.requested.width = actual.width;
    display_trial.requested.height = actual.height;
    display_trial.requested.refresh = actual.refresh;
    display_trial.requested.depth = settings->depth;
    vid->swap_interval(settings->vsync);
    IN_Activate();
    // VSync alone does not risk losing the display.
    vid_display_settings_t compare = display_trial.previous;
    compare.vsync = settings->vsync;
    if (VID_DisplaySettingsEqual(settings, &compare))
        VID_KeepDisplaySettings();
    return true;
}

static int FullscreenIndex(const vid_display_settings_t *settings)
{
    char token[64];
    const char *list = vid_modelist->string;
    int index = 1;
    if (settings->depth)
        Q_snprintf(token, sizeof(token), "%dx%d@%d:%d", settings->width, settings->height, settings->refresh, settings->depth);
    else
        Q_snprintf(token, sizeof(token), "%dx%d@%d", settings->width, settings->height, settings->refresh);
    size_t length = strlen(token);
    for (const char *p = list; *p;) {
        while (Q_isspace(*p)) p++;
        if (!*p) break;
        const char *start = p;
        while (*p && !Q_isspace(*p)) p++;
        if ((size_t)(p - start) == length && !strncmp(start, token, length))
            return index;
        index++;
    }
    char *expanded = Z_Malloc(strlen(list) + length + 2);
    Q_concat(expanded, strlen(list) + length + 2, list, *list ? " " : "", token);
    Cvar_SetByVar(vid_modelist, expanded, FROM_CODE);
    Z_Free(expanded);
    return index;
}

void VID_KeepDisplaySettings(void)
{
    if (!display_trial.active || !VID_HasDisplaySettings())
        return;
    if (!VID_DisplaySecondsLeft() || !DisplayExists(display_trial.requested.display)) {
        VID_RevertDisplaySettings();
        return;
    }
    vid_display_settings_t settings = display_trial.requested;
    vid_display_settings_t actual;
    if (!vid->get_display_settings(&actual) || actual.mode != settings.mode ||
        strcmp(actual.display, settings.display) ||
        (settings.mode == VID_DISPLAY_EXCLUSIVE &&
         (actual.width != settings.width || actual.height != settings.height || actual.refresh != settings.refresh))) {
        VID_RevertDisplaySettings();
        return;
    }
    if (settings.mode == VID_DISPLAY_WINDOWED)
        settings.window = actual.window;

    display_trial.committing = true;
    Cvar_Set("vid_monitor", settings.display);
    Cvar_Set("gl_swapinterval", va("%d", settings.vsync));
    Cvar_Set("vid_noborder", settings.mode == VID_DISPLAY_BORDERLESS ? "1" : "0");
    if (settings.mode == VID_DISPLAY_WINDOWED) {
        if (display_trial.previous.mode != VID_DISPLAY_WINDOWED)
            Cvar_Set("_vid_fullscreen_borderless", display_trial.previous.mode == VID_DISPLAY_BORDERLESS ? "1" : "0");
        Cvar_Set("win_notitle", "0");
        Cvar_Set("win_noresize", "0");
        VID_SetGeometry(&settings.window);
        Cvar_SetInteger(vid_fullscreen, 0, FROM_CODE);
    } else {
        if (display_trial.previous.mode == VID_DISPLAY_WINDOWED)
            VID_SetGeometry(&display_trial.previous.window);
        int index = max(_vid_fullscreen->integer, 1);
        if (settings.mode == VID_DISPLAY_EXCLUSIVE) {
            index = FullscreenIndex(&settings);
            // Keep the index and its meaning together across restarts.
            vid_modelist->flags |= CVAR_ARCHIVE;
        }
        Cvar_Set("_vid_fullscreen_borderless", settings.mode == VID_DISPLAY_BORDERLESS ? "1" : "0");
        Cvar_SetInteger(_vid_fullscreen, index, FROM_CODE);
        Cvar_SetInteger(vid_fullscreen, index, FROM_CODE);
    }
    display_trial.committing = false;
    display_trial.active = false;
    Q_strlcpy(display_trial.message, "Display settings kept.", sizeof(display_trial.message));
}

void VID_DisplayFrame(void)
{
    if (!display_trial.active || !VID_HasDisplaySettings())
        return;
    if (!VID_DisplaySecondsLeft()) {
        VID_RevertDisplaySettings();
    } else if (Sys_Milliseconds() - display_trial.checked >= 250) {
        display_trial.checked = Sys_Milliseconds();
        if (!DisplayExists(display_trial.requested.display))
            VID_RevertDisplaySettings();
    }
}

bool VID_ToggleDisplay(void)
{
    vid_display_settings_t settings;
    if (!VID_HasDisplaySettings())
        return false;
    if (display_trial.active) {
        VID_RevertDisplaySettings();
        return true;
    }
    if (!vid->get_display_settings(&settings))
        return true;
    if (settings.mode != VID_DISPLAY_WINDOWED) {
        settings.mode = VID_DISPLAY_WINDOWED;
        settings.window_flags = 0;
    } else {
        const char *last = Cvar_VariableString("_vid_fullscreen_borderless");
        settings.mode = *last && atoi(last) ? VID_DISPLAY_BORDERLESS : VID_DISPLAY_EXCLUSIVE;
        vrect_t rc;
        int refresh;
        if (VID_GetFullscreenMode(_vid_fullscreen->integer, &rc, &refresh, &settings.depth) && rc.width) {
            settings.width = rc.width;
            settings.height = rc.height;
            settings.refresh = refresh;
        } else {
            int count;
            vid_display_t *displays = vid->get_displays(&count);
            for (int i = 0; i < count; i++) {
                if (!strcmp(settings.display, displays[i].id)) {
                    settings.width = displays[i].desktop.width;
                    settings.height = displays[i].desktop.height;
                    settings.refresh = displays[i].refresh;
                    break;
                }
            }
            Z_Free(displays);
        }
    }
    VID_ApplyDisplaySettings(&settings);
    return true;
}
