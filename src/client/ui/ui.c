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

#include "ui.h"
#include "client/input.h"
#include "common/prompt.h"

uiStatic_t    uis;

LIST_DECL(ui_menus);

cvar_t    *ui_debug;
cvar_t    *cl_menu_cursor;
cvar_t    *ui_menu_style;
static cvar_t    *ui_open;
static cvar_t    *ui_scale;
static cvar_t    *ui_draw_layers;

typedef enum {
    UI_MENU_STYLE_CLASSIC,
    UI_MENU_STYLE_SLATE,
    UI_MENU_STYLE_CUSTOM,
    UI_MENU_STYLE_COUNT
} uiMenuStyleId_t;

typedef enum {
    UI_MENU_COLOR_BACKGROUND,
    UI_MENU_COLOR_TITLE,
    UI_MENU_COLOR_NORMAL,
    UI_MENU_COLOR_SELECTABLE,
    UI_MENU_COLOR_ALTERNATE,
    UI_MENU_COLOR_ACTIVE,
    UI_MENU_COLOR_SELECTION,
    UI_MENU_COLOR_FOCUS,
    UI_MENU_COLOR_FOCUS_BORDER,
    UI_MENU_COLOR_DISABLED,
    UI_MENU_COLOR_LIST_HEADER,
    UI_MENU_COLOR_SCROLLBAR,
    UI_MENU_COLOR_HINT_BACKGROUND,
    UI_MENU_COLOR_HINT_TEXT,
    UI_MENU_COLOR_COUNT
} uiMenuColorId_t;

typedef struct {
    const char *cvarName;
    const char *defaultValue;
} uiMenuCustomColor_t;

static cvar_t *ui_menu_custom_colors[UI_MENU_COLOR_COUNT];

static const uiColorStyle_t ui_slateColorStyle = {
    { .u32 = MakeColor(28,  31,  34, 210) },
    { .u32 = MakeColor(214, 218, 224, 255) },
    { .u32 = MakeColor(232, 235, 240, 255) },
    { .u32 = MakeColor(232, 235, 240, 255) },
    { .u32 = MakeColor(255, 255, 255, 255) },
    { .u32 = MakeColor(139, 150, 163, 255) },
    { .u32 = MakeColor(95,  102, 112, 220) },
    { .u32 = MakeColor(80,  84,  90,  150) },
    { .u32 = MakeColor(180, 184, 190, 220) },
    { .u32 = MakeColor(112, 116, 122, 255) }
};

static const uint32_t ui_slateListHeaderColor = MakeColor(62, 68, 76, 255);
static const uint32_t ui_slateScrollbarColor = MakeColor(50, 55, 62, 255);

static const uiMenuCustomColor_t ui_menu_custom_color_defs[UI_MENU_COLOR_COUNT] = {
    [UI_MENU_COLOR_BACKGROUND]      = { "ui_menu_color_panel_bg",        "28 31 34 210" },
    [UI_MENU_COLOR_TITLE]           = { "ui_menu_color_title_text",      "214 218 224 255" },
    [UI_MENU_COLOR_NORMAL]          = { "ui_menu_color_text",            "232 235 240 255" },
    [UI_MENU_COLOR_SELECTABLE]      = { "ui_menu_color_item_text",       "232 235 240 255" },
    [UI_MENU_COLOR_ALTERNATE]       = { "ui_menu_color_label_text",      "255 255 255 255" },
    [UI_MENU_COLOR_ACTIVE]          = { "ui_menu_color_focus_text",      "139 150 163 255" },
    [UI_MENU_COLOR_SELECTION]       = { "ui_menu_color_selection_bg",    "95 102 112 220" },
    [UI_MENU_COLOR_FOCUS]           = { "ui_menu_color_focus_bg",        "80 84 90 150" },
    [UI_MENU_COLOR_FOCUS_BORDER]    = { "ui_menu_color_focus_edge",      "180 184 190 220" },
    [UI_MENU_COLOR_DISABLED]        = { "ui_menu_color_disabled_text",   "112 116 122 255" },
    [UI_MENU_COLOR_LIST_HEADER]     = { "ui_menu_color_list_header_bg",  "62 68 76 255" },
    [UI_MENU_COLOR_SCROLLBAR]       = { "ui_menu_color_scrollbar_track", "50 55 62 255" },
    [UI_MENU_COLOR_HINT_BACKGROUND] = { "ui_menu_color_hint_bg",         "18 20 23 235" },
    [UI_MENU_COLOR_HINT_TEXT]       = { "ui_menu_color_hint_text",       "232 235 240 255" }
};

// ===========================================================================

/*
=================
UI_PushMenu
=================
*/
void UI_PushMenu(menuFrameWork_t *menu)
{
    int i;

    if (!menu) {
        return;
    }

    // if this menu is already present, drop back to that level
    // to avoid stacking menus by hotkeys
    for (i = 0; i < uis.menuDepth; i++) {
        if (uis.layers[i] == menu) {
            break;
        }
    }

    if (i == uis.menuDepth) {
        if (uis.menuDepth >= MAX_MENU_DEPTH) {
            Com_EPrintf("UI_PushMenu: MAX_MENU_DEPTH exceeded\n");
            return;
        }
        uis.layers[uis.menuDepth++] = menu;
    } else {
        while (uis.menuDepth > i + 1) {
            UI_PopMenu();
        }
        uis.menuDepth = i + 1;
    }

    if (menu->push && !menu->push(menu)) {
        uis.menuDepth--;
        return;
    }

    Menu_Init(menu);

    Key_SetDest((Key_GetDest() & ~KEY_CONSOLE) | KEY_MENU);

    Con_Close(true);

    if (!uis.activeMenu) {
        // opening menu moves cursor to the nice location
        IN_WarpMouse(menu->mins[0] / uis.scale, menu->mins[1] / uis.scale);

        uis.mouseCoords[0] = menu->mins[0];
        uis.mouseCoords[1] = menu->mins[1];

        uis.entersound = true;
    }

    uis.activeMenu = menu;

    UI_DoHitTest();

    if (menu->expose) {
        menu->expose(menu);
    }
}

static void UI_Resize(void)
{
    int i;

    uis.scale = R_ClampScale(ui_scale);
    uis.width = Q_rint(r_config.width * uis.scale);
    uis.height = Q_rint(r_config.height * uis.scale);

    for (i = 0; i < uis.menuDepth; i++) {
        Menu_Init(uis.layers[i]);
    }

    //CL_WarpMouse(0, 0);
}


/*
=================
UI_ForceMenuOff
=================
*/
void UI_ForceMenuOff(void)
{
    menuFrameWork_t *menu;
    int i;

    for (i = uis.menuDepth; i > 0; i--) {
        menu = uis.layers[i - 1];
        if (menu->pop) {
            menu->pop(menu);
        }
    }

    Key_SetDest(Key_GetDest() & ~KEY_MENU);
    uis.menuDepth = 0;
    uis.activeMenu = NULL;
    uis.mouseTracker = NULL;
    uis.transparent = false;
}

/*
=================
UI_PopMenu
=================
*/
void UI_PopMenu(void)
{
    menuFrameWork_t *menu;

    Q_assert(uis.menuDepth > 0);

    menu = uis.layers[--uis.menuDepth];

    if (menu->pop) {
        menu->pop(menu);
    }

    if (!uis.menuDepth) {
        UI_ForceMenuOff();
        return;
    }

    uis.activeMenu = uis.layers[uis.menuDepth - 1];
    uis.mouseTracker = NULL;

    UI_DoHitTest();
}

/*
=================
UI_IsTransparent
=================
*/
bool UI_IsTransparent(void)
{
    if (!(Key_GetDest() & KEY_MENU)) {
        return true;
    }

    if (!uis.activeMenu) {
        return true;
    }

    return uis.activeMenu->transparent;
}

bool UI_IsLive(void)
{
    if (!(Key_GetDest() & KEY_MENU)) {
        return false;
    }

    if (!uis.activeMenu) {
        return false;
    }

    return uis.activeMenu->live;
}

bool UI_IsMenuActive(const char *name)
{
    int i;

    if (!(Key_GetDest() & KEY_MENU)) {
        return false;
    }

    if (!name || !*name) {
        return false;
    }

    for (i = 0; i < uis.menuDepth; i++) {
        if (uis.layers[i] && uis.layers[i]->name &&
            !strcmp(uis.layers[i]->name, name)) {
            return true;
        }
    }

    return false;
}

menuFrameWork_t *UI_FindMenu(const char *name)
{
    menuFrameWork_t *menu;

    LIST_FOR_EACH(menuFrameWork_t, menu, &ui_menus, entry) {
        if (!strcmp(menu->name, name)) {
            return menu;
        }
    }

    return NULL;
}

/*
=================
UI_OpenMenu
=================
*/
void UI_OpenMenu(uiMenu_t type)
{
    menuFrameWork_t *menu = NULL;

    if (!uis.initialized) {
        return;
    }

    // close any existing menus
    UI_ForceMenuOff();

    switch (type) {
    case UIMENU_DEFAULT:
        if (ui_open->integer) {
            menu = UI_FindMenu("main");
        }
        break;
    case UIMENU_MAIN:
        menu = UI_FindMenu("main");
        break;
    case UIMENU_GAME:
        menu = UI_FindMenu("game");
        if (!menu) {
            menu = UI_FindMenu("main");
        }
        break;
    case UIMENU_NONE:
        break;
    default:
        Q_assert(!"bad menu");
    }

    UI_PushMenu(menu);
}

//=============================================================================

/*
=================
UI_FormatColumns
=================
*/
void *UI_FormatColumns(int extrasize, ...)
{
    va_list argptr;
    char *buffer, *p;
    int i, j;
    size_t total = 0;
    char *strings[MAX_COLUMNS];
    size_t lengths[MAX_COLUMNS];

    va_start(argptr, extrasize);
    for (i = 0; i < MAX_COLUMNS; i++) {
        if ((p = va_arg(argptr, char *)) == NULL) {
            break;
        }
        strings[i] = p;
        total += lengths[i] = strlen(p) + 1;
    }
    va_end(argptr);

    buffer = UI_Malloc(extrasize + total + 1);
    p = buffer + extrasize;
    for (j = 0; j < i; j++) {
        memcpy(p, strings[j], lengths[j]);
        p += lengths[j];
    }
    *p = 0;

    return buffer;
}

char *UI_GetColumn(char *s, int n)
{
    int i;

    for (i = 0; i < n && *s; i++) {
        s += strlen(s) + 1;
    }

    return s;
}

/*
=================
UI_CursorInRect
=================
*/
bool UI_CursorInRect(const vrect_t *rect)
{
    if (uis.mouseCoords[0] < rect->x) {
        return false;
    }
    if (uis.mouseCoords[0] >= rect->x + rect->width) {
        return false;
    }
    if (uis.mouseCoords[1] < rect->y) {
        return false;
    }
    if (uis.mouseCoords[1] >= rect->y + rect->height) {
        return false;
    }
    return true;
}

void UI_DrawString(int x, int y, int flags, const char *string)
{
    if ((flags & UI_CENTER) == UI_CENTER) {
        x -= strlen(string) * CHAR_WIDTH / 2;
    } else if (flags & UI_RIGHT) {
        x -= strlen(string) * CHAR_WIDTH;
    }

    R_DrawString(x, y, flags, MAX_STRING_CHARS, string, uis.fontHandle);
}

void UI_DrawChar(int x, int y, int flags, int ch)
{
    R_DrawChar(x, y, flags, ch, uis.fontHandle);
}

void UI_StringDimensions(vrect_t *rc, int flags, const char *string)
{
    rc->height = CHAR_HEIGHT;
    rc->width = CHAR_WIDTH * strlen(string);

    if ((flags & UI_CENTER) == UI_CENTER) {
        rc->x -= rc->width / 2;
    } else if (flags & UI_RIGHT) {
        rc->x -= rc->width;
    }
}

void UI_DrawRect8(const vrect_t *rc, int border, int c)
{
    R_DrawFill8(rc->x, rc->y, border, rc->height, c);   // left
    R_DrawFill8(rc->x + rc->width - border, rc->y, border, rc->height, c);   // right
    R_DrawFill8(rc->x + border, rc->y, rc->width - border * 2, border, c);   // top
    R_DrawFill8(rc->x + border, rc->y + rc->height - border, rc->width - border * 2, border, c);   // bottom
}

#if 0
void UI_DrawRect32(const vrect_t *rc, int border, uint32_t color)
{
    R_DrawFill32(rc->x, rc->y, border, rc->height, color);   // left
    R_DrawFill32(rc->x + rc->width - border, rc->y, border, rc->height, color);   // right
    R_DrawFill32(rc->x + border, rc->y, rc->width - border * 2, border, color);   // top
    R_DrawFill32(rc->x + border, rc->y + rc->height - border, rc->width - border * 2, border, color);   // bottom
}
#endif

//=============================================================================
/* Menu Subsystem */

/*
=================
UI_DoHitTest
=================
*/
bool UI_DoHitTest(void)
{
    menuCommon_t *item;

    if (!uis.activeMenu) {
        return false;
    }

    Menu_UpdateShowIf(uis.activeMenu);

    if (uis.mouseTracker) {
        item = uis.mouseTracker;
    } else {
        if (!(item = Menu_HitTest(uis.activeMenu))) {
            return false;
        }
    }

    if (!UI_IsItemSelectable(item)) {
        return false;
    }

    Menu_MouseMove(item);

    if (item->flags & QMF_HASFOCUS) {
        return false;
    }

    Menu_SetFocus(item);

    return true;
}

/*
=================
UI_MouseEvent
=================
*/
void UI_MouseEvent(int x, int y)
{
    x = Q_clip(x, 0, r_config.width - 1);
    y = Q_clip(y, 0, r_config.height - 1);

    uis.mouseCoords[0] = Q_rint(x * uis.scale);
    uis.mouseCoords[1] = Q_rint(y * uis.scale);

    UI_DoHitTest();
}

static bool UI_ShouldDrawCursor(void)
{
    cvar_t *vid_noborder;

    if (!(r_config.flags & QVF_FULLSCREEN) || !uis.cursorHandle) {
        return false;
    }

    vid_noborder = Cvar_WeakGet("vid_noborder");
    if (vid_noborder && vid_noborder->integer) {
        return false;
    }

    return true;
}

static int UI_ClampColorComponent(int value)
{
    if (value < 0) {
        return 0;
    }
    if (value > 255) {
        return 255;
    }
    return value;
}

static bool UI_ParseMenuColor(const char *s, color_t *color)
{
    int r, g, b, a = 255;
    int components;

    if (!s || !*s) {
        return false;
    }

    if (*s == '#' || !strchr(s, ' ')) {
        return SCR_ParseColor(s, color);
    }

    components = sscanf(s, "%d %d %d %d", &r, &g, &b, &a);
    if (components < 3) {
        return false;
    }

    color->u8[0] = UI_ClampColorComponent(r);
    color->u8[1] = UI_ClampColorComponent(g);
    color->u8[2] = UI_ClampColorComponent(b);
    color->u8[3] = UI_ClampColorComponent(a);
    return true;
}

static void ui_menu_style_changed(cvar_t *self)
{
    const char *value;

    if (!Q_stricmp(self->string, "classic")) {
        value = "0";
    } else if (!Q_stricmp(self->string, "slate")) {
        value = "1";
    } else if (!Q_stricmp(self->string, "custom")) {
        value = "2";
    } else {
        value = va("%d", Q_clip(self->integer, 0, UI_MENU_STYLE_COUNT - 1));
    }

    if (strcmp(self->string, value)) {
        Cvar_SetByVar(self, value, FROM_CODE);
    }
}

static uiMenuStyleId_t UI_MenuStyleId(void)
{
    if (!ui_menu_style) {
        return UI_MENU_STYLE_CLASSIC;
    }

    if (!Q_stricmp(ui_menu_style->string, "classic")) {
        return UI_MENU_STYLE_CLASSIC;
    }
    if (!Q_stricmp(ui_menu_style->string, "slate")) {
        return UI_MENU_STYLE_SLATE;
    }
    if (!Q_stricmp(ui_menu_style->string, "custom")) {
        return UI_MENU_STYLE_CUSTOM;
    }

    return Q_clip(ui_menu_style->integer, 0, UI_MENU_STYLE_COUNT - 1);
}

static void UI_ApplyCustomColor(uiMenuColorId_t id, color_t *color)
{
    color_t parsed;
    cvar_t *cvar = ui_menu_custom_colors[id];

    if (!cvar) {
        return;
    }

    if (UI_ParseMenuColor(cvar->string, &parsed)) {
        *color = parsed;
    }
}

static void UI_ApplyCustomColor32(uiMenuColorId_t id, uint32_t *color)
{
    color_t parsed;
    cvar_t *cvar = ui_menu_custom_colors[id];

    if (!cvar) {
        return;
    }

    if (UI_ParseMenuColor(cvar->string, &parsed)) {
        *color = parsed.u32;
    }
}

static void UI_ApplyMenuStyle(void)
{
    uis.color = uis.baseColor;
    uis.listHeaderColor = uis.color.normal.u32;
    uis.scrollbarColor = uis.color.normal.u32;
    uis.hintBackgroundColor = MakeColor(0, 0, 255, 255);
    uis.hintTextColor = MakeColor(255, 255, 255, 255);
    uis.styleFocusFill = false;
    uis.styleMenuBackground = false;

    switch (UI_MenuStyleId()) {
    case UI_MENU_STYLE_CLASSIC:
        return;
    case UI_MENU_STYLE_SLATE:
        uis.color = ui_slateColorStyle;
        uis.listHeaderColor = ui_slateListHeaderColor;
        uis.scrollbarColor = ui_slateScrollbarColor;
        uis.hintBackgroundColor = MakeColor(18, 20, 23, 235);
        uis.hintTextColor = uis.color.normal.u32;
        break;
    case UI_MENU_STYLE_CUSTOM:
        uis.color = ui_slateColorStyle;
        uis.listHeaderColor = ui_slateListHeaderColor;
        uis.scrollbarColor = ui_slateScrollbarColor;
        uis.hintBackgroundColor = MakeColor(18, 20, 23, 235);
        uis.hintTextColor = uis.color.normal.u32;
        UI_ApplyCustomColor(UI_MENU_COLOR_BACKGROUND, &uis.color.background);
        UI_ApplyCustomColor(UI_MENU_COLOR_TITLE, &uis.color.title);
        UI_ApplyCustomColor(UI_MENU_COLOR_NORMAL, &uis.color.normal);
        UI_ApplyCustomColor(UI_MENU_COLOR_SELECTABLE, &uis.color.selectable);
        UI_ApplyCustomColor(UI_MENU_COLOR_ALTERNATE, &uis.color.alternate);
        UI_ApplyCustomColor(UI_MENU_COLOR_ACTIVE, &uis.color.active);
        UI_ApplyCustomColor(UI_MENU_COLOR_SELECTION, &uis.color.selection);
        UI_ApplyCustomColor(UI_MENU_COLOR_FOCUS, &uis.color.focus);
        UI_ApplyCustomColor(UI_MENU_COLOR_FOCUS_BORDER, &uis.color.focus_border);
        UI_ApplyCustomColor(UI_MENU_COLOR_DISABLED, &uis.color.disabled);
        UI_ApplyCustomColor32(UI_MENU_COLOR_LIST_HEADER, &uis.listHeaderColor);
        UI_ApplyCustomColor32(UI_MENU_COLOR_SCROLLBAR, &uis.scrollbarColor);
        UI_ApplyCustomColor32(UI_MENU_COLOR_HINT_BACKGROUND, &uis.hintBackgroundColor);
        UI_ApplyCustomColor32(UI_MENU_COLOR_HINT_TEXT, &uis.hintTextColor);
        break;
    default:
        return;
    }

    uis.styleFocusFill = true;
    uis.styleMenuBackground = true;
}

uint32_t UI_MenuBackgroundColor(const menuFrameWork_t *menu)
{
    return uis.styleMenuBackground ? uis.color.background.u32 : menu->color.u32;
}

uint32_t UI_MenuListHeaderColor(void)
{
    return uis.listHeaderColor;
}

uint32_t UI_MenuScrollbarColor(void)
{
    return uis.scrollbarColor;
}

uint32_t UI_MenuHintBackgroundColor(void)
{
    return uis.hintBackgroundColor;
}

uint32_t UI_MenuHintTextColor(void)
{
    return uis.hintTextColor;
}

bool UI_MenuStyleFocusFill(void)
{
    return uis.styleFocusFill;
}

static void UI_ResetMenuColors_f(void)
{
    int i;

    for (i = 0; i < UI_MENU_COLOR_COUNT; i++) {
        if (ui_menu_custom_colors[i]) {
            Cvar_SetByVar(ui_menu_custom_colors[i],
                          ui_menu_custom_colors[i]->default_string,
                          FROM_MENU);
        }
    }

    Com_Printf("Custom menu colors reset to defaults.\n");
}

/*
=================
UI_Draw
=================
*/
void UI_Draw(unsigned realtime)
{
    int i;

    uis.realtime = realtime;

    if (!(Key_GetDest() & KEY_MENU)) {
        return;
    }

    if (!uis.activeMenu) {
        return;
    }

    UI_ApplyMenuStyle();

    R_ClearColor();
    R_SetScale(uis.scale);

    if (!ui_draw_layers->integer) {
        // draw top menu
        if (uis.activeMenu->draw) {
            uis.activeMenu->draw(uis.activeMenu);
        } else {
            Menu_Draw(uis.activeMenu);
        }
    } else {
        // draw all layers
        for (i = 0; i < uis.menuDepth; i++) {
            if (uis.layers[i]->draw) {
                uis.layers[i]->draw(uis.layers[i]);
            } else {
                Menu_Draw(uis.layers[i]);
            }
        }
    }

    // draw custom cursor only in exclusive fullscreen when the OS cursor is disabled
    if (UI_ShouldDrawCursor()) {
        R_DrawPic(uis.mouseCoords[0] - uis.cursorWidth / 2,
                  uis.mouseCoords[1] - uis.cursorHeight / 2, uis.cursorHandle);
    }

    if (ui_debug->integer) {
        UI_DrawString(uis.width - 4, 4, UI_RIGHT,
                      va("%3i %3i", uis.mouseCoords[0], uis.mouseCoords[1]));
    }

    // delay playing the enter sound until after the
    // menu has been drawn, to avoid delay while
    // caching images
    if (uis.entersound) {
        uis.entersound = false;
        S_StartLocalSound("misc/menu1.wav");
    }

    R_ClearColor();
    R_SetScale(1.0f);
}

void UI_StartSound(menuSound_t sound)
{
    switch (sound) {
    case QMS_IN:
        S_StartLocalSound("misc/menu1.wav");
        break;
    case QMS_MOVE:
        S_StartLocalSound("misc/menu2.wav");
        break;
    case QMS_OUT:
        S_StartLocalSound("misc/menu3.wav");
        break;
    case QMS_BEEP:
        S_StartLocalSound("misc/talk1.wav");
        break;
    default:
        break;
    }
}

/*
=================
UI_KeyEvent
=================
*/
void UI_KeyEvent(int key, bool down)
{
    menuSound_t sound;

    if (!uis.activeMenu) {
        return;
    }

    if (!down) {
        if (key == K_MOUSE1) {
            uis.mouseTracker = NULL;
        }
        return;
    }

    sound = Menu_Keydown(uis.activeMenu, key);

    UI_StartSound(sound);
}

/*
=================
UI_CharEvent
=================
*/
void UI_CharEvent(int key)
{
    menuCommon_t *item;
    menuSound_t sound;

    if (!uis.activeMenu) {
        return;
    }

    Menu_UpdateShowIf(uis.activeMenu);

    if ((item = Menu_ItemAtCursor(uis.activeMenu)) == NULL ||
        (sound = Menu_CharEvent(item, key)) == QMS_NOTHANDLED) {
        Menu_UpdateShowIf(uis.activeMenu);
        return;
    }

    UI_StartSound(sound);
    Menu_UpdateShowIf(uis.activeMenu);
}

static void UI_Menu_g(genctx_t *ctx)
{
    menuFrameWork_t *menu;

    LIST_FOR_EACH(menuFrameWork_t, menu, &ui_menus, entry)
        Prompt_AddMatch(ctx, menu->name);
}

static void UI_PushMenu_c(genctx_t *ctx, int argnum)
{
    if (argnum == 1) {
        UI_Menu_g(ctx);
    }
}

static void UI_PushMenu_f(void)
{
    menuFrameWork_t *menu;
    char *s;

    if (Cmd_Argc() < 2) {
        Com_Printf("Usage: %s <menu>\n", Cmd_Argv(0));
        return;
    }
    s = Cmd_Argv(1);
    menu = UI_FindMenu(s);
    if (menu) {
        UI_PushMenu(menu);
    } else {
        Com_Printf("No such menu: %s\n", s);
    }
}

static void UI_PopMenu_f(void)
{
    if (uis.activeMenu) {
        UI_PopMenu();
    }
}


static const cmdreg_t c_ui[] = {
    { "forcemenuoff", UI_ForceMenuOff },
    { "pushmenu", UI_PushMenu_f, UI_PushMenu_c },
    { "popmenu", UI_PopMenu_f },
    { "ui_reset_menu_colors", UI_ResetMenuColors_f },

    { NULL, NULL }
};

static void ui_scale_changed(cvar_t *self)
{
    UI_Resize();
}

void UI_ModeChanged(void)
{
    ui_scale = Cvar_Get("ui_scale", "0", 0);
    ui_scale->changed = ui_scale_changed;
    UI_Resize();
}

static void UI_FreeMenus(void)
{
    menuFrameWork_t *menu, *next;

    LIST_FOR_EACH_SAFE(menuFrameWork_t, menu, next, &ui_menus, entry) {
        if (menu->free) {
            menu->free(menu);
        }
    }
    List_Init(&ui_menus);
}


const char *UI_GetCursorTextureName(const char *name)
{
    if (!name || !*name || Q_strcasecmp(name, "none") == 0 || strcmp(name, "0") == 0) {
        return NULL;
    }
    if (Q_strcasecmp(name, "cross") == 0 || strcmp(name, "1") == 0 || Q_strcasecmp(name, "ch1") == 0) {
        return "ch1";
    }
    if (Q_strcasecmp(name, "dot") == 0 || strcmp(name, "2") == 0 || Q_strcasecmp(name, "ch2") == 0) {
        return "ch2";
    }
    if (Q_strcasecmp(name, "angle") == 0 || strcmp(name, "3") == 0 || Q_strcasecmp(name, "ch3") == 0) {
        return "ch3";
    }
    if (strcmp(name, "4") == 0 || Q_strcasecmp(name, "ch4") == 0) {
        return "ch4";
    }
    if (strcmp(name, "5") == 0 || Q_strcasecmp(name, "ch5") == 0) {
        return "ch5";
    }
    return name;
}

void UI_SetCursor(const char *name)
{
    const char *tex_name = UI_GetCursorTextureName(name);
    if (!tex_name) {
        uis.cursorHandle = 0;
        uis.cursorWidth = 0;
        uis.cursorHeight = 0;
        return;
    }

    qhandle_t handle = R_RegisterPic(tex_name);
    if (handle) {
        uis.cursorHandle = handle;
        R_GetPicSize(&uis.cursorWidth, &uis.cursorHeight, uis.cursorHandle);
    } else {
        uis.cursorHandle = 0;
        uis.cursorWidth = 0;
        uis.cursorHeight = 0;
    }
}

static void cl_menu_cursor_changed(cvar_t *self)
{
    if (Q_strcasecmp(self->string, "none") == 0 || strcmp(self->string, "0") == 0) {
        self->integer = 0;
    } else if (Q_strcasecmp(self->string, "cross") == 0 || strcmp(self->string, "1") == 0 || Q_strcasecmp(self->string, "ch1") == 0) {
        self->integer = 1;
    } else if (Q_strcasecmp(self->string, "dot") == 0 || strcmp(self->string, "2") == 0 || Q_strcasecmp(self->string, "ch2") == 0) {
        self->integer = 2;
    } else if (Q_strcasecmp(self->string, "angle") == 0 || strcmp(self->string, "3") == 0 || Q_strcasecmp(self->string, "ch3") == 0) {
        self->integer = 3;
    } else if (strcmp(self->string, "4") == 0 || Q_strcasecmp(self->string, "ch4") == 0) {
        self->integer = 4;
    } else if (strcmp(self->string, "5") == 0 || Q_strcasecmp(self->string, "ch5") == 0) {
        self->integer = 5;
    } else {
        self->integer = -1;
    }

    UI_SetCursor(self->string);
}

/*
=================
UI_Init
=================
*/
void UI_Init(void)
{
    int i;

    Cmd_Register(c_ui);

    ui_debug = Cvar_Get("ui_debug", "0", 0);
    ui_open = Cvar_Get("ui_open", "0", 0);
    ui_draw_layers = Cvar_Get("ui_draw_layers", "0", 0);
    ui_menu_style = Cvar_Get("ui_menu_style", "0", CVAR_ARCHIVE);
    ui_menu_style->changed = ui_menu_style_changed;
    ui_menu_style_changed(ui_menu_style);
    for (i = 0; i < UI_MENU_COLOR_COUNT; i++) {
        ui_menu_custom_colors[i] =
            Cvar_Get(ui_menu_custom_color_defs[i].cvarName,
                     ui_menu_custom_color_defs[i].defaultValue,
                     CVAR_ARCHIVE);
    }
    cl_menu_cursor = Cvar_Get("cl_menu_cursor", "ch5", CVAR_ARCHIVE);
    cl_menu_cursor->changed = cl_menu_cursor_changed;

    UI_ModeChanged();

    uis.fontHandle = R_RegisterFont("conchars");
    cl_menu_cursor_changed(cl_menu_cursor);

    for (int i = 0; i < NUM_CURSOR_FRAMES; i++) {
        uis.bitmapCursors[i] = R_RegisterPic(va("m_cursor%d", i));
    }

    uis.color.background.u32    = MakeColor(0,   0,   0, 255);
    uis.color.title.u32         = MakeColor(15, 128, 235, 255);
    uis.color.normal.u32        = MakeColor(15, 128, 235, 255);
    uis.color.selectable.u32    = MakeColor(15, 128, 235, 255);
    uis.color.alternate.u32     = MakeColor(255, 255, 255, 255);
    uis.color.active.u32        = MakeColor(15, 128, 235, 255);
    uis.color.selection.u32     = MakeColor(15, 128, 235, 255);
    uis.color.focus.u32         = MakeColor(80, 80, 80, 110);
    uis.color.focus_border.u32  = MakeColor(180, 180, 180, 160);
    uis.color.disabled.u32      = MakeColor(127, 127, 127, 255);
    uis.listHeaderColor         = uis.color.normal.u32;
    uis.scrollbarColor          = uis.color.normal.u32;
    uis.hintBackgroundColor     = MakeColor(0, 0, 255, 255);
    uis.hintTextColor           = MakeColor(255, 255, 255, 255);
    uis.styleFocusFill          = false;
    uis.styleMenuBackground     = false;

    strcpy(uis.weaponModel, "w_railgun.md2");

    // load custom menus
    UI_LoadScript();
    uis.baseColor = uis.color;

    // load built-in menus
    M_Menu_PlayerConfig();
    M_Menu_Servers();
    M_Menu_Demos();

    Com_DPrintf("Registered %d menus.\n", List_Count(&ui_menus));

    uis.initialized = true;
}

/*
=================
UI_Shutdown
=================
*/
void UI_Shutdown(void)
{
    if (!uis.initialized) {
        return;
    }
    UI_ForceMenuOff();

    ui_scale->changed = NULL;

    PlayerModel_Free();

    UI_FreeMenus();
    Menu_FreeColorPicker();

    Cmd_Deregister(c_ui);

    memset(&uis, 0, sizeof(uis));

    Z_LeakTest(TAG_UI);
}
