/* Real console history and filesystem I/O, with process boundaries stubbed. */
#undef NDEBUG
#include "../src/common/files.c"
#include "../src/client/console.c"
#include <assert.h>
#ifdef _WIN32
#include <process.h>
#else
#include <unistd.h>
#endif

static cvar_t base_var, home_var, game_var, history_var;
cvar_t *sys_basedir = &base_var;
cvar_t *sys_homedir = &home_var;
static int allocations;
static char test_root[MAX_OSPATH];

void Com_LPrintf(print_type_t type, const char *format, ...)
{
}

void Com_SetLastError(const char *message)
{
}

const char *Com_GetLastError(void)
{
    return "fixture";
}

void Com_Error(error_type_t code, const char *format, ...)
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
    allocations++;
    return p;
}

void *Z_TagMallocz(size_t size, memtag_t tag)
{
    return memset(Z_TagMalloc(size, tag), 0, size);
}

void *Z_Malloc(size_t size)
{
    return Z_TagMalloc(size, TAG_GENERAL);
}

void *Z_Realloc(void *p, size_t size)
{
    if (!p)
        allocations++;
    p = realloc(p, size);
    assert(p);
    return p;
}

void Z_Free(void *p)
{
    if (p) {
        allocations--;
        free(p);
    }
}

void Z_Freep(void *ptr)
{
    void **p = ptr;
    Z_Free(*p);
    *p = NULL;
}

char *Z_TagCopyString(const char *text, memtag_t tag)
{
    return strcpy(Z_TagMalloc(strlen(text) + 1, tag), text);
}

/* The fixture directories contain no game assets or archives. */
void Sys_ListFiles_r(listfiles_t *list, const char *path, int depth)
{
}

void SV_RestartFilesystem(void)
{
}

cvar_t *Cvar_FullSet(const char *name, const char *value, int flags, from_t from)
{
    return NULL;
}

/* These UI and command entry points must never run in the offline fixture. */
cmdbuf_t cmd_buffer;
char *cmd_optarg;
const uint32_t colorTable[8];
refcfg_t r_config;
const vid_driver_t *vid;
client_state_t cl;
client_static_t cls;
unsigned com_framenum, com_localTime, com_localTime2;

void Cbuf_AddText(cmdbuf_t *buf, const char *text)
{
    abort();
}

void Cmd_Register(const cmdreg_t *reg)
{
    abort();
}

void Cmd_Deregister(const cmdreg_t *reg)
{
    abort();
}

int Cmd_Argc(void)
{
    abort();
}

char *Cmd_Argv(int arg)
{
    abort();
}

char *Cmd_RawArgs(void)
{
    abort();
}

int Cmd_ParseOptions(const cmd_option_t *opt)
{
    abort();
}

void Cmd_PrintHelp(const cmd_option_t *opt)
{
    abort();
}

void Cmd_PrintUsage(const cmd_option_t *opt, const char *suffix)
{
    abort();
}

void Cmd_PrintHint(void)
{
    abort();
}

void Cmd_Command_g(genctx_t *ctx)
{
    abort();
}

void Cmd_Alias_g(genctx_t *ctx)
{
    abort();
}

void Cmd_TokenizeString(const char *text, bool expand)
{
    abort();
}

char *Cmd_ArgsFrom(int from)
{
    abort();
}

char *Cmd_RawArgsFrom(int from)
{
    abort();
}

int Cmd_ArgOffset(int arg)
{
    abort();
}

int Cmd_FindArgForOffset(int offset)
{
    abort();
}

void Com_Generic_c(genctx_t *ctx, int argnum)
{
    abort();
}

void Com_Address_g(genctx_t *ctx)
{
    abort();
}

size_t Com_Time_m(char *buffer, size_t size)
{
    abort();
}

void Com_AddConfigFile(const char *name, unsigned flags)
{
    abort();
}

cvar_t *Cvar_Get(const char *name, const char *value, int flags)
{
    abort();
}

void Cvar_SetByVar(cvar_t *var, const char *value, from_t from)
{
    abort();
}

float Cvar_ClampValue(cvar_t *var, float low, float high)
{
    abort();
}

int Cvar_ClampInteger(cvar_t *var, int low, int high)
{
    abort();
}

void Cvar_Variable_g(genctx_t *ctx)
{
    abort();
}

void Z_LeakTest(memtag_t tag)
{
    abort();
}

bool NET_StringToAdr(const char *s, netadr_t *a, int port)
{
    abort();
}

void CL_RestartFilesystem(bool total)
{
    abort();
}

void SCR_EndLoadingPlaque(void)
{
    abort();
}

void SCR_UpdateScreen(void)
{
    abort();
}

bool SCR_ParseColor(const char *s, color_t *color)
{
    abort();
}

qhandle_t R_RegisterImage(const char *name, imagetype_t type, imageflags_t flags)
{
    abort();
}

void R_ClearColor(void)
{
    abort();
}

void R_SetAlpha(float alpha)
{
    abort();
}

void R_SetColor(uint32_t color)
{
    abort();
}

float R_ClampScale(cvar_t *var)
{
    abort();
}

void R_SetScale(float scale)
{
    abort();
}

void R_DrawChar(int x, int y, int flags, int ch, qhandle_t font)
{
    abort();
}

int R_DrawString(int x, int y, int flags, size_t max, const char *s, qhandle_t font)
{
    abort();
}

void R_DrawKeepAspectPic(int x, int y, int w, int h, qhandle_t pic)
{
    abort();
}

void Key_SetDest(keydest_t dest)
{
    abort();
}

int Key_IsDown(int key)
{
    abort();
}

void CL_ClientCommand(const char *text)
{
    abort();
}

void CL_SendRcon(const netadr_t *adr, const char *pass, const char *cmd)
{
    abort();
}

void cl_timeout_changed(cvar_t *self)
{
    abort();
}

float SCR_FadeAlpha(unsigned start, unsigned visible, unsigned fade)
{
    abort();
}

int SCR_DrawStringEx(int x, int y, int flags, size_t max, const char *s, qhandle_t font)
{
    abort();
}

void HUD_LayoutBegin(int id)
{
    abort();
}

void HUD_LayoutEnd(void)
{
    abort();
}

bool Key_GetOverstrikeMode(void)
{
    abort();
}

void Key_SetOverstrikeMode(bool overstrike)
{
    abort();
}

static void FixturePath(char *path, const char *name)
{
    assert(Q_snprintf(path, MAX_OSPATH, "%s/%s", test_root, name) < MAX_OSPATH);
}

static void WriteFixture(const char *name, const char *text)
{
    char path[MAX_OSPATH];
    FixturePath(path, name);
    assert(!FS_CreatePath(path));
    FILE *f = fopen(path, "w");
    assert(f);
    assert(fputs(text, f) >= 0);
    assert(!fclose(f));
}

static void ExpectFile(const char *name, const char *expected)
{
    char path[MAX_OSPATH], contents[4096];
    FixturePath(path, name);
    FILE *f = fopen(path, "r");
    assert(f);
    size_t len = fread(contents, 1, sizeof(contents) - 1, f);
    assert(!ferror(f));
    contents[len] = 0;
    assert(!strcmp(contents, expected));
    assert(!fclose(f));
}

static void EnterCommand(const char *text)
{
    IF_Replace(&con.prompt.inputLine, text);
    assert(!strcmp(Prompt_Action(&con.prompt), text));
}

static void ExpectPrevious(const char *text)
{
    Prompt_HistoryUp(&con.prompt);
    assert(!strcmp(con.prompt.inputLine.text, text));
    Prompt_HistoryDown(&con.prompt);
    assert(!con.prompt.inputLine.text[0]);
}

static void RemoveFixture(const char *name)
{
    char path[MAX_OSPATH];
    FixturePath(path, name);
    assert(!remove(path));
}

static void RemoveFixtureDir(const char *name)
{
    char path[MAX_OSPATH];
    FixturePath(path, name);
#ifdef _WIN32
    assert(!_rmdir(path));
#else
    assert(!rmdir(path));
#endif
}

int main(void)
{
    char base[MAX_OSPATH], home[MAX_OSPATH];
#ifdef _WIN32
    unsigned pid = _getpid();
#else
    unsigned pid = getpid();
#endif
    Q_snprintf(test_root, sizeof(test_root), "console-history-%u", pid);
    assert(!os_mkdir(test_root));
    FixturePath(base, "install");
    FixturePath(home, "home");
    base_var.string = base;
    home_var.string = "";
    game_var.string = "jump";
    fs_game = &game_var;
    history_var.integer = HISTORY_SIZE;
    con_history = &history_var;
    IF_Init(&con.prompt.inputLine, 0, MAX_FIELD_TEXT - 1);
    List_Init(&fs_hard_links);
    List_Init(&fs_soft_links);

    /* A Jump-only installation can save history without any baseq2 directory. */
    setup_base_paths();
    setup_game_paths();
    Con_PostInit();
    assert(!con.prompt.inputLineNum);
    EnterCommand("first-jump-command");
    Con_Shutdown();
    ExpectFile("install/jump/.conhistory", "first-jump-command\n");
    assert(FS_LoadFileEx(COM_HISTORYFILE_NAME, NULL,
                        FS_PATH_BASE | FS_TYPE_REAL, TAG_FREE) == Q_ERR(ENOENT));
    RemoveFixture("install/jump/.conhistory");
    free_all_paths();

    /* Jump-only startup inherits old history, then persists in jump/. */
    WriteFixture("install/baseq2/.conhistory", "legacy-one\nlegacy-two\n");
    setup_base_paths();
    setup_game_paths();
    Con_PostInit();
    assert(con.prompt.inputLineNum == 2);
    ExpectPrevious("legacy-two");
    EnterCommand("jump-only");
    Con_Shutdown();
    ExpectFile("install/jump/.conhistory", "legacy-one\nlegacy-two\njump-only\n");
    ExpectFile("install/baseq2/.conhistory", "legacy-one\nlegacy-two\n");
    Con_PostInit();
    assert(con.prompt.inputLineNum == 3);
    ExpectPrevious("jump-only");

    /* Save with the new game cvar but the old filesystem, as during a switch. */
    EnterCommand("jump-last");
    game_var.string = "ctf";
    Con_SaveHistory();
    WriteFixture("install/ctf/.conhistory", "ctf-only\n");
    FS_Restart(false);
    Con_PostInit();
    assert(con.prompt.inputLineNum == 1);
    ExpectPrevious("ctf-only");
    EnterCommand("ctf-last");
    game_var.string = "jump";
    Con_SaveHistory();
    FS_Restart(false);
    Con_PostInit();
    assert(con.prompt.inputLineNum == 4);
    ExpectPrevious("jump-last");
    ExpectFile("install/ctf/.conhistory", "ctf-only\nctf-last\n");

    /* Empty mod history must not import base history or retain old entries. */
    WriteFixture("install/jump/.conhistory", "");
    Con_PostInit();
    assert(!con.prompt.inputLineNum);
    Prompt_HistoryUp(&con.prompt);
    assert(!con.prompt.inputLine.text[0]);
    Prompt_Clear(&con.prompt);
    EnterCommand("one");
    EnterCommand("two");
    EnterCommand("three");
    history_var.integer = 2;
    Con_SaveHistory();
    ExpectFile("install/jump/.conhistory", "two\nthree\n");

    /* Disabling persistence leaves existing files untouched. */
    history_var.integer = 0;
    EnterCommand("not-saved");
    Con_Shutdown();
    ExpectFile("install/jump/.conhistory", "two\nthree\n");
    Con_PostInit();
    assert(!con.prompt.inputLineNum);
    history_var.integer = HISTORY_SIZE;

    /* Returning to baseq2 restores its own history. */
    game_var.string = "";
    FS_Restart(false);
    Con_PostInit();
    assert(con.prompt.inputLineNum == 2);
    ExpectPrevious("legacy-two");
    EnterCommand("base-only");
    Con_Shutdown();
    ExpectFile("install/baseq2/.conhistory", "legacy-one\nlegacy-two\nbase-only\n");

    /* homedir provides both fallback history and the mod's write directory. */
    free_all_paths();
    home_var.string = home;
    game_var.string = "jump";
    WriteFixture("home/baseq2/.conhistory", "home-legacy\n");
    setup_base_paths();
    setup_game_paths();
    Con_PostInit();
    assert(con.prompt.inputLineNum == 1);
    ExpectPrevious("home-legacy");
    EnterCommand("home-jump");
    Con_Shutdown();
    ExpectFile("home/jump/.conhistory", "home-legacy\nhome-jump\n");
    ExpectFile("install/jump/.conhistory", "two\nthree\n");
    Con_PostInit();
    ExpectPrevious("home-jump");

    /* System history retains its explicit base-directory policy. */
    commandPrompt_t system_prompt = { 0 };
    Prompt_LoadHistory(&system_prompt, COM_HISTORYFILE_NAME, FS_PATH_BASE);
    assert(system_prompt.inputLineNum == 1);
    assert(!strcmp(system_prompt.history[0], "home-legacy"));
    Prompt_SaveHistory(&system_prompt, SYS_HISTORYFILE_NAME, HISTORY_SIZE, FS_PATH_BASE);
    ExpectFile("home/baseq2/.syshistory", "home-legacy\n");
    Prompt_Clear(&system_prompt);
    Prompt_Clear(&con.prompt);
    free_all_paths();
    assert(!allocations);

    RemoveFixture("install/baseq2/.conhistory");
    RemoveFixture("install/jump/.conhistory");
    RemoveFixture("install/ctf/.conhistory");
    RemoveFixture("home/baseq2/.conhistory");
    RemoveFixture("home/baseq2/.syshistory");
    RemoveFixture("home/jump/.conhistory");
    RemoveFixtureDir("install/baseq2");
    RemoveFixtureDir("install/jump");
    RemoveFixtureDir("install/ctf");
    RemoveFixtureDir("install");
    RemoveFixtureDir("home/baseq2");
    RemoveFixtureDir("home/jump");
    RemoveFixtureDir("home");
#ifdef _WIN32
    assert(!_rmdir(test_root));
#else
    assert(!rmdir(test_root));
#endif
    puts("console history: persistence, isolation, fallback, limits and homedir passed");
    return 0;
}
