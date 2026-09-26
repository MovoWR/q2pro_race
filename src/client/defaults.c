// SPDX-License-Identifier: GPL-2.0-or-later
#include "shared/shared.h"
#include "common/common.h"
#include "common/files.h"
#include "client/client.h"

// Apply after the original game's defaults, before player startup scripts.
void CL_AddDefaultConfig(unsigned flags)
{
    if (COM_DEDICATED || Q_stricmp(fs_game->string, "jump") ||
        FS_FileExistsEx(COM_CONFIG_CFG, FS_TYPE_REAL | flags) ||
        FS_FileExistsEx(COM_DEFAULT_CFG, FS_TYPE_REAL | flags) ||
        FS_FileExistsEx(COM_DEFAULT_CFG, FS_TYPE_PAK | FS_PATH_GAME))
        return;

    Cbuf_AddText(&cmd_buffer,
        "unbindall\n"
        "bind w +forward\n"
        "bind s +back\n"
        "bind a +moveleft\n"
        "bind d +moveright\n"
        "bind SPACE +moveup\n"
        "bind CTRL +movedown\n"
        "bind MOUSE1 +attack\n"
        "bind MOUSE2 +dj\n"
        "bind SHIFT +speed\n"
        "bind F1 inven\n"
        "bind TAB score\n"
        "bind ENTER invuse\n"
        "bind [ invprev\n"
        "bind ] invnext\n"
        "bind e \"team easy\"\n"
        "bind r \"team hard\"\n"
        "bind q store\n"
        "bind f kill\n"
        "bind t messagemode\n"
        "bind y messagemode2\n"
        "bind F12 screenshot\n"
        "bind ESCAPE togglemenu\n"
        "bind ` toggleconsole\n"
        "bind ~ toggleconsole\n"
        "seta cl_run 1\n"
        "seta gl_beamstyle 1\n");
    Cbuf_Execute(&cmd_buffer);
}
