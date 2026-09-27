/* Exercise first-launch defaults and config replay without a client or server. */
#undef NDEBUG
#include "../src/client/keys.c"
#include <assert.h>

client_state_t cl;
client_static_t cls;
const vid_driver_t *vid;
unsigned com_eventTime, com_framenum, com_localTime2;
bool com_initialized = true;
cvar_t *fs_game, *dedicated, *developer, *sv_running;

static struct {
    const char *name;
    unsigned flags;
    int result;
} files[8];
static int file_count, file_queries;
static char saved[16384];

/* The relevant stock PAK defaults that caused the first-launch regression. */
static const char stock_defaults[] =
    "unbindall\n"
    "bind UPARROW +forward\n"
    "bind MOUSE3 +forward\n"
    "bind DOWNARROW +back\n"
    "bind , +moveleft\n"
    "bind . +moveright\n"
    "bind a +lookup\n"
    "bind s \"use silencer\"\n"
    "bind CTRL +attack\n"
    "bind MOUSE2 +strafe\n"
    "bind c +movedown\n"
    "bind SPACE +moveup\n"
    "bind TAB inven\n"
    "bind x score\n"
    "bind F1 \"cmd help\"\n"
    "set cl_run 0\n";

void Com_LPrintf(print_type_t type, const char *format, ...) { }
void Com_Error(error_type_t type, const char *format, ...)
{
    va_list args;
    va_start(args, format);
    vfprintf(stderr, format, args);
    va_end(args);
    abort();
}

void *Z_TagMalloc(size_t size, memtag_t tag)
{
    void *p = malloc(size);
    assert(p);
    return p;
}
void *Z_Malloc(size_t size) { return Z_TagMalloc(size, TAG_GENERAL); }
void Z_Free(void *p) { free(p); }
void Z_Freep(void *p) { Z_Free(*(void **)p); *(void **)p = NULL; }
char *Z_TagCopyString(const char *text, memtag_t tag)
{
    return text ? strcpy(Z_TagMalloc(strlen(text) + 1, tag), text) : NULL;
}
char *Z_CvarCopyString(const char *text) { return Z_TagCopyString(text, TAG_CVAR); }

int FS_LoadFileEx(const char *name, void **buffer, unsigned flags, memtag_t tag)
{
    assert(!buffer && tag == TAG_FREE);
    file_queries++;
    for (int i = 0; i < file_count; i++) {
        if (strcmp(name, files[i].name))
            continue;
        if ((flags & FS_TYPE_MASK) && (flags & FS_TYPE_MASK) != (files[i].flags & FS_TYPE_MASK))
            continue;
        if ((flags & FS_PATH_MASK) && (flags & FS_PATH_MASK) != (files[i].flags & FS_PATH_MASK))
            continue;
        return files[i].result;
    }
    return Q_ERR(ENOENT);
}

int FS_FPrintf(qhandle_t file, const char *format, ...)
{
    assert(file == 1);
    size_t used = strlen(saved);
    va_list args;
    va_start(args, format);
    int length = vsnprintf(saved + used, sizeof(saved) - used, format, args);
    va_end(args);
    assert(length >= 0 && (size_t)length < sizeof(saved) - used);
    return length;
}

/* Unrelated commands, completion and runtime input must not run here. */
void Prompt_AddMatch(genctx_t *ctx, const char *text) { abort(); }
void Com_Generic_c(genctx_t *ctx, int argnum) { abort(); }
void Com_SetColor(color_index_t color) { abort(); }
void FS_File_g(const char *path, const char *ext, unsigned flags, genctx_t *ctx) { abort(); }
size_t FS_NormalizePathBuffer(char *out, const char *in, size_t size) { abort(); }
bool CL_ForwardToServer(void) { abort(); }
bool CL_CheatsOK(void) { return true; }
void CL_UpdateUserinfo(cvar_t *var, from_t from) { abort(); }
void CL_ClientCommand(const char *text) { abort(); }
void IN_Activate(void) { abort(); }
void CL_CheckForPause(void) { abort(); }
void VID_ToggleFullscreen(void) { abort(); }
void Con_ToggleConsole_f(void) { abort(); }
void Con_Close(bool force) { abort(); }
void Key_Console(int key) { abort(); }
void Key_Message(int key) { abort(); }
void Char_Console(int key) { abort(); }
void Char_Message(int key) { abort(); }
void SCR_FinishCinematic(void) { abort(); }
#if USE_UI
void UI_KeyEvent(int key, bool down) { abort(); }
void UI_CharEvent(int key) { abort(); }
void UI_OpenMenu(uiMenu_t menu) { abort(); }
bool UI_IsMenuActive(const char *name) { abort(); }
#endif

static void Execute(const char *text)
{
    Cbuf_AddText(&cmd_buffer, text);
    Cbuf_Execute(&cmd_buffer);
    assert(!cmd_buffer.cursize);
}

static void Reset(void)
{
    file_count = file_queries = 0;
    Cvar_Set("game", "jump");
    Cvar_Set("dedicated", "0");
    Cvar_Set("gl_beamstyle", "0");
    Cvar_FindVar("gl_beamstyle")->flags = 0;
    Execute(stock_defaults);
}

static void File(const char *name, unsigned flags, int result)
{
    assert(file_count < q_countof(files));
    files[file_count].name = name;
    files[file_count].flags = flags;
    files[file_count++].result = result;
}

static void ExpectBinding(const char *key, const char *command)
{
    assert(!strcmp(Key_BindingForKey(Key_StringToKeynum(key)), command));
}

static void ExpectDefaults(void)
{
    ExpectBinding("w", "+forward");
    ExpectBinding("s", "+back");
    ExpectBinding("a", "+moveleft");
    ExpectBinding("d", "+moveright");
    ExpectBinding("SPACE", "+moveup");
    ExpectBinding("CTRL", "+movedown");
    ExpectBinding("MOUSE1", "+attack");
    ExpectBinding("MOUSE2", "+dj");
    ExpectBinding("F1", "inven");
    ExpectBinding("TAB", "score");
    ExpectBinding("ENTER", "invuse");
    ExpectBinding("[", "invprev");
    ExpectBinding("]", "invnext");
    ExpectBinding("e", "");
    ExpectBinding("r", "");
    ExpectBinding("F5", "team hard");
    ExpectBinding("F6", "team easy");
    ExpectBinding("q", "store");
    ExpectBinding("f", "");
    ExpectBinding("k", "kill");
    ExpectBinding("1", "replay g 1");
    ExpectBinding("UPARROW", "");
    ExpectBinding("MOUSE3", "");
    ExpectBinding(",", "");
    ExpectBinding(".", "");
    ExpectBinding("c", "");
    ExpectBinding("x", "");
    assert(Cvar_VariableInteger("cl_run") == 1);
    assert(Cvar_VariableInteger("gl_beamstyle") == 1);
    assert(Cvar_FindVar("cl_run")->flags & CVAR_ARCHIVE);
    assert(Cvar_FindVar("gl_beamstyle")->flags & CVAR_ARCHIVE);
}

static void TestFresh(void)
{
    Reset();
    CL_AddDefaultConfig(FS_PATH_ANY);
    ExpectDefaults();
    /* Later engine registration must retain the archived first-run value. */
    assert(Cvar_Get("gl_beamstyle", "0", 0)->flags & CVAR_ARCHIVE);

    Reset();
    Cvar_Set("game", "JuMp");
    File(COM_CONFIG_CFG, FS_TYPE_PAK | FS_PATH_GAME, 10);
    CL_AddDefaultConfig(FS_PATH_ANY);
    ExpectDefaults(); /* An archived q2config is never a player config. */
}

static void TestPreservation(void)
{
    const unsigned locations[] = {
        FS_TYPE_REAL | FS_PATH_BASE, FS_TYPE_REAL | FS_PATH_GAME
    };
    const int results[] = { 0, 42, Q_ERR(EACCES) };
    const char *names[] = { COM_CONFIG_CFG, COM_DEFAULT_CFG };
    for (size_t n = 0; n < q_countof(names); n++) {
        for (size_t l = 0; l < q_countof(locations); l++) {
            for (size_t r = 0; r < q_countof(results); r++) {
                Reset();
                File(names[n], locations[l], results[r]);
                Execute("bind w +back\nset cl_run 0\nset gl_beamstyle 2\n");
                CL_AddDefaultConfig(FS_PATH_ANY);
                ExpectBinding("w", "+back");
                ExpectBinding("F1", "cmd help");
                assert(Cvar_VariableInteger("cl_run") == 0);
                assert(Cvar_VariableInteger("gl_beamstyle") == 2);
            }
        }
    }
    Reset();
    File(COM_DEFAULT_CFG, FS_TYPE_PAK | FS_PATH_GAME, 42);
    CL_AddDefaultConfig(FS_PATH_ANY);
    ExpectBinding("TAB", "inven");

    Reset();
    CL_AddDefaultConfig(FS_PATH_ANY);
    Execute("bind w +back\nunbind f\nseta cl_run 0\nseta gl_beamstyle 2\n");
    ExpectBinding("w", "+back");
    ExpectBinding("f", "");
    assert(Cvar_VariableInteger("cl_run") == 0);
    assert(Cvar_VariableInteger("gl_beamstyle") == 2);
}

static void TestOtherModes(void)
{
    const char *games[] = { "", "baseq2", "ctf" };
    for (size_t i = 0; i < q_countof(games); i++) {
        Reset();
        Cvar_Set("game", games[i]);
        CL_AddDefaultConfig(FS_PATH_ANY);
        ExpectBinding("a", "+lookup");
        assert(!file_queries && Cvar_VariableInteger("cl_run") == 0);
    }
    Reset();
    Cvar_Set("dedicated", "1");
    CL_AddDefaultConfig(FS_PATH_ANY);
    ExpectBinding("TAB", "inven");
    assert(!file_queries);

    Reset();
    File(COM_CONFIG_CFG, FS_TYPE_REAL | FS_PATH_BASE, 42);
    CL_AddDefaultConfig(FS_PATH_GAME);
    ExpectDefaults(); /* Switching to an unconfigured Jump directory. */
}

static void TestSaveReload(void)
{
    Reset();
    CL_AddDefaultConfig(FS_PATH_ANY);
    saved[0] = 0;
    Key_WriteBindings(1);
    Cvar_WriteVariables(1, CVAR_ARCHIVE, false);
    assert(!strncmp(saved, "unbindall\n", 10));

    Reset();
    File(COM_CONFIG_CFG, FS_TYPE_REAL | FS_PATH_GAME, strlen(saved));
    CL_AddDefaultConfig(FS_PATH_ANY);
    Execute(saved);
    ExpectDefaults();

    Execute("unbind w\nbind f \"say custom\"\nseta cl_run 0\nseta gl_beamstyle 2\n");
    saved[0] = 0;
    Key_WriteBindings(1);
    Cvar_WriteVariables(1, CVAR_ARCHIVE, false);
    Reset();
    File(COM_CONFIG_CFG, FS_TYPE_REAL | FS_PATH_GAME, strlen(saved));
    CL_AddDefaultConfig(FS_PATH_ANY);
    Execute(saved);
    ExpectBinding("w", "");
    ExpectBinding("f", "say custom");
    ExpectBinding("UPARROW", "");
    assert(Cvar_VariableInteger("cl_run") == 0);
    assert(Cvar_VariableInteger("gl_beamstyle") == 2);
}

int main(void)
{
    Cbuf_Init();
    Cmd_Init();
    Cvar_Init();
    Key_Init();
    fs_game = Cvar_Get("game", "jump", 0);
    dedicated = Cvar_Get("dedicated", "0", 0);
    developer = Cvar_Get("developer", "0", 0);
    sv_running = Cvar_Get("sv_running", "0", 0);
    Cvar_Get("cl_run", "1", CVAR_ARCHIVE);
    TestFresh();
    TestPreservation();
    TestOtherModes();
    TestSaveReload();
    puts("default config tests passed");
    return 0;
}
