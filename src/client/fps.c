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
/* FPS commands and persistent hold/release slots. */
#include "client.h"
#include "client/fps.h"

// Cvar_Set() writes with FROM_CODE, and change_string_value() deliberately
// skips the `changed' callback in that case. Setting cl_maxfps that way would
// update the cvar but leave the client ticking at the old rate, so re-time the
// frame loop explicitly.
static void CL_SetMaxFps(const char *value)
{
    if (!CL_ParseFpsInteger(value)) {
        Com_Printf("Error: FPS must be a positive integer\n");
        return;
    }
    Cvar_Set("cl_maxfps", value);
    CL_UpdateFrameTimes();
}

void CL_FpsDown_f(void)
{
    if (Cmd_Argc() < 3) {
        Com_Printf("usage: +fps <down_fps> <up_fps>\n");
        return;
    }

    const char *down_str = Cmd_Argv(1);
    const char *up_str = Cmd_Argv(2);

    if (!CL_ParseFpsInteger(down_str)) {
        Com_Printf("Error: down_fps must be a positive integer\n");
        return;
    }
    if (!CL_ParseFpsInteger(up_str)) {
        Com_Printf("Error: up_fps must be a positive integer\n");
        return;
    }

    CL_SetMaxFps(down_str);
}

void CL_FpsUp_f(void)
{
    if (Cmd_Argc() < 3) {
        Com_Printf("usage: -fps <down_fps> <up_fps>\n");
        return;
    }

    const char *down_str = Cmd_Argv(1);
    const char *up_str = Cmd_Argv(2);

    if (!CL_ParseFpsInteger(down_str)) {
        Com_Printf("Error: down_fps must be a positive integer\n");
        return;
    }
    if (!CL_ParseFpsInteger(up_str)) {
        Com_Printf("Error: up_fps must be a positive integer\n");
        return;
    }

    CL_SetMaxFps(up_str);
}

static cvar_t *fps_hold[NUM_FPS_SLOTS];
static cvar_t *fps_release[NUM_FPS_SLOTS];

// returns a zero based slot index, or -1 if the argument is out of range
static int CL_ParseFpsSlot(void)
{
    int slot = CL_ParseFpsInteger(Cmd_Argv(1));

    if (slot < 1 || slot > NUM_FPS_SLOTS) {
        Com_Printf("usage: %s <slot 1-%d>\n", Cmd_Argv(0), NUM_FPS_SLOTS);
        return -1;
    }

    return slot - 1;
}

void CL_FpsHoldDown_f(void)
{
    int slot = CL_ParseFpsSlot();

    if (slot >= 0 && fps_hold[slot])
        CL_SetMaxFps(fps_hold[slot]->string);
}

void CL_FpsHoldUp_f(void)
{
    int slot = CL_ParseFpsSlot();

    if (slot >= 0 && fps_release[slot])
        CL_SetMaxFps(fps_release[slot]->string);
}

void CL_FpsShortcut_f(void)
{
    const char *cmd = Cmd_Argv(0);
    // Registered shortcuts use the same positive integer validation.
    if (cmd && Q_tolower(cmd[0]) == 'f') {
        int fps = CL_ParseFpsInteger(cmd + 1);
        if (fps >= 20 && fps <= 120) {
            char val[16];
            Q_snprintf(val, sizeof(val), "%i", fps);
            CL_SetMaxFps(val);
        }
    }
}

void CL_InitFpsSlots(void)
{
    const char *hold = Cvar_VariableString("fps_default_hold");
    const char *release = Cvar_VariableString("fps_default_release");

    if (!CL_ParseFpsInteger(hold))
        hold = "30";
    if (!CL_ParseFpsInteger(release))
        release = "120";
    for (int i = 0; i < NUM_FPS_SLOTS; i++) {
        char name[32];
        Q_snprintf(name, sizeof(name), "fps_hold_%d", i + 1);
        fps_hold[i] = Cvar_Get(name, hold, CVAR_ARCHIVE);
        Q_snprintf(name, sizeof(name), "fps_release_%d", i + 1);
        fps_release[i] = Cvar_Get(name, release, CVAR_ARCHIVE);
    }
}
