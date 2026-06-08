/*
Copyright (C) 2026

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.
*/

#include "ui.h"

typedef struct {
    int firstFrame;
    int lastFrame;
    unsigned frameMs;
    bool loop;
} uiModelAnim_t;

typedef enum {
    UI_MODEL_STAND,
    UI_MODEL_RUN,
    UI_MODEL_JUMP,
    UI_MODEL_WAVE,
    UI_MODEL_SALUTE,
    UI_MODEL_POINT,
    UI_MODEL_TAUNT,
    UI_MODEL_FLIP,
    UI_MODEL_ATTACK,
    UI_MODEL_CROUCH,
    UI_MODEL_PAIN,
UI_MODEL_CROUCH_WALK,
UI_MODEL_CROUCH_ATTACK,
UI_MODEL_CROUCH_PAIN,
UI_MODEL_DEATH1,
UI_MODEL_DEATH2,
UI_MODEL_DEATH3
} uiModelCueId_t;

typedef struct {
    uiModelCueId_t cue;
    unsigned duration;
} uiModelCue_t;

typedef struct {
    bool initialized;
    refdef_t refdef;
    vrect_t viewport;
    entity_t entities[2];
    int numEntities;
    char skin[MAX_QPATH];
    uiModelCueId_t cue;
    uiModelCueId_t ambientCue;
    uiModelCueId_t oneShotCue;
    int cueIndex;
    int frame;
    int animEnd;
    unsigned frameTime;
    unsigned cueTime;
    unsigned oneShotEndTime;
    int focusMode;
    int focusStage;
    unsigned focusEndTime;
    unsigned registrationSequence;
    vec3_t camAnimOffset;
    unsigned camAnimEndTime;
    int prevMouseX;
    bool prevMouseValid;
    float orbitYaw;
    float orbitVelocity;
    float displayOrbitYaw;
    bool dragging;
} uiModelPreview_t;

static uiModelPreview_t menuModel;

static const uiModelAnim_t modelAnims[] = {
    [UI_MODEL_STAND]  = {   0,  39, 100, true  },
    [UI_MODEL_RUN]    = {  40,  45, 100, true  },
    [UI_MODEL_JUMP]   = {  66,  71, 100, false },
    [UI_MODEL_WAVE]   = { 112, 122, 100, false },
    [UI_MODEL_SALUTE] = {  84,  94, 100, false },
    [UI_MODEL_POINT]  = { 123, 134, 100, false },
    [UI_MODEL_TAUNT]  = {  95, 111, 100, false },
    [UI_MODEL_FLIP]   = {  72,  83, 100, false },
    [UI_MODEL_ATTACK] = {  46,  53, 100, false },
    [UI_MODEL_CROUCH] = { 135, 153, 100, false },
    [UI_MODEL_PAIN] = { 54, 65, 100, false },
    [UI_MODEL_CROUCH_WALK] = { 154, 159, 100, true },
    [UI_MODEL_CROUCH_ATTACK] = { 160, 168, 100, false },
    [UI_MODEL_CROUCH_PAIN] = { 169, 177, 100, false },
    [UI_MODEL_DEATH1] = { 178, 183, 100, false },
    [UI_MODEL_DEATH2] = { 184, 189, 100, false },
    [UI_MODEL_DEATH3] = { 190, 197, 100, false }


};

static const uiModelCue_t menuCues[] = {
    { UI_MODEL_STAND, 2600 },
    { UI_MODEL_RUN,   1200 },
    { UI_MODEL_JUMP,   600 },
    { UI_MODEL_RUN,   1300 },
    { UI_MODEL_WAVE,  1100 },
    { UI_MODEL_RUN,   1200 },
    { UI_MODEL_STAND, 1800 },
    { UI_MODEL_SALUTE, 1100 },
    { UI_MODEL_RUN,    900 },
    { UI_MODEL_POINT, 1200 },
    { UI_MODEL_STAND, 2200 },
    { UI_MODEL_RUN,   1100 },
    { UI_MODEL_FLIP,  1200 },
    { UI_MODEL_RUN,   1200 },
    { UI_MODEL_STAND, 2400 },
    { UI_MODEL_CROUCH, 4000},
    { UI_MODEL_CROUCH_WALK, 4000},
    { UI_MODEL_CROUCH_PAIN,  4000},
    { UI_MODEL_TAUNT, 1700 }
};

static bool MenuWantsModel(const menuFrameWork_t *menu)
{
    if (!ui_builtin_menu_active) {
        return false;
    }

    if (!menu || !menu->name) {
        return false;
    }

    return !strcmp(menu->name, "main") ||
        !strcmp(menu->name, "game") ||
        !strcmp(menu->name, "play") ||
        !strcmp(menu->name, "model") ||
        !strcmp(menu->name, "multiplayer") ||
        !strcmp(menu->name, "options");
}

static void ParseCurrentSkin(char *model, size_t modelSize,
                             char *skin, size_t skinSize)
{
    char value[MAX_QPATH];
    char *p;

    Cvar_VariableStringBuffer("skin", value, sizeof(value));
    if ((p = strchr(value, '/')) || (p = strchr(value, '\\'))) {
        *p++ = 0;
        Q_strlcpy(model, value, modelSize);
        Q_strlcpy(skin, p, skinSize);
    } else {
        Q_strlcpy(model, "male", modelSize);
        Q_strlcpy(skin, "grunt", skinSize);
    }
}

static float Cvar_ClampedValue(cvar_t *cvar, float minValue, float maxValue)
{
    if (!cvar) {
        return minValue;
    }

    return Q_clipf(cvar->value, minValue, maxValue);
}

static const uiModelAnim_t *CueAnim(uiModelCueId_t cue)
{
    int maxCue = (int)q_countof(modelAnims) - 1;

    return &modelAnims[Q_clip((int)cue, 0, maxCue)];
}

static void SetCue(uiModelPreview_t *preview, uiModelCueId_t cue,
                   bool preserveOldFrame)
{
    const uiModelAnim_t *anim = CueAnim(cue);
    int oldFrame = preview->frame;
    int i;

    preview->cue = cue;
    preview->animEnd = anim->lastFrame;
    preview->frame = anim->firstFrame;
    preview->frameTime = uis.realtime;

    for (i = 0; i < preview->numEntities; i++) {
        preview->entities[i].oldframe = preserveOldFrame ? oldFrame : preview->frame;
        preview->entities[i].frame = preview->frame;
        preview->entities[i].backlerp = preserveOldFrame ? 1.0f : 0.0f;
    }
}

static void ResetAnimation(uiModelPreview_t *preview)
{
    preview->cueIndex = 0;
    preview->cueTime = uis.realtime;
    preview->oneShotEndTime = 0;
    preview->focusMode = 0;
    preview->camAnimEndTime = 0;
    preview->orbitYaw = 0.0f;
    preview->orbitVelocity = 0.0f;
    preview->displayOrbitYaw = 0.0f;
    preview->dragging = false;
    preview->ambientCue = menuCues[preview->cueIndex].cue;
    SetCue(preview, menuCues[preview->cueIndex].cue, false);
}

static void AdvanceCue(uiModelPreview_t *preview)
{
    preview->cueIndex++;
    if (preview->cueIndex >= q_countof(menuCues)) {
        preview->cueIndex = 0;
    }
    preview->cueTime = uis.realtime;
    preview->ambientCue = menuCues[preview->cueIndex].cue;
    SetCue(preview, menuCues[preview->cueIndex].cue, true);
}

static void StartOneShot(uiModelPreview_t *preview, uiModelCueId_t cue)
{
    const uiModelAnim_t *anim = CueAnim(cue);
    unsigned duration;

    preview->oneShotCue = cue;
    duration = (anim->lastFrame - anim->firstFrame + 1) * anim->frameMs;
    preview->oneShotEndTime = uis.realtime + duration;
    SetCue(preview, cue, true);
}

static void EndOneShot(uiModelPreview_t *preview)
{
    preview->oneShotEndTime = 0;
    SetCue(preview, preview->ambientCue, true);
}

static const char *ItemCommand(const menuCommon_t *item)
{
    if (!item) {
        return NULL;
    }

    switch (item->type) {
    case MTYPE_ACTION:
    case MTYPE_SAVEGAME:
    case MTYPE_LOADGAME:
        return ((const menuAction_t *)item)->cmd;
    case MTYPE_BITMAP:
        return ((const menuBitmap_t *)item)->cmd;
    default:
        return NULL;
    }
}

static bool ItemNameIs(const menuCommon_t *item, const char *name)
{
    return item && item->name && !Q_stricmp(item->name, name);
}

static bool ItemCommandHas(const menuCommon_t *item, const char *text)
{
    const char *cmd = ItemCommand(item);

    return cmd && Q_stristr(cmd, text);
}

static void LoadPreviewModel(uiModelPreview_t *preview)
{
    char model[MAX_QPATH];
    char skin[MAX_QPATH];
    char scratch[MAX_QPATH];

    ParseCurrentSkin(model, sizeof(model), skin, sizeof(skin));
    Q_concat(preview->skin, sizeof(preview->skin), model, "/", skin);
    preview->registrationSequence = R_RegistrationSequence();

    memset(preview->entities, 0, sizeof(preview->entities));
    preview->numEntities = 0;

    Q_concat(scratch, sizeof(scratch), "players/", model, "/tris.md2");
    preview->entities[0].model = R_RegisterModel(scratch);
    if (!preview->entities[0].model) {
        return;
    }

    Q_concat(scratch, sizeof(scratch), "players/", model, "/", skin);
    preview->entities[0].skin = R_RegisterSkin(scratch);
    preview->entities[0].flags = RF_FULLBRIGHT;
    preview->numEntities++;

    preview->refdef.entities = preview->entities;
    preview->refdef.rdflags = RDF_NOWORLDMODEL;
    ResetAnimation(preview);
}

static void ApplyModelCvars(uiModelPreview_t *preview)
{
    vec3_t origin = { 0.0f, 0.0f, 0.0f };
    vec3_t angles = { 0.0f, 0.0f, 0.0f };
    int i;

    origin[0] = Cvar_ClampedValue(ui_menu_model_distance, 32.0f, 160.0f);
    angles[YAW] = Cvar_ClampedValue(ui_menu_model_yaw, 0.0f, 360.0f);
    angles[YAW] += preview->displayOrbitYaw;

    if (preview->camAnimEndTime > uis.realtime) {
        float t = (float)(preview->camAnimEndTime - uis.realtime) / 350.0f;
        float ease = t * t;
        origin[0] += preview->camAnimOffset[0] * ease;
        angles[YAW] += preview->camAnimOffset[1] * ease;
    }

    for (i = 0; i < preview->numEntities; i++) {
        VectorCopy(origin, preview->entities[i].origin);
        VectorCopy(origin, preview->entities[i].oldorigin);
        VectorCopy(angles, preview->entities[i].angles);
    }
}

static void EnsurePreviewLoaded(uiModelPreview_t *preview)
{
    char model[MAX_QPATH];
    char skin[MAX_QPATH];
    char current[MAX_QPATH];

    ParseCurrentSkin(model, sizeof(model), skin, sizeof(skin));
    Q_concat(current, sizeof(current), model, "/", skin);

    if (!preview->initialized || strcmp(preview->skin, current) ||
        preview->registrationSequence != R_RegistrationSequence()) {
        preview->initialized = true;
        LoadPreviewModel(preview);
    }
}

static bool PointInPreviewViewport(const uiModelPreview_t *preview, int x, int y)
{
    int vp_x = preview->viewport.x;
    int vp_y = preview->viewport.y;
    int vp_w = preview->viewport.width;
    int vp_h = preview->viewport.height;

    if (vp_w <= 0 || vp_h <= 0) {
        return false;
    }

    return x >= vp_x && x < vp_x + vp_w &&
        y >= vp_y && y < vp_y + vp_h;
}

static void UpdateAnimation(uiModelPreview_t *preview)
{
    const uiModelCue_t *cue;
    const uiModelAnim_t *anim;
    unsigned frameMs;
    int nextFrame;
    int i;

    if (!preview->numEntities) {
        return;
    }

    if (preview->cueIndex < 0 || preview->cueIndex >= q_countof(menuCues)) {
        ResetAnimation(preview);
    }

    if (preview->oneShotEndTime) {
        if (uis.realtime >= preview->oneShotEndTime) {
            if (preview->focusMode == 1) {
                if (preview->focusStage == 0) {
                    preview->focusStage = 1;
                    preview->focusEndTime = uis.realtime + 3000;
                    preview->oneShotCue = UI_MODEL_STAND;
                    preview->oneShotEndTime = preview->focusEndTime;
                    SetCue(preview, UI_MODEL_STAND, true);
                } else {
                    preview->focusStage = 0;
                    StartOneShot(preview, UI_MODEL_WAVE);
                }
            } else if (preview->focusMode == 2) {
                StartOneShot(preview, preview->oneShotCue);
            } else if (preview->focusMode == 3) {
                preview->focusMode = 0;
                preview->oneShotEndTime = UINT_MAX;
                preview->frame = CueAnim(preview->cue)->lastFrame;
                preview->animEnd = preview->frame;
                preview->frameTime = uis.realtime;
                for (i = 0; i < preview->numEntities; i++) {
                    preview->entities[i].oldframe = preview->frame;
                    preview->entities[i].frame = preview->frame;
                    preview->entities[i].backlerp = 0.0f;
                }
            } else {
                EndOneShot(preview);
            }
        }
    } else {
        cue = &menuCues[preview->cueIndex];
        if (uis.realtime - preview->cueTime >= cue->duration) {
            AdvanceCue(preview);
        }
    }

    anim = CueAnim(preview->cue);
    frameMs = anim->frameMs;
    if (frameMs < 16) {
        frameMs = 16;
    }

    while (uis.realtime - preview->frameTime >= frameMs) {
        preview->frameTime += frameMs;
        nextFrame = preview->frame + 1;
        if (nextFrame > preview->animEnd || nextFrame < anim->firstFrame) {
            nextFrame = anim->loop ? anim->firstFrame : preview->animEnd;
        }
        if (nextFrame == preview->frame && !anim->loop) {
            break;
        }

        for (i = 0; i < preview->numEntities; i++) {
            preview->entities[i].oldframe = preview->entities[i].frame;
            preview->entities[i].frame = nextFrame;
        }
        preview->frame = nextFrame;
    }

    for (i = 0; i < preview->numEntities; i++) {
        preview->entities[i].backlerp =
            Q_clipf(1.0f - (float)(uis.realtime - preview->frameTime) /
                    (float)frameMs, 0.0f, 1.0f);
    }
}

static void UpdateOrbit(uiModelPreview_t *preview)
{
    preview->orbitYaw += preview->orbitVelocity;
    preview->orbitVelocity *= 0.91f;

    if (fabsf(preview->orbitVelocity) < 0.01f) {
        preview->orbitVelocity = 0.0f;
    }

    preview->displayOrbitYaw +=
        (preview->orbitYaw - preview->displayOrbitYaw) * 0.33f;
}

void UI_DrawMenuBackgroundModel(const menuFrameWork_t *menu)
{
    int w, h;
    float x, y, scale;
    refdef_t refdef;

    if (!ui_menu_model || !ui_menu_model->integer || !MenuWantsModel(menu)) {
        return;
    }

    EnsurePreviewLoaded(&menuModel);
    if (!menuModel.numEntities) {
        return;
    }

    w = uis.width;
    h = uis.height;

    x = Cvar_ClampedValue(ui_menu_model_x, 0.0f, 1.0f);
    y = Cvar_ClampedValue(ui_menu_model_y, 0.0f, 1.0f);
    scale = Cvar_ClampedValue(ui_menu_model_scale, 0.25f, 1.75f);

    menuModel.viewport.width = w * 3 / 8 * scale;
    menuModel.viewport.height = h * 3 / 4 * scale;
    menuModel.viewport.x = w * x - menuModel.viewport.width / 2;
    menuModel.viewport.y = h * y - menuModel.viewport.height / 2;

    if (!ui_menu_model_orbit || !ui_menu_model_orbit->integer) {
        menuModel.prevMouseValid = false;
    }

    UpdateAnimation(&menuModel);
    UpdateOrbit(&menuModel);
    ApplyModelCvars(&menuModel);

    menuModel.refdef.fov_x = 115;
    menuModel.refdef.fov_y = V_CalcFov(menuModel.refdef.fov_x,
                                       menuModel.viewport.width,
                                       menuModel.viewport.height);
    menuModel.refdef.time = uis.realtime * 0.001f;
    menuModel.refdef.num_entities = menuModel.numEntities;

    refdef = menuModel.refdef;
    refdef.x = Q_rint(menuModel.viewport.x / uis.scale);
    refdef.y = Q_rint(menuModel.viewport.y / uis.scale);
    refdef.width = Q_rint(menuModel.viewport.width / uis.scale);
    refdef.height = Q_rint(menuModel.viewport.height / uis.scale);
    R_RenderFrame(&refdef);
    R_SetScale(uis.scale);
}

typedef struct {
    const char *name;
    const char *commandContains;
    int focusMode;
    uiModelCueId_t cue;
} uiMenuItemCue_t;

static const uiMenuItemCue_t menuItemCues[] = {
    /* main menu */
    { "multiplayer",    "pushmenu multiplayer", 1, UI_MODEL_WAVE },
    { "options",        "pushmenu options",     2, UI_MODEL_POINT },
    { "video setup",    NULL,                   2, UI_MODEL_STAND },
    { "quit",           "quit",                 3, UI_MODEL_CROUCH_PAIN },

    /* in-game menu */
    { "Join game",            "pushmenu game_server",  1, UI_MODEL_SALUTE },
    { "HUD & Jump Tools",     "pushmenu jump",         2, UI_MODEL_POINT },
    { "Quick Video Settings", "pushmenu video_quick",  2, UI_MODEL_CROUCH_WALK },
    { "System Options",       "pushmenu options",      2, UI_MODEL_CROUCH },
    { "Find Jump Servers",    "@jump",                 1, UI_MODEL_RUN },
    { "Address Book",         "favorites://",          1, UI_MODEL_SALUTE },
    { "Disconnect",           "disconnect",            3, UI_MODEL_PAIN },
    { "Quit Game",            "quit",                  3, UI_MODEL_CROUCH_PAIN },

    /* multiplayer submenu */
    { "browse q2servers.com", "+http://q2servers.com/?raw=2", 1, UI_MODEL_POINT },
    { "browse jump servers",  "mod=jump",                     1, UI_MODEL_RUN },
    { "browse address book",  "favorites://",                 1, UI_MODEL_POINT },
    { "browse demos",         "pushmenu demos",               1, UI_MODEL_SALUTE },

    /* options submenu */
    { "player setup",     "pushmenu players",      2, UI_MODEL_SALUTE },
    { "jump setup",       "pushmenu jump",         2, UI_MODEL_JUMP },
    { "input setup",      "pushmenu input",        2, UI_MODEL_ATTACK },
    { "key bindings",     "pushmenu keys",         2, UI_MODEL_SALUTE },
    { "sound setup",      "pushmenu sound",        2, UI_MODEL_STAND },
    { "effects setup",    "pushmenu effects",      2, UI_MODEL_FLIP },
    { "screen setup",     "pushmenu screen",       2, UI_MODEL_CROUCH },
    { "menu setup",       "pushmenu menusetup",    2, UI_MODEL_STAND },
    { "download options", "pushmenu downloads",    2, UI_MODEL_RUN },
    { "address book",     "pushmenu addressbook",  1, UI_MODEL_POINT },

    /* game_server actions */
    { "Join Team Hard",   "team hard",      1, UI_MODEL_SALUTE },
    { "Join Team Easy",   "team easy",      1, UI_MODEL_WAVE },
    { "Vote YES",         "yes",            1, UI_MODEL_JUMP },
    { "Vote NO",          "no",             3, UI_MODEL_TAUNT },
    { "Vote More Time",   "votetime",       1, UI_MODEL_POINT },
    { "Vote Random Map",  "mapvote random", 1, UI_MODEL_FLIP },
};

void UI_ModelPreview_MenuItemFocused(const menuFrameWork_t *menu,
                                     const menuCommon_t *item)
{
    int i;

    if (!ui_menu_model || !ui_menu_model->integer || !MenuWantsModel(menu) ||
        !item || !item->name) {
        return;
    }

    EnsurePreviewLoaded(&menuModel);
    if (!menuModel.numEntities) {
        return;
    }

    for (i = 0; i < (int)q_countof(menuItemCues); i++) {
        const uiMenuItemCue_t *cue = &menuItemCues[i];

        if (ItemNameIs(item, cue->name) ||
            (cue->commandContains && ItemCommandHas(item, cue->commandContains))) {
            menuModel.focusMode = cue->focusMode;
            menuModel.focusStage = 0;
            StartOneShot(&menuModel, cue->cue);
            return;
        }
    }
}

void UI_ModelPreview_MouseMove(const menuFrameWork_t *menu, int x, int y)
{
    if (!ui_menu_model || !ui_menu_model->integer ||
        !ui_menu_model_orbit || !ui_menu_model_orbit->integer ||
        !MenuWantsModel(menu) || !menuModel.dragging) {
        menuModel.prevMouseValid = false;
        return;
    }

    if (menuModel.prevMouseValid) {
        menuModel.orbitVelocity += (float)(x - menuModel.prevMouseX) * 0.20f;
    }

    menuModel.prevMouseX = x;
    menuModel.prevMouseValid = true;
}

bool UI_ModelPreview_MouseDown(const menuFrameWork_t *menu, int x, int y)
{
    if (!ui_menu_model || !ui_menu_model->integer ||
        !ui_menu_model_orbit || !ui_menu_model_orbit->integer ||
        !MenuWantsModel(menu)) {
        return false;
    }

    if (!PointInPreviewViewport(&menuModel, x, y)) {
        return false;
    }

    if (Menu_HitTest((menuFrameWork_t *)menu)) {
        return false;
    }

    menuModel.dragging = true;
    menuModel.prevMouseX = x;
    menuModel.prevMouseValid = true;
    return true;
}

void UI_ModelPreview_MouseUp(void)
{
    menuModel.dragging = false;
}

void UI_ModelPreview_Shutdown(void)
{
    memset(&menuModel, 0, sizeof(menuModel));
}
