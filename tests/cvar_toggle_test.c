/* Exercise registered toggle and real cvar writes with offline engine boundaries. */
#undef NDEBUG
#include "../src/common/cvar.c"
#include <assert.h>

static const char *const *test_argv;
static int test_argc, messages, callbacks, userinfo_updates, allocations;
static from_t test_from, userinfo_from;
static xcommand_t toggle_command;
static cvar_t server_var, developer_var;
cvar_t *sv_running = &server_var;
cvar_t *developer = &developer_var;
cvar_t *fs_game;
bool com_initialized = true;
unsigned com_framenum, com_localTime2;
int cmd_optind;

int Cmd_Argc(void)
{
    return test_argc;
}

char *Cmd_Argv(int index)
{
    return (char *)(index >= 0 && index < test_argc ? test_argv[index] : "");
}

from_t Cmd_From(void)
{
    return test_from;
}

void Cmd_Register(const cmdreg_t *commands)
{
    for (; commands->name; commands++) {
        if (!strcmp(commands->name, "toggle")) {
            assert(!toggle_command);
            toggle_command = commands->function;
        }
    }
}

void Com_LPrintf(print_type_t type, const char *format, ...)
{
    messages++;
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

char *Z_TagCopyString(const char *value, memtag_t tag)
{
    return strcpy(Z_TagMalloc(strlen(value) + 1, tag), value);
}

char *Z_CvarCopyString(const char *value)
{
    return Z_TagCopyString(value, TAG_CVAR);
}

void Z_Free(void *p)
{
    if (p) {
        allocations--;
        free(p);
    }
}

void Z_Freep(void *p)
{
    void **ptr = p;
    Z_Free(*ptr);
    *ptr = NULL;
}

bool CL_CheatsOK(void)
{
    return false;
}

void CL_UpdateUserinfo(cvar_t *var, from_t from)
{
    userinfo_updates++;
    userinfo_from = from;
}

/* Unrelated cvar commands and completion must not run in this fixture. */
char *Cmd_ArgsFrom(int from)
{
    abort();
}

void Cmd_Option_c(const cmd_option_t *opt, xgenerator_t g, genctx_t *ctx, int argnum)
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

void Prompt_AddMatch(genctx_t *ctx, const char *s)
{
    abort();
}

int FS_FPrintf(qhandle_t f, const char *format, ...)
{
    abort();
}

static void changed(cvar_t *var)
{
    callbacks++;
}

static void clear_vars(void)
{
    while (cvar_vars) {
        cvar_t *var = cvar_vars;
        cvar_vars = var->next;
        Z_Free(var->string);
        Z_Free(var->default_string);
        Z_Free(var->latched_string);
        Z_Free(var);
    }
    memset(cvarHash, 0, sizeof(cvarHash));
    assert(allocations == 0);
}

static cvar_t *setup(const char *value, int flags)
{
    clear_vars();
    cvar_t *var = Cvar_Get("cl_maxfps", value, flags);
    var->changed = changed;
    var->modified = false;
    server_var.integer = 0;
    cvar_modified = 0;
    test_from = FROM_CONSOLE;
    return var;
}

static void run(const char *const *args, size_t count)
{
    test_argv = args;
    test_argc = (int)count;
    messages = callbacks = userinfo_updates = 0;
    assert(toggle_command);
    toggle_command();
}

#define TOGGLE(...) do { \
    const char *args[] = { "toggle", __VA_ARGS__ }; \
    run(args, q_countof(args)); \
} while (0)

static void test_recovery_and_cycles(void)
{
    cvar_t *var = setup("60", 0);
    const char *values[] = { "30", "120", "30" };
    for (size_t i = 0; i < q_countof(values); i++) {
        TOGGLE("cl_maxfps", "30", "120");
        assert(!strcmp(var->string, values[i]));
        assert(callbacks == 1 && messages == 0 && var->modified);
    }

    var = setup("30", 0);
    const char *three[] = { "60", "120", "30" };
    for (size_t i = 0; i < q_countof(three); i++) {
        TOGGLE("cl_maxfps", "30", "60", "120");
        assert(!strcmp(var->string, three[i]) && callbacks == 1);
    }

    var = setup("60", 0);
    TOGGLE("cl_maxfps", "30");
    assert(!strcmp(var->string, "30") && callbacks == 1);
    TOGGLE("cl_maxfps", "30");
    assert(!strcmp(var->string, "30") && callbacks == 0);
}

static void test_existing_string_matching(void)
{
    cvar_t *var = setup("GREEN", 0);
    TOGGLE("cl_maxfps", "red", "green", "blue");
    assert(!strcmp(var->string, "blue"));

    var = setup("30.0", 0);
    TOGGLE("cl_maxfps", "30", "120");
    assert(!strcmp(var->string, "30"));
    var = setup("030", 0);
    TOGGLE("cl_maxfps", "30", "120");
    assert(!strcmp(var->string, "30"));
}

static void test_missing_arguments_and_boolean_toggle(void)
{
    cvar_t *var = setup("0", 0);
    const char *missing[] = { "toggle" };
    run(missing, q_countof(missing));
    assert(messages > 0 && callbacks == 0 && !strcmp(var->string, "0"));
    TOGGLE("unknown_cvar");
    assert(messages > 0 && callbacks == 0 && !Cvar_FindVar("unknown_cvar"));
    TOGGLE("cl_maxfps");
    assert(!strcmp(var->string, "1") && callbacks == 1 && messages == 0);
    TOGGLE("cl_maxfps");
    assert(!strcmp(var->string, "0") && callbacks == 1 && messages == 0);
    var = setup("60", 0);
    TOGGLE("cl_maxfps");
    assert(!strcmp(var->string, "60") && callbacks == 0 && messages > 0);
}

static void test_setter_protections_and_provenance(void)
{
    const int protected_flags[] = { CVAR_ROM, CVAR_CHEAT, CVAR_NOSET };
    for (size_t i = 0; i < q_countof(protected_flags); i++) {
        cvar_t *var = setup("60", protected_flags[i]);
        TOGGLE("cl_maxfps", "30", "120");
        assert(!strcmp(var->string, "60") && callbacks == 0 && messages > 0);
    }

    cvar_t *var = setup("60", CVAR_LATCH);
    server_var.integer = 1;
    TOGGLE("cl_maxfps", "30", "120");
    assert(!strcmp(var->string, "60") && !strcmp(var->latched_string, "30"));
    assert(callbacks == 0);

    var = setup("60", CVAR_USERINFO);
    test_from = FROM_MENU;
    TOGGLE("cl_maxfps", "30", "120");
    assert(!strcmp(var->string, "30") && callbacks == 1);
    assert(userinfo_updates == 1 && userinfo_from == FROM_MENU);
    assert((var->flags & (CVAR_MODIFIED | CVAR_ARCHIVE)) == (CVAR_MODIFIED | CVAR_ARCHIVE));
}

int main(void)
{
    Cvar_Init();
    test_recovery_and_cycles();
    test_existing_string_matching();
    test_missing_arguments_and_boolean_toggle();
    test_setter_protections_and_provenance();
    clear_vars();
    puts("cvar-toggle: all checks passed");
    return 0;
}
