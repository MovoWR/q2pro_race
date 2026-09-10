/* Display settings shared by the menu, confirmation controller and video backend. */
#pragma once

typedef enum {
    VID_DISPLAY_WINDOWED,
    VID_DISPLAY_BORDERLESS,
    VID_DISPLAY_EXCLUSIVE
} vid_display_mode_t;

typedef struct {
    char id[64];
    char name[128];
    vrect_t desktop;
    int refresh;
    bool primary;
} vid_display_t;

typedef struct {
    int width, height, refresh;
} vid_display_resolution_t;

enum {
    VID_WINDOW_BORDERLESS = BIT(0),
    VID_WINDOW_NOTITLE = BIT(1),
    VID_WINDOW_NORESIZE = BIT(2)
};

typedef struct {
    vid_display_mode_t mode;
    char display[64];
    vrect_t window;
    int width, height, refresh;
    unsigned window_flags;
    int depth; // Legacy explicit fullscreen bit depth; zero uses the desktop.
    int vsync;
} vid_display_settings_t;

bool VID_HasDisplaySettings(void);
bool VID_ApplyDisplaySettings(const vid_display_settings_t *settings);
void VID_KeepDisplaySettings(void);
void VID_RevertDisplaySettings(void);
void VID_DisplayFrame(void);
const vid_display_settings_t *VID_PendingDisplaySettings(void);
int VID_DisplaySecondsLeft(void);
const char *VID_DisplayMessage(void);
bool VID_DisplayCommitting(void);
bool VID_DisplaySettingsEqual(const vid_display_settings_t *a, const vid_display_settings_t *b);
bool VID_ToggleDisplay(void);
