/* Selected key/action reminders in the client HUD. */
#pragma once

#include "common/cvar.h"

/* Import preloaded settings without changing the real reset default. New names
 * win when both exist; legacy names are not live aliases. */
static inline cvar_t *SCR_RegisterBindReminderCvar(const char *name,
                                                 const char *legacy_name,
                                                 const char *value)
{
    bool exists = Cvar_FindVar(name) != NULL;
    cvar_t *legacy = Cvar_FindVar(legacy_name);
    cvar_t *var = Cvar_Get(name, value, CVAR_ARCHIVE);

    if (!exists && legacy && !(legacy->flags & CVAR_WEAK))
        Cvar_SetByVar(var, legacy->string, FROM_CODE);
    return var;
}

void SCR_BindRemindersInit(void);
void SCR_DrawBindReminders(float hud_alpha);
void SCR_PreviewBindReminders(void);
