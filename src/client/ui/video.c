/* Native display menu. Draft controls never write engine cvars. */
#include "ui.h"
#include "client/video.h"

enum { DISPLAY_MODE, DISPLAY_MONITOR, DISPLAY_SIZE, DISPLAY_REFRESH, DISPLAY_VSYNC, DISPLAY_ROWS };
enum { DISPLAY_APPLY, DISPLAY_BACK, DISPLAY_GRAPHICS, DISPLAY_KEEP, DISPLAY_REVERT };

static struct {
    menuFrameWork_t menu, confirm;
    menuSpinControl_t rows[DISPLAY_ROWS];
    menuAction_t apply, back, graphics, keep, revert;
    menuSeparator_t countdown;
    vid_display_settings_t original, draft;
    vid_display_t *displays;
    vid_display_resolution_t *modes, *sizes;
    int num_displays, num_modes, num_sizes;
    int *rates, num_rates;
    unsigned poll_time;
    bool was_pending;
    char countdown_text[64], custom_vsync[32];
} video_menu;

static char *mode_names[] = { "Windowed", "Borderless fullscreen", "Exclusive fullscreen", NULL };
static char *vsync_names[] = { "Off", "On", NULL, NULL };

static void FreeNames(menuSpinControl_t *row)
{
    if (!row->itemnames)
        return;
    for (int i = 0; row->itemnames[i]; i++)
        Z_Free(row->itemnames[i]);
    Z_Free(row->itemnames);
    row->itemnames = NULL;
}

static void AddName(menuSpinControl_t *row, const char *name)
{
    int count = row->numItems++;
    row->itemnames = Z_Realloc(row->itemnames, (count + 2) * sizeof(char *));
    row->itemnames[count] = UI_CopyString(name);
    row->itemnames[count + 1] = NULL;
}

static void ClearNames(menuSpinControl_t *row)
{
    FreeNames(row);
    row->numItems = 0;
    row->curvalue = 0;
}

static void AddSize(int width, int height)
{
    for (int i = 0; i < video_menu.num_sizes; i++)
        if (video_menu.sizes[i].width == width && video_menu.sizes[i].height == height)
            return;
    video_menu.sizes = Z_Realloc(video_menu.sizes, (video_menu.num_sizes + 1) * sizeof(*video_menu.sizes));
    video_menu.sizes[video_menu.num_sizes++] = (vid_display_resolution_t){ width, height, 0 };
}

static void UpdateApply(void)
{
    bool valid = video_menu.num_displays > 0 &&
        (video_menu.draft.mode != VID_DISPLAY_EXCLUSIVE || video_menu.num_rates > 0);
    if (valid && !VID_DisplaySettingsEqual(&video_menu.original, &video_menu.draft))
        video_menu.apply.generic.flags &= ~QMF_GRAYED;
    else
        video_menu.apply.generic.flags |= QMF_GRAYED;
}

static void RebuildRates(void)
{
    menuSpinControl_t *row = &video_menu.rows[DISPLAY_REFRESH];
    ClearNames(row);
    Z_Free(video_menu.rates);
    video_menu.rates = NULL;
    video_menu.num_rates = 0;
    if (video_menu.draft.mode != VID_DISPLAY_EXCLUSIVE) {
        int refresh = video_menu.num_displays ?
            video_menu.displays[video_menu.rows[DISPLAY_MONITOR].curvalue].refresh : 0;
        AddName(row, refresh > 1 ? va("%d Hz (Desktop)", refresh) : "Desktop");
        row->generic.flags |= QMF_GRAYED;
        return;
    }
    row->generic.flags &= ~QMF_GRAYED;
    for (int i = 0; i < video_menu.num_modes; i++) {
        const vid_display_resolution_t *mode = &video_menu.modes[i];
        if (mode->width != video_menu.draft.width || mode->height != video_menu.draft.height)
            continue;
        int j;
        for (j = 0; j < video_menu.num_rates; j++)
            if (video_menu.rates[j] == mode->refresh)
                break;
        if (j != video_menu.num_rates)
            continue;
        video_menu.rates = Z_Realloc(video_menu.rates, (video_menu.num_rates + 1) * sizeof(int));
        video_menu.rates[video_menu.num_rates++] = mode->refresh;
        AddName(row, va("%d Hz", mode->refresh));
        if (mode->refresh == video_menu.draft.refresh)
            row->curvalue = row->numItems - 1;
    }
    if (video_menu.num_rates)
        video_menu.draft.refresh = video_menu.rates[row->curvalue];
    else {
        AddName(row, "Unavailable");
        row->generic.flags |= QMF_GRAYED;
    }
}

static void RebuildSizes(void)
{
    static const int window_sizes[][2] = {
        { 640, 480 }, { 800, 600 }, { 1024, 768 }, { 1280, 720 },
        { 1280, 960 }, { 1600, 900 }, { 1920, 1080 }, { 2560, 1440 }
    };
    menuSpinControl_t *row = &video_menu.rows[DISPLAY_SIZE];
    ClearNames(row);
    Z_Free(video_menu.sizes);
    video_menu.sizes = NULL;
    video_menu.num_sizes = 0;
    row->generic.name = video_menu.draft.mode == VID_DISPLAY_WINDOWED ? "Window size" : "Resolution";
    row->generic.flags &= ~QMF_GRAYED;
    if (video_menu.draft.mode == VID_DISPLAY_BORDERLESS) {
        if (video_menu.num_displays) {
            const vid_display_t *display = &video_menu.displays[video_menu.rows[DISPLAY_MONITOR].curvalue];
            AddName(row, va("%d x %d (Desktop)", display->desktop.width, display->desktop.height));
        } else {
            AddName(row, "Unavailable");
        }
        row->generic.flags |= QMF_GRAYED;
    } else {
        int width = video_menu.draft.width, height = video_menu.draft.height;
        if (video_menu.draft.mode == VID_DISPLAY_WINDOWED) {
            width = video_menu.draft.window.width;
            height = video_menu.draft.window.height;
            // Preserve the opening size and ordering across monitor polls.
            AddSize(video_menu.original.window.width, video_menu.original.window.height);
            for (int i = 0; i < q_countof(window_sizes); i++)
                AddSize(window_sizes[i][0], window_sizes[i][1]);
        } else {
            for (int i = 0; i < video_menu.num_modes; i++)
                AddSize(video_menu.modes[i].width, video_menu.modes[i].height);
        }
        for (int i = 0; i < video_menu.num_sizes; i++) {
            AddName(row, va("%d x %d", video_menu.sizes[i].width, video_menu.sizes[i].height));
            if (width == video_menu.sizes[i].width && height == video_menu.sizes[i].height)
                row->curvalue = i;
        }
        if (video_menu.num_sizes && video_menu.draft.mode == VID_DISPLAY_EXCLUSIVE) {
            video_menu.draft.width = video_menu.sizes[row->curvalue].width;
            video_menu.draft.height = video_menu.sizes[row->curvalue].height;
        } else if (!video_menu.num_sizes) {
            AddName(row, "Unavailable");
            row->generic.flags |= QMF_GRAYED;
        }
    }
    RebuildRates();
    UpdateApply();
}

static void LoadMonitorModes(void)
{
    Z_Free(video_menu.modes);
    video_menu.modes = vid->get_display_modes(video_menu.draft.display, &video_menu.num_modes);
    RebuildSizes();
}

static void LoadDisplays(void)
{
    menuSpinControl_t *row = &video_menu.rows[DISPLAY_MONITOR];
    ClearNames(row);
    Z_Free(video_menu.displays);
    video_menu.displays = vid->get_displays(&video_menu.num_displays);
    int selected = -1;
    for (int i = 0; i < video_menu.num_displays; i++) {
        AddName(row, va("%d - %.30s%s", i + 1, video_menu.displays[i].name,
                        video_menu.displays[i].primary ? " (Primary)" : ""));
        if (!strcmp(video_menu.displays[i].id, video_menu.draft.display))
            selected = i;
    }
    row->generic.flags &= ~QMF_GRAYED;
    if (selected < 0)
        for (int i = 0; i < video_menu.num_displays; i++)
            if (video_menu.displays[i].primary)
                selected = i;
    row->curvalue = max(selected, 0);
    if (video_menu.num_displays)
        Q_strlcpy(video_menu.draft.display, video_menu.displays[row->curvalue].id, sizeof(video_menu.draft.display));
    else
        AddName(row, "Unavailable");
    if (video_menu.num_displays <= 1)
        row->generic.flags |= QMF_GRAYED;
    LoadMonitorModes();
}

static menuSound_t Change(menuCommon_t *item)
{
    switch (item->id) {
    case DISPLAY_MODE:
        video_menu.draft.mode = video_menu.rows[DISPLAY_MODE].curvalue;
        video_menu.draft.depth = 0;
        video_menu.draft.window_flags = 0;
        RebuildSizes();
        break;
    case DISPLAY_MONITOR:
        video_menu.draft.depth = 0;
        Q_strlcpy(video_menu.draft.display,
                  video_menu.displays[video_menu.rows[DISPLAY_MONITOR].curvalue].id,
                  sizeof(video_menu.draft.display));
        LoadMonitorModes();
        break;
    case DISPLAY_SIZE: {
        const vid_display_resolution_t *size = &video_menu.sizes[video_menu.rows[DISPLAY_SIZE].curvalue];
        if (video_menu.draft.mode == VID_DISPLAY_WINDOWED) {
            video_menu.draft.window.width = size->width;
            video_menu.draft.window.height = size->height;
        } else {
            video_menu.draft.width = size->width;
            video_menu.draft.height = size->height;
        }
        RebuildRates();
        break;
    }
    case DISPLAY_REFRESH:
        video_menu.draft.refresh = video_menu.rates[video_menu.rows[DISPLAY_REFRESH].curvalue];
        break;
    case DISPLAY_VSYNC:
        video_menu.draft.vsync = video_menu.rows[DISPLAY_VSYNC].curvalue == 2 ?
            video_menu.original.vsync : video_menu.rows[DISPLAY_VSYNC].curvalue;
        break;
    }
    UpdateApply();
    Menu_Layout(&video_menu.menu);
    return QMS_MOVE;
}

static bool Push(menuFrameWork_t *menu)
{
    if (!vid->get_display_settings(&video_menu.original))
        return false;
    video_menu.draft = video_menu.original;
    if (video_menu.draft.mode == VID_DISPLAY_WINDOWED)
        video_menu.draft.window_flags = 0;
    video_menu.rows[DISPLAY_MODE].curvalue = video_menu.draft.mode;
    if (video_menu.draft.vsync != 0 && video_menu.draft.vsync != 1) {
        Q_snprintf(video_menu.custom_vsync, sizeof(video_menu.custom_vsync), "Custom (%d)", video_menu.draft.vsync);
        vsync_names[2] = video_menu.custom_vsync;
        video_menu.rows[DISPLAY_VSYNC].curvalue = 2;
    } else {
        vsync_names[2] = NULL;
        video_menu.rows[DISPLAY_VSYNC].curvalue = video_menu.draft.vsync;
    }
    LoadDisplays();
    video_menu.poll_time = Sys_Milliseconds();
    return true;
}

static menuSound_t Action(menuCommon_t *item)
{
    switch (item->id) {
    case DISPLAY_APPLY:
        VID_ApplyDisplaySettings(&video_menu.draft);
        if (!VID_PendingDisplaySettings()) {
            Push(&video_menu.menu);
            Menu_Init(&video_menu.menu);
            video_menu.menu.status = (char *)VID_DisplayMessage();
        }
        return QMS_IN;
    case DISPLAY_BACK:
        UI_PopMenu();
        return QMS_OUT;
    case DISPLAY_GRAPHICS:
        UI_PushMenu(UI_FindMenu("video_graphics"));
        return QMS_IN;
    case DISPLAY_KEEP:
        VID_KeepDisplaySettings();
        break;
    case DISPLAY_REVERT:
        VID_RevertDisplaySettings();
        break;
    }
    return QMS_OUT;
}

static menuSound_t ConfirmKey(menuFrameWork_t *menu, int key)
{
    if (key == K_ESCAPE || key == K_MOUSE2) {
        VID_RevertDisplaySettings();
        return QMS_OUT;
    }
    return QMS_NOTHANDLED;
}

static void Free(menuFrameWork_t *menu)
{
    if (menu == &video_menu.confirm) {
        Z_Free(menu->items);
        memset(menu, 0, sizeof(*menu));
        return;
    }
    for (int i = DISPLAY_MONITOR; i <= DISPLAY_REFRESH; i++)
        FreeNames(&video_menu.rows[i]);
    Z_Free(video_menu.displays);
    Z_Free(video_menu.modes);
    Z_Free(video_menu.sizes);
    Z_Free(video_menu.rates);
    Z_Free(menu->items);
    video_menu.displays = NULL;
    video_menu.modes = video_menu.sizes = NULL;
    video_menu.rates = NULL;
    memset(menu, 0, sizeof(*menu));
}

static void InitAction(menuFrameWork_t *menu, menuAction_t *action, const char *name, int id)
{
    action->generic.type = MTYPE_ACTION;
    action->generic.name = (char *)name;
    action->generic.id = id;
    action->generic.activate = Action;
    Menu_AddItem(menu, action);
}

void M_Menu_Video(void)
{
    if (!VID_HasDisplaySettings())
        return;
    menuFrameWork_t *old = UI_FindMenu("video");
    if (old) {
        List_Remove(&old->entry);
        if (old->free) old->free(old);
    }
    memset(&video_menu, 0, sizeof(video_menu));
    menuFrameWork_t *menu = &video_menu.menu;
    menu->name = "video";
    menu->title = "Video - Display";
    menu->status = "Apply tests changes. Back or Escape discards changes not applied.";
    menu->push = Push;
    menu->free = Free;
    menu->color = uis.color.background;
    menu->compact = true;
    menu->transparent = true;
    static const char *names[] = { "Display mode", "Monitor", "Resolution", "Refresh rate", "VSync" };
    static const char *hints[] = {
        "Windowed has a title bar. Borderless uses the desktop. Exclusive changes the display mode.",
        "Choose the monitor used by the game.",
        "Window size in Windowed; desktop resolution in Borderless; display resolution in Exclusive.",
        "Only Exclusive fullscreen changes the monitor refresh rate.",
        "Synchronize rendering with the monitor to prevent tearing."
    };
    for (int i = 0; i < DISPLAY_ROWS; i++) {
        menuSpinControl_t *row = &video_menu.rows[i];
        row->generic.type = MTYPE_SPINCONTROL;
        row->generic.name = (char *)names[i];
        row->generic.status = (char *)hints[i];
        row->generic.id = i;
        row->generic.change = Change;
        Menu_AddItem(menu, row);
    }
    video_menu.rows[DISPLAY_MODE].itemnames = mode_names;
    video_menu.rows[DISPLAY_VSYNC].itemnames = vsync_names;
    InitAction(menu, &video_menu.graphics, "Graphics settings...", DISPLAY_GRAPHICS);
    InitAction(menu, &video_menu.apply, "Apply changes", DISPLAY_APPLY);
    InitAction(menu, &video_menu.back, "Back", DISPLAY_BACK);
    List_Append(&ui_menus, &menu->entry);

    menu = &video_menu.confirm;
    menu->name = "video_confirm";
    menu->title = "Keep these display settings?";
    menu->status = "Escape or timeout restores the previous display.";
    menu->keydown = ConfirmKey;
    menu->free = Free;
    menu->color = uis.color.background;
    menu->compact = true;
    video_menu.countdown.generic.type = MTYPE_SEPARATOR;
    video_menu.countdown.generic.name = video_menu.countdown_text;
    Menu_AddItem(menu, &video_menu.countdown);
    InitAction(menu, &video_menu.keep, "Keep changes", DISPLAY_KEEP);
    InitAction(menu, &video_menu.revert, "Revert", DISPLAY_REVERT);
    List_Append(&ui_menus, &menu->entry);
}

void M_VideoFrame(void)
{
    if (!VID_HasDisplaySettings() || !video_menu.menu.name)
        return;
    bool pending = VID_PendingDisplaySettings() != NULL;
    if (pending) {
        Q_snprintf(video_menu.countdown_text, sizeof(video_menu.countdown_text),
                   "Reverting in %d seconds", VID_DisplaySecondsLeft());
        if (uis.activeMenu != &video_menu.confirm)
            UI_PushMenu(&video_menu.confirm);
        Menu_Layout(&video_menu.confirm);
    } else if (video_menu.was_pending) {
        if (uis.activeMenu == &video_menu.confirm)
            UI_PopMenu();
        if (uis.activeMenu == &video_menu.menu) {
            Push(&video_menu.menu);
            Menu_Init(&video_menu.menu);
            video_menu.menu.status = (char *)VID_DisplayMessage();
        }
    }
    video_menu.was_pending = pending;
    if (!pending && uis.activeMenu == &video_menu.menu &&
        Sys_Milliseconds() - video_menu.poll_time >= 1000) {
        // Monitor identities can change while this menu is open.
        LoadDisplays();
        Menu_Layout(&video_menu.menu);
        video_menu.poll_time = Sys_Milliseconds();
    }
}
