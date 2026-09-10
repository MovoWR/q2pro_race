/* Exercise production sh handlers and completion with engine boundaries stubbed. */
#include "../src/jump/sh_init.c"
#include "../src/jump/sh_hud_menu.c"
#include "../src/jump/sh_menus.c"
#undef NDEBUG
#include <assert.h>

static int cmd_argc;
static char *cmd_argv[8];
static char args_buffer[MAX_STRING_CHARS];
static cvar_t test_vars[256];
static int test_var_count, renderer_calls, matches, cvar_sets;
static const char *match_names[128];
static char test_output[32768];
static size_t test_output_length;

void Com_LPrintf(print_type_t type, const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    test_output_length += vsnprintf(test_output + test_output_length,
        sizeof(test_output) - test_output_length, fmt, ap);
    va_end(ap);
}

cvar_t *Cvar_FindVar(const char *name)
{
    for (int i = 0; i < test_var_count; i++)
        if (!strcmp(test_vars[i].name, name)) return &test_vars[i];
    return NULL;
}

static char *CopyString(const char *value)
{
    char *copy = malloc(strlen(value) + 1);
    assert(copy);
    return strcpy(copy, value);
}

static void ParseValue(cvar_t *var)
{
    var->integer = atoi(var->string);
    var->value = strtof(var->string, NULL);
    if (var->value != 0.0f && !isnormal(var->value)) var->value = 0;
}

cvar_t *Cvar_Get(const char *name, const char *value, int flags)
{
    cvar_t *var = Cvar_FindVar(name);
    if (var) { var->flags |= flags; return var; }
    assert(test_var_count < q_countof(test_vars));
    var = &test_vars[test_var_count++];
    var->name = CopyString(name);
    var->string = CopyString(value);
    var->default_string = CopyString(value);
    var->flags = flags;
    ParseValue(var);
    return var;
}

cvar_t *Cvar_Set(const char *name, const char *value)
{
    cvar_t *var = Cvar_Get(name, value, 0);
    cvar_sets++;
    free(var->string);
    var->string = CopyString(value);
    ParseValue(var);
    return var;
}

const char *Cvar_VariableString(const char *name)
{
    cvar_t *var = Cvar_FindVar(name);
    return var ? var->string : "";
}

int Q_strcasecmp(const char *a, const char *b)
{
    while (*a && Q_tolower(*a) == Q_tolower(*b)) { a++; b++; }
    return Q_tolower(*a) - Q_tolower(*b);
}

uint32_t Q_rand_uniform(uint32_t n) { return 0; }
void R_SetColor(uint32_t value) { renderer_calls++; }
void Prompt_AddMatch(genctx_t *ctx, const char *name)
{
    assert(matches < q_countof(match_names));
    match_names[matches++] = name;
}

int Cmd_Argc(void) { return cmd_argc; }
char *Cmd_Argv(int arg) { return arg >= 0 && arg < cmd_argc ? cmd_argv[arg] : ""; }
char *Cmd_ArgsFrom(int from)
{
    args_buffer[0] = 0;
    for (int i = from; i < cmd_argc; i++) {
        if (i > from) strcat(args_buffer, " ");
        strcat(args_buffer, cmd_argv[i]);
    }
    return args_buffer;
}

/* Inputs are individual tokens after console tokenization. Spaces within a
 * value represent a quoted argument; additional arguments use separate slots. */
static void Args(const char *section, const char *command, const char *value)
{
    memset(cmd_argv, 0, sizeof(cmd_argv));
    cmd_argc = value ? 4 : command ? 3 : section ? 2 : 1;
    cmd_argv[0] = "sh";
    cmd_argv[1] = (char *)section;
    cmd_argv[2] = (char *)command;
    cmd_argv[3] = (char *)value;
    test_output_length = 0;
    test_output[0] = 0;
}

static void Invoke(const char *section, const char *command, const char *value)
{
    Args(section, command, value);
    SH_Cmd_f();
}

static bool Matched(const char *name)
{
    for (int i = 0; i < matches; i++)
        if (!strcmp(match_names[i], name)) return true;
    return false;
}

static void Complete(const char *section, const char *command, int argnum)
{
    Args(section, command, NULL);
    matches = 0;
    genctx_t ctx = { 0 };
    ctx.argnum = argnum;
    SH_Cmd_g(&ctx, argnum);
}

/* Helper position and UPS share strict parsing and fixed setting limits. */
static void CheckNumericSettings(void)
{
    static const struct {
        const char *section, *command, *cvar, *low, *high, *below, *above;
    } cases[] = {
        { "hud", "ypos", "sh_y", "-4000", "4000", "-4000.01", "4000.01" },
        { "ups", "ypos", "sh_ups_y", "-4000", "4000", "-4000.01", "4000.01" },
        { "ups", "scale", "sh_ups_scale", "0.25", "8", "0.24", "8.01" },
    };
    static const char *invalid[] = {
        "", " ", "abc", "+", "1e", "nan", "NaN", "nan(1)", "inf", "-inf",
        "1e100", "-1e100", "1e-100", "0.5junk", "0.5 1"
    };

    for (int i = 0; i < q_countof(cases); i++) {
        cvar_t *var = Cvar_FindVar(cases[i].cvar);
        assert(var);
        const char *endpoints[] = { cases[i].low, cases[i].high };
        for (int j = 0; j < q_countof(endpoints); j++) {
            int before = cvar_sets;
            Invoke(cases[i].section, cases[i].command, endpoints[j]);
            assert(cvar_sets == before + 1);
            assert(!strcmp(var->string, endpoints[j]));
            assert(strstr(test_output, "set to:"));
        }

        Cvar_Set(cases[i].cvar, var->default_string);
        int before = cvar_sets;
        Invoke(cases[i].section, cases[i].command, NULL);
        assert(cvar_sets == before && !strcmp(var->string, var->default_string));
        assert(test_output[0] == '-' && strstr(test_output, var->string));

        char suffix[32];
        snprintf(suffix, sizeof(suffix), "%sjunk", cases[i].low);
        const char *bad_values[] = { cases[i].below, cases[i].above, suffix };
        for (int j = 0; j < q_countof(bad_values) + q_countof(invalid); j++) {
            const char *value = j < q_countof(bad_values)
                                ? bad_values[j] : invalid[j - q_countof(bad_values)];
            Invoke(cases[i].section, cases[i].command, value);
            assert(cvar_sets == before && !strcmp(var->string, var->default_string));
            assert(strstr(test_output, "Invalid value"));
        }

        /* A separate empty argument must be rejected too, even though joining
         * arguments would look like harmless trailing whitespace. */
        const char *extra[] = { "1", "junk", "", " " };
        for (int j = 0; j < q_countof(extra); j++) {
            Args(cases[i].section, cases[i].command, cases[i].low);
            cmd_argv[cmd_argc++] = (char *)extra[j];
            SH_Cmd_f();
            assert(cvar_sets == before && !strcmp(var->string, var->default_string));
            assert(strstr(test_output, "Invalid value"));
        }
    }

    const char *valid[] = { " 0.5 ", "+5e-1", "-0" };
    for (int i = 0; i < q_countof(valid); i++) {
        int before = cvar_sets;
        Invoke("ups", "ypos", valid[i]);
        assert(cvar_sets == before + 1);
        assert(!strcmp(cl_strafehelperUpsY->string, valid[i]));
    }
}

int main(void)
{
    SH_Init();
    for (int i = 0; i < test_var_count; i++)
        assert(strncmp(test_vars[i].name, "sh_ice", 6));
    int registered_vars = test_var_count;
    Invoke("ice", "enable", NULL);
    assert(test_var_count == registered_vars && !Cvar_FindVar("sh_ice"));
    assert(strstr(test_output, "Usage: sh <section>") && !strstr(test_output, "sh ice"));
    Invoke("status", NULL, NULL);
    assert(strstr(test_output, "Center UPS Status"));
    assert(!strstr(test_output, "Ice Turn Meter"));

    Invoke("ups", "status", NULL);
    assert(strstr(test_output, "- Y offset: -5.00\n"));
    Invoke("status", NULL, NULL);
    const char *ups_status = strstr(test_output, "Center UPS Status");
    assert(ups_status);
    assert(strstr(ups_status, "Y offset"));

    CheckNumericSettings();

    const char *valid[] = { "0", "-50", "50", "-4000", "4000", "-12.5", " 5 ", "+5e-1", "-0" };
    for (int i = 0; i < q_countof(valid); i++) {
        Invoke("hud", "ypos", valid[i]);
        assert(!strcmp(cl_strafeHelperY->string, valid[i]));
        assert(strstr(test_output, "offset from center set"));
    }
    Invoke("hud", "center_marker", NULL);
    assert(cl_strafeHelperCenterMarker->integer == 0);
    assert(!strcmp(test_output, "Center marker disabled.\n"));
    Invoke("hud", "center_marker", NULL);
    assert(cl_strafeHelperCenterMarker->integer == 1);
    assert(!strcmp(test_output, "Center marker enabled.\n"));

    Complete(NULL, NULL, 1);
    assert(matches == 4 && Matched("hud") && Matched("ups") && Matched("status") && Matched("help") && !Matched("ice"));
    Complete("hud", NULL, 2);
    assert(Matched("ypos") && Matched("preset") && !Matched("fade_inactive"));
    Complete("hud", "bar_style", 3);
    assert(matches == 4 && Matched("solid") && Matched("gradient"));
    Complete("hud", "smoothing_mode", 3);
    assert(matches == 5 && Matched("linear") && Matched("exponential"));
    Complete("ice", "style", 3);
    assert(matches == 0);
    Complete("ups", "format", 3);
    assert(matches == 3 && Matched("prefix"));
    Complete("hud", "fade_inactive", 3);
    assert(matches == 0 && renderer_calls == 0);
    puts("sh-menu: production dispatch, complete numeric parsing, toggles and completion passed");
    return 0;
}