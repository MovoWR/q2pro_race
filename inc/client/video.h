/*
Copyright (C) 2003-2006 Andrey Nazarov

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

#pragma once

#include "client/display.h"

typedef struct {
    const char *name;

    bool (*probe)(void);
    bool (*init)(void);
    void (*shutdown)(void);
    void (*fatal_shutdown)(void);
    void (*pump_events)(void);

    // This backend supplies layout-aware character events separately from
    // physical key events, so Key_Event must not synthesize US-layout text.
    bool native_text_input;

    char *(*get_mode_list)(void);
    int (*get_dpi_scale)(void);
    void (*set_mode)(void);
    void (*update_gamma)(const byte *table);

    void *(*get_proc_addr)(const char *sym);
    void (*swap_buffers)(void);
    void (*swap_interval)(int val);

    // Optional structured display settings. Returned lists belong to the caller.
    vid_display_t *(*get_displays)(int *count);
    vid_display_resolution_t *(*get_display_modes)(const char *display, int *count);
    bool (*get_display_settings)(vid_display_settings_t *settings);
    bool (*apply_display_settings)(const vid_display_settings_t *settings);

    char *(*get_selection_data)(void);
    char *(*get_clipboard_data)(void);
    void (*set_clipboard_data)(const char *data);

    // maps a canonical (US layout) keynum to the character actually printed on
    // that physical key under the user's current keyboard layout. returns 0 if
    // there is no sensible label, and may be NULL if the backend can't tell.
    int (*get_key_label)(int keynum);

    bool (*init_mouse)(void);
    void (*shutdown_mouse)(void);
    void (*grab_mouse)(bool grab);
    void (*warp_mouse)(int x, int y);
    bool (*get_mouse_motion)(int *dx, int *dy);
    bool (*uses_system_cursor)(void);
} vid_driver_t;

extern cvar_t       *vid_geometry;
extern cvar_t       *vid_modelist;
extern cvar_t       *vid_fullscreen;
extern cvar_t       *_vid_fullscreen;

extern const vid_driver_t   *vid;

bool VID_GetFullscreen(vrect_t *rc, int *freq_p, int *depth_p);
bool VID_GetFullscreenMode(int index, vrect_t *rc, int *freq_p, int *depth_p);
bool VID_GetGeometry(vrect_t *rc);
void VID_SetGeometry(const vrect_t *rc);
void VID_ToggleFullscreen(void);
