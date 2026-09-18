/* Exercise production keyboard routing without starting a client or server. */
#undef NDEBUG
#include "../src/client/keys.c"
#include <assert.h>

client_state_t cl;
client_static_t cls;
cmdbuf_t cmd_buffer;
const vid_driver_t *vid;
unsigned com_eventTime;
static cvar_t game_var;
cvar_t *fs_game = &game_var;
static cvar_t developer_var;
cvar_t *developer = &developer_var;

static char bindings_sent[4096], server_sent[1024];
static int fullscreen_toggles, console_toggles, console_keys, message_keys;
static int ui_down[256], ui_up[256], menu_opens;
static uiMenu_t last_menu;

void Com_LPrintf(print_type_t type, const char *format, ...) { }
void Com_Error(error_type_t type, const char *format, ...)
{
    va_list args;
    va_start(args, format);
    vfprintf(stderr, format, args);
    va_end(args);
    abort();
}

void Z_Free(void *ptr) { free(ptr); }
char *Z_TagCopyString(const char *text, memtag_t tag)
{
    if (!text)
        return NULL;
    char *copy = malloc(strlen(text) + 1);
    assert(copy);
    return strcpy(copy, text);
}

void Cmd_Register(const cmdreg_t *reg) { }
int Cmd_Argc(void) { return 0; }
char *Cmd_Argv(int n) { return ""; }
char *Cmd_ArgsFrom(int n) { return ""; }
void Com_Generic_c(genctx_t *ctx, int argnum) { }
void Prompt_AddMatch(genctx_t *ctx, const char *text) { }
int FS_FPrintf(qhandle_t file, const char *format, ...) { abort(); }

static void Append(char *buffer, size_t size, const char *text)
{
    assert(strlen(buffer) + strlen(text) < size);
    strcat(buffer, text);
}

void Cbuf_AddText(cmdbuf_t *buffer, const char *text)
{
    assert(buffer == &cmd_buffer);
    Append(bindings_sent, sizeof(bindings_sent), text);
}

void CL_ClientCommand(const char *text)
{
    Append(server_sent, sizeof(server_sent), text);
    Append(server_sent, sizeof(server_sent), "\n");
}

void IN_Activate(void) { }
void CL_CheckForPause(void) { }
void VID_ToggleFullscreen(void) { fullscreen_toggles++; }
void Con_ToggleConsole_f(void) { console_toggles++; }
void Con_Close(bool force) { }
void Key_Console(int key) { console_keys++; }
void Key_Message(int key) { message_keys++; }
void Char_Console(int key) { }
void Char_Message(int key) { }
void UI_CharEvent(int key) { }
void UI_KeyEvent(int key, bool down)
{
    if (down)
        ui_down[key]++;
    else
        ui_up[key]++;
}
void UI_OpenMenu(uiMenu_t menu) { menu_opens++; last_menu = menu; }
bool UI_IsMenuActive(const char *name) { return false; }
void SCR_FinishCinematic(void) { }

static const struct {
    unsigned key;
    const char *command;
} shortcuts[] = {
    { 'm', "inven\n" },
    { K_UPARROW, "invprev\n" },
    { K_DOWNARROW, "invnext\n" },
    { K_ENTER, "invuse\n" },
};

static void Reset(void)
{
    Key_ClearStates();
    Key_Unbindall_f();
    memset(&cl, 0, sizeof(cl));
    memset(&cls, 0, sizeof(cls));
    cls.state = ca_active;
    cls.key_dest = KEY_GAME;
    game_var.string = "jump";
    fs_game = &game_var;
    bindings_sent[0] = server_sent[0] = 0;
    fullscreen_toggles = console_toggles = console_keys = message_keys = 0;
    menu_opens = 0;
    memset(ui_down, 0, sizeof(ui_down));
    memset(ui_up, 0, sizeof(ui_up));
    com_eventTime = 0;
}

static void Event(unsigned key, bool down)
{
    Key_Event(key, down, ++com_eventTime);
}

static void ExpectBindingPair(unsigned key, unsigned pressed, unsigned released)
{
    char expected[128];
    snprintf(expected, sizeof(expected), "+attack %u %u\n-attack %u %u\n",
             key, pressed, key, released);
    assert(!strcmp(bindings_sent, expected));
}

static void TestMappings(void)
{
    for (size_t i = 0; i < q_countof(shortcuts); i++) {
        Reset();
        unsigned key = shortcuts[i].key;
        Key_SetBinding(key, "+attack");
        Event(K_CTRL, true);
        Event(key, true);
        Event(key, true);
        Event(key, true);
        assert(!strcmp(server_sent, shortcuts[i].command));
        assert(!bindings_sent[0]);
        Event(key, false);
        Event(K_CTRL, false);
        assert(!bindings_sent[0]);
        assert(!strcmp(Key_BindingForKey(key), "+attack"));
        assert(!Key_AnyKeyDown());
    }

    Reset();
    game_var.string = "JuMp";
    Event(K_CTRL, true);
    Event('m', true);
    assert(!strcmp(server_sent, "inven\n"));
}

static void TestGuards(void)
{
    for (size_t i = 0; i < q_countof(shortcuts); i++) {
        unsigned key = shortcuts[i].key;
        for (int guard = 0; guard < 8; guard++) {
            Reset();
            Key_SetBinding(key, "+attack");
            switch (guard) {
            case 0: break; /* no Ctrl */
            case 1: cls.state = ca_disconnected; break;
            case 2: cls.state = ca_connected; break;
            case 3: cls.state = ca_cinematic; break;
            case 4: cls.demo.playback = true; break;
            case 5: fs_game = NULL; break;
            case 6: game_var.string = "baseq2"; break;
            case 7: Event(K_SHIFT, true); break;
            }
            if (guard)
                Event(K_CTRL, true);
            unsigned pressed = com_eventTime + 1;
            Event(key, true);
            Event(key, false);
            assert(!server_sent[0]);
            ExpectBindingPair(key, pressed, pressed + 1);
        }

        static const keydest_t destinations[] = {
            KEY_CONSOLE, KEY_MENU, KEY_MESSAGE, KEY_CONSOLE | KEY_MENU
        };
        for (size_t d = 0; d < q_countof(destinations); d++) {
            Reset();
            Key_SetBinding(key, "+attack");
            cls.key_dest = destinations[d];
            Event(K_CTRL, true);
            Event(key, true);
            Event(key, false);
            assert(!server_sent[0] && !bindings_sent[0]);
            if (cls.key_dest & KEY_CONSOLE)
                assert(console_keys > 0);
            else if (cls.key_dest & KEY_MENU)
                assert(ui_down[key] == 1 && ui_up[key] == 1);
            else
                assert(message_keys > 0);
        }

        Reset();
        Key_SetBinding(key, "+attack");
        Event(K_CTRL, true);
        Event(K_ALT, true);
        Event(key, true);
        Event(key, false);
        assert(!server_sent[0]);
        if (key == K_ENTER) {
            assert(fullscreen_toggles == 1 && !bindings_sent[0]);
        } else {
            ExpectBindingPair(key, 3, 4);
        }
    }
}

static void TestConsumedRelease(void)
{
    for (size_t i = 0; i < q_countof(shortcuts); i++) {
        for (int transition = 0; transition < 4; transition++) {
            Reset();
            unsigned key = shortcuts[i].key;
            Key_SetBinding(key, "+attack");
            Event(K_CTRL, true);
            Event(key, true);
            Event(K_CTRL, false);
            if (transition == 1)
                Key_SetDest(KEY_MENU);
            else if (transition == 2)
                Key_SetDest(KEY_CONSOLE);
            else if (transition == 3) {
                cls.state = ca_disconnected;
                game_var.string = "baseq2";
            }
            Event(key, true);
            Event(key, false);
            assert(!strcmp(server_sent, shortcuts[i].command));
            assert(!bindings_sent[0]);
            assert(!ui_down[key] && !ui_up[key]);
            assert(!console_keys);
            assert(!Key_IsDown(key));
        }
    }

    /* Changing modifiers during a consumed Enter must not trigger Alt+Enter. */
    Reset();
    Event(K_CTRL, true);
    Event(K_ENTER, true);
    Event(K_CTRL, false);
    Event(K_ALT, true);
    Event(K_ENTER, true);
    Event(K_ENTER, false);
    assert(!fullscreen_toggles && !strcmp(server_sent, "invuse\n"));
}

static void TestEarlierBinding(void)
{
    for (size_t i = 0; i < q_countof(shortcuts); i++) {
        Reset();
        unsigned key = shortcuts[i].key;
        Key_SetBinding(key, "+attack");
        Event(key, true);
        Event(K_CTRL, true);
        Event(key, true);
        Event(key, false);
        Event(K_CTRL, false);
        assert(!server_sent[0]);
        ExpectBindingPair(key, 1, 4);
        assert(!Key_AnyKeyDown());
    }
}

static void TestFocusCleanup(void)
{
    for (size_t i = 0; i < q_countof(shortcuts); i++) {
        Reset();
        unsigned key = shortcuts[i].key;
        Key_SetBinding(key, "+attack");
        Event(K_CTRL, true);
        Event(key, true);
        Key_SetDest(KEY_MENU);
        Key_ClearStates();
        assert(!Key_IsDown(key) && !Key_IsDown(K_CTRL));
        assert(!Key_AnyKeyDown() && !bindings_sent[0]);
        assert(!ui_down[key] && !ui_up[key]);
        assert(!strcmp(server_sent, shortcuts[i].command));

        /* Focus recovery must not leave the base key permanently consumed. */
        Key_SetDest(KEY_GAME);
        unsigned pressed = com_eventTime + 1;
        Event(key, true);
        Event(key, false);
        ExpectBindingPair(key, pressed, pressed + 1);
    }
}

static void TestExistingShortcuts(void)
{
    Reset();
    Event(K_ALT, true);
    Event(K_ENTER, true);
    Event(K_ENTER, true);
    Event(K_ENTER, false);
    assert(fullscreen_toggles == 1 && !server_sent[0]);

    Reset();
    Event(K_CTRL, true);
    Event(K_ESCAPE, true);
    assert(menu_opens == 1 && last_menu == UIMENU_GAME);
    assert(!server_sent[0]);

    Reset();
    cl.frame.ps.stats[STAT_LAYOUTS] = LAYOUTS_LAYOUT;
    Event(K_CTRL, true);
    Event(K_ESCAPE, true);
    assert(!strcmp(server_sent, "putaway\n") && !menu_opens);
    Event(K_ESCAPE, true);
    assert(menu_opens == 1 && last_menu == UIMENU_GAME);

    Reset();
    Event(K_CTRL, true);
    Event('`', true);
    Event('`', true);
    assert(console_toggles == 1 && !server_sent[0]);
}

static void TestPhysicalCtrl(void)
{
    static const unsigned controls[] = { K_LCTRL, K_RCTRL };
    for (size_t c = 0; c < q_countof(controls); c++) {
        for (size_t i = 0; i < q_countof(shortcuts); i++) {
            Reset();
            unsigned key = shortcuts[i].key;
            Key_SetBinding(key, "+attack");
            Key_Event2(controls[c], true, ++com_eventTime);
            assert(Key_IsDown(K_CTRL) && Key_IsDown(controls[c]));
            Key_Event2(key, true, ++com_eventTime);
            Key_Event2(controls[c], false, ++com_eventTime);
            Key_Event2(key, true, ++com_eventTime);
            Key_Event2(key, false, ++com_eventTime);
            assert(!strcmp(server_sent, shortcuts[i].command));
            assert(!bindings_sent[0]);
            assert(!Key_AnyKeyDown());
        }
    }
}

static void TestOverlappingModifiers(void)
{
    static const unsigned modifiers[][2] = {
        { K_LCTRL, K_RCTRL },
        { K_LSHIFT, K_RSHIFT },
        { K_LALT, K_RALT },
    };
    for (size_t m = 0; m < q_countof(modifiers); m++) {
        for (size_t released = 0; released < 2; released++) {
            Reset();
            Key_SetBinding('m', "+attack");
            if (m)
                Event(K_CTRL, true);
            Key_Event2(modifiers[m][0], true, ++com_eventTime);
            Key_Event2(modifiers[m][1], true, ++com_eventTime);
            Key_Event2(modifiers[m][released], false, ++com_eventTime);
            unsigned pressed = com_eventTime + 1;
            Event('m', true);
            Event('m', false);
            if (m) {
                assert(!server_sent[0]);
                ExpectBindingPair('m', pressed, pressed + 1);
            } else {
                assert(!strcmp(server_sent, "inven\n"));
                assert(!bindings_sent[0]);
            }
        }
    }
}
int main(void)
{
    Key_Init();
    TestMappings();
    TestGuards();
    TestConsumedRelease();
    TestEarlierBinding();
    TestFocusCleanup();
    TestExistingShortcuts();
    TestPhysicalCtrl();
    TestOverlappingModifiers();
    Reset();
    puts("jump-menu-keys: mappings, guards, releases, focus and existing shortcuts passed");
    return 0;
}
