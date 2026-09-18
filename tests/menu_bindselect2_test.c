/* Exercise the production binding/dropdown lifecycle without a client or renderer. */
#undef NDEBUG
#include "../src/client/ui/menu.c"
#include "client/fps.h"
#include <assert.h>

uiStatic_t uis;
refcfg_t r_config;
cmdbuf_t cmd_buffer;
int menu_drawing_item_index;
static cvar_t unused_cvar;
cvar_t *ui_debug = &unused_cvar;
cvar_t *cl_drawStrafeHelper = &unused_cvar;
cvar_t *ui_menu_focus_width = &unused_cvar, *ui_menu_focus_padding_x = &unused_cvar;
cvar_t *ui_menu_focus_padding_y = &unused_cvar, *ui_menu_bar_image = &unused_cvar;
cvar_t *ui_menu_bar_image_alpha = &unused_cvar, *ui_menu_bar_image_mode = &unused_cvar;
cvar_t *ui_menu_anim = &unused_cvar, *ui_menu_anim_focus_ms = &unused_cvar;
cvar_t *ui_menu_title_top_padding = &unused_cvar, *ui_menu_title_item_gap = &unused_cvar;

static char *presets[] = { "20", "22", "25", "26", "30", "31", "32", "33", "60", "66", "90", "100", "120" };
static menuFrameWork_t menu;
static menuBindSelect2_t rows[NUM_FPS_SLOTS];
static void *menu_items[NUM_FPS_SLOTS];
static vrect_t item_rects[NUM_FPS_SLOTS][q_countof(presets)];
static struct {
    cvar_t var;
    char name[32], text[512], original[512];
    int writes;
} slots[NUM_FPS_SLOTS][2];
static char row_names[NUM_FPS_SLOTS][4], commands[NUM_FPS_SLOTS][32];
static char bindings[256][128];
static int writes, binding_writes, pops;
static keywaitcb_t waiting;
static void *waiting_arg;
static struct { int x, y; char text[128]; } drawn[128];
static int draw_count;

static void Unexpected(void) { assert(!"unexpected boundary call"); abort(); }
void Com_Error(error_type_t type, const char *format, ...) { Unexpected(); }
void Com_LPrintf(print_type_t type, const char *format, ...) { Unexpected(); }
void Cbuf_AddText(cmdbuf_t *buf, const char *text) { Unexpected(); }
size_t Com_EscapeString(char *dst, const char *src, size_t size) { Unexpected(); return 0; }

static void SetSlot(int slot, int side, const char *text)
{
    assert(strlen(text) < sizeof(slots[slot][side].text));
    strcpy(slots[slot][side].text, text);
    slots[slot][side].var.integer = Q_atoi(text);
    slots[slot][side].var.value = Q_atof(text);
}

void Cvar_SetByVar(cvar_t *var, const char *text, from_t from)
{
    assert(from == FROM_MENU);
    for (int slot = 0; slot < NUM_FPS_SLOTS; slot++)
        for (int side = 0; side < 2; side++)
            if (var == &slots[slot][side].var) {
                SetSlot(slot, side, text);
                slots[slot][side].writes++;
                writes++;
                return;
            }
    Unexpected();
}
cvar_t *Cvar_FindVar(const char *name) { Unexpected(); return NULL; }
cvar_t *Cvar_Get(const char *name, const char *value, int flags) { Unexpected(); return NULL; }
cvar_t *Cvar_WeakGet(const char *name) { Unexpected(); return NULL; }
void Cvar_SetValue(cvar_t *var, float value, from_t from) { Unexpected(); }
void Cvar_SetInteger(cvar_t *var, int value, from_t from) { Unexpected(); }
int Cvar_ClampInteger(cvar_t *var, int low, int high) { Unexpected(); return 0; }

int Key_EnumBindings(int key, const char *command)
{
    for (; key < q_countof(bindings); key++)
        if (bindings[key][0] && !Q_stricmp(bindings[key], command))
            return key;
    return -1;
}
void Key_SetBinding(int key, const char *command)
{
    assert(key >= 0 && key < q_countof(bindings));
    Q_strlcpy(bindings[key], command ? command : "", sizeof(bindings[key]));
    binding_writes++;
}
const char *Key_KeynumToLabel(int key) { return va("key%d", key); }
int Key_IsDown(int key) { return 0; }
void Key_WaitKey(keywaitcb_t callback, void *arg) { waiting = callback; waiting_arg = arg; }

void UI_PushMenu(menuFrameWork_t *m) { Unexpected(); }
void UI_PopMenu(void) { Menu_Pop(&menu); pops++; }
void UI_StartSound(menuSound_t sound) { }
int UI_MenuSpacing(void) { return CHAR_HEIGHT + 2; }
bool UI_CursorInRect(const vrect_t *r)
{
    return uis.mouseCoords[0] >= r->x && uis.mouseCoords[0] < r->x + r->width &&
           uis.mouseCoords[1] >= r->y && uis.mouseCoords[1] < r->y + r->height;
}
void UI_DrawString(int x, int y, int flags, const char *text)
{
    assert(draw_count < q_countof(drawn) && strlen(text) < sizeof(drawn[0].text));
    drawn[draw_count].x = x;
    drawn[draw_count].y = y;
    strcpy(drawn[draw_count++].text, text);
}
void UI_DrawChar(int x, int y, int flags, int ch) { Unexpected(); }
void UI_DrawRect8(const vrect_t *rect, int border, int color) { Unexpected(); }
void UI_StringDimensions(vrect_t *rect, int flags, const char *text) { Unexpected(); }
void UI_DrawMenuBackgroundModel(const menuFrameWork_t *m) { Unexpected(); }
void UI_ModelPreview_MenuItemFocused(const menuFrameWork_t *m, const menuCommon_t *item) { }
void UI_SetColor_Wrapper(uint32_t color) { }
void UI_SetAltColor_Wrapper(uint32_t color) { }
void UI_ClearColor_Wrapper(void) { }
void UI_DrawFill32_Wrapper(int x, int y, int w, int h, uint32_t color) { }
float Menu_Ease01(float value) { return value; }
bool UI_MenuStyleFocusFill(void) { return false; }
uiMenuStyleId_t UI_MenuStyleId(void) { return UI_MENU_STYLE_CUSTOM; }
uint32_t UI_MenuBackgroundColor(const menuFrameWork_t *m) { return 0; }
#define COLOR_STUB(name) uint32_t name(void) { return 0; }
COLOR_STUB(UI_MenuListHeaderColor)
COLOR_STUB(UI_MenuScrollbarColor)
COLOR_STUB(UI_MenuHintBackgroundColor)
COLOR_STUB(UI_MenuHintTextColor)
COLOR_STUB(UI_MenuFocusMarkerColor)
COLOR_STUB(UI_MenuValueColor)
COLOR_STUB(UI_MenuValueActiveColor)
COLOR_STUB(UI_MenuValueChangedColor)
COLOR_STUB(UI_MenuSliderTrackColor)
COLOR_STUB(UI_MenuSliderFillColor)
COLOR_STUB(UI_MenuSliderThumbColor)
COLOR_STUB(UI_MenuSliderBorderColor)
COLOR_STUB(UI_MenuSortedHeaderColor)
COLOR_STUB(UI_MenuPanelBorderColor)
COLOR_STUB(UI_MenuPanelShadowColor)
#undef COLOR_STUB

/* Unrelated field/render/savegame paths must not be used by this fixture. */
bool IF_KeyEvent(inputField_t *f, int key) { Unexpected(); return false; }
bool IF_CharEvent(inputField_t *f, int key) { Unexpected(); return false; }
void IF_Init(inputField_t *f, size_t visible, size_t limit) { Unexpected(); }
void IF_Replace(inputField_t *f, const char *text) { Unexpected(); }
int IF_Draw(const inputField_t *f, int x, int y, int flags, qhandle_t font) { Unexpected(); return 0; }
void Z_Free(void *ptr) { free(ptr); }
void *Z_Realloc(void *ptr, size_t size) { return realloc(ptr, size); }
void *Z_Malloc(size_t size) { return malloc(size); }
void *Z_TagMalloc(size_t size, memtag_t tag) { return malloc(size); }
char *Z_TagCopyString(const char *text, memtag_t tag) { Unexpected(); return NULL; }
bool SCR_ParseColor(const char *text, color_t *color) { Unexpected(); return false; }
qhandle_t R_RegisterImage(const char *name, imagetype_t type, imageflags_t flags) { Unexpected(); return 0; }
void (R_SetColor)(uint32_t color) { Unexpected(); }
void R_SetClipRect(const clipRect_t *clip) { Unexpected(); }
int R_DrawString(int x, int y, int flags, size_t length, const char *text, qhandle_t font) { Unexpected(); return 0; }
bool R_GetPicSize(int *w, int *h, qhandle_t pic) { Unexpected(); return false; }
void R_DrawPic(int x, int y, qhandle_t pic) { Unexpected(); }
void R_DrawStretchPic(int x, int y, int w, int h, qhandle_t pic) { Unexpected(); }
void R_DrawKeepAspectPic(int x, int y, int w, int h, qhandle_t pic) { Unexpected(); }
void R_DrawFill8(int x, int y, int w, int h, int color) { Unexpected(); }
#if USE_SAVEGAMES
char *SV_GetSaveInfo(const char *dir) { Unexpected(); return NULL; }
#endif
bool SH_GetPresetColors(const char *name, const char **a, const char **o, const char **c) { Unexpected(); return false; }

static void Setup(void)
{
    memset(&menu, 0, sizeof(menu));
    memset(rows, 0, sizeof(rows));
    memset(slots, 0, sizeof(slots));
    memset(bindings, 0, sizeof(bindings));
    memset(&uis, 0, sizeof(uis));
    g_openSelect = NULL;
    writes = binding_writes = pops = draw_count = 0;
    waiting = NULL;
    waiting_arg = NULL;
    menu.name = "jumpholdfps";
    menu.items = menu_items;
    menu.nitems = NUM_FPS_SLOTS;
    menu.live = true;
    menu.y1 = 0;
    menu.y2 = 720;
    uis.width = 1280;
    uis.height = 720;
    uis.scale = 1;
    uis.activeMenu = &menu;
    for (int slot = 0; slot < NUM_FPS_SLOTS; slot++) {
        menuBindSelect2_t *row = &rows[slot];
        Q_snprintf(row_names[slot], sizeof(row_names[slot]), "%d", slot + 1);
        Q_snprintf(commands[slot], sizeof(commands[slot]), "+fps_hold %d", slot + 1);
        row->generic = (menuCommon_t) { .type = MTYPE_BINDSELECT2, .name = row_names[slot],
            .parent = &menu, .x = 640, .y = 40 + slot * 24, .flags = slot ? 0 : QMF_HASFOCUS };
        row->cmd = commands[slot];
        row->itemRects = item_rects[slot];
        menu_items[slot] = row;
        for (int side = 0; side < 2; side++) {
            Q_snprintf(slots[slot][side].name, sizeof(slots[slot][side].name),
                       "fps_%s_%d", side ? "release" : "hold", slot + 1);
            slots[slot][side].var = (cvar_t) { .name = slots[slot][side].name,
                .string = slots[slot][side].text, .default_string = side ? "144" : "45", .flags = CVAR_ARCHIVE };
            SetSlot(slot, side, side ? "144" : "45");
            row->cvar[side] = &slots[slot][side].var;
            row->itemnames[side] = presets;
            row->numItems[side] = q_countof(presets);
        }
    }
}

static void Snapshot(void)
{
    for (int slot = 0; slot < NUM_FPS_SLOTS; slot++)
        for (int side = 0; side < 2; side++)
            strcpy(slots[slot][side].original, slots[slot][side].text);
}

static void AssertUnchangedExcept(int edited_slot, int edited_side)
{
    for (int slot = 0; slot < NUM_FPS_SLOTS; slot++)
        for (int side = 0; side < 2; side++) {
            if (slot == edited_slot && side == edited_side)
                continue;
            assert(!strcmp(slots[slot][side].text, slots[slot][side].original));
            assert(slots[slot][side].writes == 0);
        }
}

static void Open(void)
{
    assert(Menu_Push(&menu));
    for (int slot = 0; slot < NUM_FPS_SLOTS; slot++)
        BindSelect2_Init(&rows[slot]);
}

static void TestVisit(void)
{
    static const char *const values[] = {
        "45", "144", "030", "30.9", "", "broken", "2147483647", "4294967297",
        "20", "120", "0", "000000000000000000000000000030"
    };
    Setup();
    for (int slot = 0; slot < NUM_FPS_SLOTS; slot++)
        for (int side = 0; side < 2; side++)
            SetSlot(slot, side, values[(slot + side) % q_countof(values)]);
    Snapshot();
    for (int visit = 0; visit < 2; visit++) {
        Open();
        Menu_Pop(&menu);
        assert(writes == 0);
        AssertUnchangedExcept(-1, -1);
    }
    Open();
    for (int slot = 0; slot < NUM_FPS_SLOTS; slot++)
        for (int side = 0; side < 2; side++) {
            int expected = -1;
            for (int i = 0; i < q_countof(presets); i++)
                if (!strcmp(presets[i], slots[slot][side].text))
                    expected = i;
            assert(rows[slot].curvalue[side] == expected);
        }
    Menu_Keydown(&menu, K_ESCAPE);
    assert(pops == 1 && writes == 0);
}

static void TestKeyboard(void)
{
    for (int slot = 0; slot < NUM_FPS_SLOTS; slot++)
        for (int side = 0; side < 2; side++)
            for (int direction = 0; direction < 2; direction++) {
                Setup(); Snapshot(); Open();
                Menu_SetFocus(&rows[slot].generic);
                rows[slot].focusPart = side + 1;
                Menu_Keydown(&menu, direction ? K_LEFTARROW : K_RIGHTARROW);
                assert(writes == 1 && slots[slot][side].writes == 1);
                assert(!strcmp(slots[slot][side].text, direction ? "120" : "20"));
                AssertUnchangedExcept(slot, side);
                Menu_Pop(&menu);
                assert(writes == 1);
            }
    for (int side = 0; side < 2; side++)
        for (int direction = 0; direction < 2; direction++) {
            Setup(); Snapshot(); Open();
            rows[0].focusPart = side + 1;
            Menu_Keydown(&menu, K_ENTER);
            assert(rows[0].open && rows[0].hoverIndex == -1);
            Menu_Keydown(&menu, direction ? K_UPARROW : K_DOWNARROW);
            assert(writes == 0);
            Menu_Keydown(&menu, K_ENTER);
            assert(!rows[0].open && g_openSelect == NULL && writes == 1);
            assert(!strcmp(slots[0][side].text, direction ? "120" : "20"));
            AssertUnchangedExcept(0, side);
        }
}

static void TestCancellation(void)
{
    for (int side = 0; side < 2; side++) {
        Setup(); Snapshot(); Open();
        rows[0].focusPart = side + 1;
        Menu_Keydown(&menu, K_ENTER);
        Menu_Keydown(&menu, K_ENTER);
        assert(!rows[0].open && writes == 0);
        Menu_Keydown(&menu, K_ENTER);
        Menu_Keydown(&menu, K_DOWNARROW);
        Menu_Keydown(&menu, K_ESCAPE);
        assert(!rows[0].open && writes == 0);
        Menu_Keydown(&menu, K_ENTER);
        Menu_Keydown(&menu, side ? K_LEFTARROW : K_RIGHTARROW);
        Menu_Keydown(&menu, K_ENTER);
        assert(!rows[0].open && writes == 0);
        Menu_Keydown(&menu, K_ENTER);
        uis.mouseCoords[0] = uis.mouseCoords[1] = -100;
        Menu_Keydown(&menu, K_MOUSE1);
        assert(!rows[0].open && writes == 0);
        Menu_Keydown(&menu, K_ENTER);
        Menu_Pop(&menu);
        assert(!rows[0].open && g_openSelect == NULL && writes == 0);
        AssertUnchangedExcept(-1, -1);
    }
}

static void TestMouse(void)
{
    for (int side = 0; side < 2; side++)
        for (int stale = 0; stale < 2; stale++) {
            Setup(); Snapshot(); Open();
            rows[0].focusPart = side + 1;
            Menu_Keydown(&menu, K_ENTER);
            BindSelect2_DrawDropDown(&rows[0]);
            int target = 8;
            vrect_t rect = rows[0].itemRects[target];
            uis.mouseCoords[0] = rect.x + rect.width / 2;
            uis.mouseCoords[1] = rect.y + CHAR_HEIGHT / 2;
            rows[0].hoverIndex = stale ? 0 : -1;
            Menu_Keydown(&menu, K_MOUSE1);
            assert(writes == 1 && !strcmp(slots[0][side].text, "60"));
            assert(!rows[0].open && g_openSelect == NULL);
            AssertUnchangedExcept(0, side);
            Menu_Pop(&menu);
            assert(writes == 1);
        }
}

static void TestBindings(void)
{
    Setup(); Snapshot();
    rows[0].legacycmd = "+fps_hold 1 old menu arguments";
    strcpy(bindings['a'], rows[0].legacycmd);
    strcpy(bindings['b'], rows[0].legacycmd);
    Open();
    assert(binding_writes == 2 && !strcmp(bindings['a'], rows[0].cmd) && !strcmp(bindings['b'], rows[0].cmd));
    rows[0].focusPart = 0;
    Menu_Keydown(&menu, K_ENTER);
    assert(waiting && waiting_arg == &rows[0] && menu.keywait);
    waiting(waiting_arg, 'c');
    assert(!waiting && !menu.keywait && !strcmp(bindings['c'], rows[0].cmd));
    assert(!bindings['a'][0] && !bindings['b'][0]);
    rows[0].focusPart = 0;
    Menu_Keydown(&menu, K_DEL);
    assert(!bindings['c'][0]);
    Menu_Keydown(&menu, K_ENTER);
    assert(waiting);
    waiting(waiting_arg, K_ESCAPE);
    Menu_Pop(&menu);
    assert(writes == 0);
    AssertUnchangedExcept(-1, -1);
}

static void TestExternalReset(void)
{
    Setup();
    SetSlot(0, 0, "30"); SetSlot(0, 1, "120");
    Open();
    /* A reset restores startup-seeded defaults; another change may arrive from a config. */
    SetSlot(0, 0, slots[0][0].var.default_string);
    SetSlot(0, 1, "1000");
    Snapshot();
    Menu_Pop(&menu);
    assert(writes == 0);
    AssertUnchangedExcept(-1, -1);
    Open();
    assert(rows[0].curvalue[0] == -1 && rows[0].curvalue[1] == -1);
    Menu_Pop(&menu);
    assert(writes == 0);
}

static bool Drew(const char *text)
{
    for (int i = 0; i < draw_count; i++)
        if (!strcmp(drawn[i].text, text))
            return true;
    return false;
}

static void TestDisplay(void)
{
    Setup();
    SetSlot(0, 0, "2147483647"); SetSlot(0, 1, "30.9");
    Snapshot(); Open();
    BindSelect2_Draw(&rows[0]);
    assert(Drew("2147483647 \x1f") && Drew("30.9 \x1f"));
    assert(rows[0].colWidth[1] >= 12 * CHAR_WIDTH);
    char long_value[400];
    memset(long_value, '9', sizeof(long_value) - 1); long_value[sizeof(long_value) - 1] = 0;
    SetSlot(0, 0, long_value); SetSlot(0, 1, "bad\nvalue\t!");
    Snapshot(); BindSelect2_Update(&menu); draw_count = 0;
    BindSelect2_Draw(&rows[0]);
    assert(Drew("9999999999999... \x1f") && Drew("bad value ! \x1f"));
    assert(rows[0].colWidth[1] == 18 * CHAR_WIDTH);
    assert(rows[0].generic.rect.width < uis.width);
    for (int slot = 1; slot < NUM_FPS_SLOTS; slot++) {
        assert(BindSelect2_TableLeft(&rows[slot]) == BindSelect2_TableLeft(&rows[0]));
        for (int side = 0; side < 2; side++)
            assert(BindSelect2_SelectX(&rows[slot], side) == BindSelect2_SelectX(&rows[0], side));
    }
    Menu_Pop(&menu);
    assert(writes == 0);
    AssertUnchangedExcept(-1, -1);
}

int main(void)
{
    TestVisit();
    TestKeyboard();
    TestCancellation();
    TestMouse();
    TestBindings();
    TestExternalReset();
    TestDisplay();
    puts("Bindselect2 preserves custom slots and commits only explicit keyboard/mouse selections");
    return 0;
}
