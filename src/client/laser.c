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
// Client-side temporary lasers and rail cores.

#include "client.h"
#include "laser.h"
#include "src/jump/strafe_helper.h"

#define MAX_LASERS  256

static laser_t  cl_lasers[MAX_LASERS];

void CL_ClearLasers(void)
{
    memset(cl_lasers, 0, sizeof(cl_lasers));
}

laser_t *CL_AllocLaser(void)
{
    laser_t *l;
    int i;

    for (i = 0, l = cl_lasers; i < MAX_LASERS; i++, l++) {
        if (cl.time - l->starttime >= l->lifetime) {
            memset(l, 0, sizeof(*l));
            l->starttime = cl.time;
            return l;
        }
    }

    return NULL;
}

static float Cvar_GetCappedValue(cvar_t *cvar, float min, float max)
{
    float value = cvar->value;
    if (value < min) {
        return min;
    } else if (value > max) {
        return max;
    }
    return value;
}

void CL_AddLasers(void)
{
    laser_t *l;
    entity_t ent;
    int i, time;

    memset(&ent, 0, sizeof(ent));

    for (i = 0, l = cl_lasers; i < MAX_LASERS; i++, l++) {
        time = l->lifetime - (cl.time - l->starttime);
        if (time <= 0) {
            continue;
        }
        if (l->color == -1) {
            ent.rgba = l->rgba;
            ent.alpha = (float)time / (float)l->lifetime;
            if (l->race) {
                float alpha = cl_race_alpha->value;
                if (alpha <= 0.0f)
                    continue;
                if (alpha > 1.0f)
                    alpha = 1.0f;
                ent.alpha *= alpha;
            }
        } else {
            ent.alpha = 0.30f;
        }

        ent.skinnum = l->color;
        ent.flags = RF_TRANSLUCENT | RF_BEAM;
        VectorCopy(l->start, ent.origin);
        VectorCopy(l->end, ent.oldorigin);
        ent.frame = l->width;
        V_AddEntity(&ent);
    }
}

void CL_ParseLaser(unsigned colors)
{
    laser_t *l;
    color_t parsedColor;

    l = CL_AllocLaser();
    if (!l)
        return;

    VectorCopy(te.pos1, l->start);
    VectorCopy(te.pos2, l->end);
    l->race = fs_game && !Q_stricmp(fs_game->string, "jump");
    if (!l->race) {
        l->lifetime = 100;
        l->color = (colors >> ((Q_rand() % 4) * 8)) & 0xff;
        l->width = 4;
        return;
    }

    l->color = -1;
    l->lifetime = Cvar_GetCappedValue(cl_race_life, 0.0f, 5000.0f);
    l->width = Cvar_GetCappedValue(cl_race_width, 0.0f, 20.0f);

    const char *colorStr = cl_race_color->string;
    if (!shc_ParseColorCvar(colorStr, NULL, &parsedColor)) {
        parsedColor.u32 = 0xFF0000FF;
    }
    l->rgba = parsedColor;
}
