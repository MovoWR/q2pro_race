/* Exercise production FPS commands and slot initialization without running a client. */
#undef NDEBUG
#include "../src/client/fps.c"
#include <assert.h>

static struct {
    cvar_t var;
    char name[40], text[128];
} variables[32];
static int variable_count, argc_value, writes, retimes, messages;
static const char *argv_value[5];

int Cmd_Argc(void) { return argc_value; }
char *Cmd_Argv(int n)
{
    return (char *)(n >= 0 && n < argc_value ? argv_value[n] : "");
}

static void SetText(cvar_t *var, const char *value)
{
    assert(strlen(value) < sizeof(variables[0].text));
    strcpy(var->string, value);
}

cvar_t *Cvar_Get(const char *name, const char *value, int flags)
{
    for (int i = 0; i < variable_count; i++)
        if (!strcmp(variables[i].name, name))
            return &variables[i].var;
    assert(variable_count < q_countof(variables));
    int i = variable_count++;
    assert(strlen(name) < sizeof(variables[i].name));
    strcpy(variables[i].name, name);
    variables[i].var = (cvar_t){ .name = variables[i].name, .string = variables[i].text, .flags = flags };
    SetText(&variables[i].var, value);
    return &variables[i].var;
}

const char *Cvar_VariableString(const char *name)
{
    for (int i = 0; i < variable_count; i++)
        if (!strcmp(variables[i].name, name))
            return variables[i].text;
    return "";
}

cvar_t *Cvar_Set(const char *name, const char *value)
{
    assert(!strcmp(name, "cl_maxfps"));
    cvar_t *var = Cvar_Get(name, "60", 0);
    SetText(var, value);
    writes++;
    return var;
}

void CL_UpdateFrameTimes(void) { retimes++; }
void Com_LPrintf(print_type_t type, const char *format, ...) { messages++; }
void Com_Error(error_type_t type, const char *format, ...) { abort(); }

static void Reset(void)
{
    variable_count = 0;
    memset(variables, 0, sizeof(variables));
    Cvar_Get("cl_maxfps", "60", 0);
    writes = retimes = messages = 0;
}

static void Invoke(xcommand_t command, const char *name, const char *arg1, const char *arg2)
{
    argc_value = arg2 ? 3 : arg1 ? 2 : 1;
    argv_value[0] = name;
    argv_value[1] = arg1;
    argv_value[2] = arg2;
    writes = retimes = messages = 0;
    command();
}

static void ExpectUnchanged(void)
{
    assert(!writes && !retimes && messages > 0);
    assert(!strcmp(Cvar_VariableString("cl_maxfps"), "60"));
}

static const char *const invalid_values[] = {
    "", "0", "-30", "+30", "30.0", "30junk", " 30", "30 ", "1e2", "0x1e",
    "2147483648", "4294967297", "99999999999999999999999999999999999999"
};

static void TestDefaults(void)
{
    for (size_t i = 0; i < q_countof(invalid_values); i++) {
        Reset();
        Cvar_Get("fps_default_hold", invalid_values[i], CVAR_ARCHIVE);
        Cvar_Get("fps_default_release", invalid_values[i], CVAR_ARCHIVE);
        cvar_t *saved = Cvar_Get("fps_hold_2", "75", CVAR_ARCHIVE);
        cvar_t *invalid_saved = Cvar_Get("fps_release_3", "broken", CVAR_ARCHIVE);
        CL_InitFpsSlots();
        assert(!strcmp(fps_hold[0]->string, "30"));
        assert(!strcmp(fps_release[0]->string, "120"));
        assert(fps_hold[1] == saved && !strcmp(saved->string, "75"));
        assert(fps_release[2] == invalid_saved && !strcmp(invalid_saved->string, "broken"));
        assert(!strcmp(Cvar_VariableString("fps_default_hold"), invalid_values[i]));
        assert(!writes && !retimes);
    }
    Reset();
    Cvar_Get("fps_default_hold", "45", CVAR_ARCHIVE);
    cvar_t *release = Cvar_Get("fps_default_release", "1000", CVAR_ARCHIVE);
    CL_InitFpsSlots();
    for (int i = 0; i < NUM_FPS_SLOTS; i++) {
        assert(!strcmp(fps_hold[i]->string, "45"));
        assert(!strcmp(fps_release[i]->string, "1000"));
        assert(fps_hold[i]->flags & CVAR_ARCHIVE);
        assert(fps_release[i]->flags & CVAR_ARCHIVE);
    }
    SetText(release, "90");
    CL_InitFpsSlots();
    assert(!strcmp(fps_release[0]->string, "1000"));
}

static void TestSlots(void)
{
    Reset();
    Cvar_Get("fps_default_hold", "30", CVAR_ARCHIVE);
    Cvar_Get("fps_default_release", "120", CVAR_ARCHIVE);
    CL_InitFpsSlots();
    static const char *const invalid_slots[] = {
        "", "0", "13", "1junk", "1.0", "+1", "-1", " 1", "1 ",
        "2147483648", "4294967297", "-2147483648"
    };
    for (size_t i = 0; i < q_countof(invalid_slots); i++) {
        Invoke(CL_FpsHoldDown_f, "+fps_hold", invalid_slots[i], NULL);
        ExpectUnchanged();
        Invoke(CL_FpsHoldUp_f, "-fps_hold", invalid_slots[i], NULL);
        ExpectUnchanged();
    }
    Invoke(CL_FpsHoldDown_f, "+fps_hold", NULL, NULL);
    ExpectUnchanged();
    for (size_t i = 0; i < q_countof(invalid_values); i++) {
        SetText(fps_hold[0], invalid_values[i]);
        SetText(fps_release[0], invalid_values[i]);
        Invoke(CL_FpsHoldDown_f, "+fps_hold", "1", NULL);
        ExpectUnchanged();
        Invoke(CL_FpsHoldUp_f, "-fps_hold", "1", NULL);
        ExpectUnchanged();
    }
    SetText(fps_hold[0], "00030");
    SetText(fps_release[0], "2147483647");
    Invoke(CL_FpsHoldDown_f, "+fps_hold", "01", NULL);
    assert(writes == 1 && retimes == 1 && !messages);
    assert(!strcmp(Cvar_VariableString("cl_maxfps"), "00030"));
    Invoke(CL_FpsHoldUp_f, "-fps_hold", "1", NULL);
    assert(writes == 1 && retimes == 1 && !messages);
    assert(!strcmp(Cvar_VariableString("cl_maxfps"), "2147483647"));
}

static void TestPairsAndShortcuts(void)
{
    Reset();
    for (size_t i = 0; i < q_countof(invalid_values); i++) {
        Invoke(CL_FpsDown_f, "+fps", invalid_values[i], "120");
        ExpectUnchanged();
        Invoke(CL_FpsUp_f, "-fps", invalid_values[i], "120");
        ExpectUnchanged();
        Invoke(CL_FpsDown_f, "+fps", "30", invalid_values[i]);
        ExpectUnchanged();
        Invoke(CL_FpsUp_f, "-fps", "30", invalid_values[i]);
        ExpectUnchanged();
    }
    Invoke(CL_FpsDown_f, "+fps", "30", NULL);
    ExpectUnchanged();
    Invoke(CL_FpsUp_f, "-fps", NULL, NULL);
    ExpectUnchanged();
    Invoke(CL_FpsDown_f, "+fps", "1", "2147483647");
    assert(writes == 1 && retimes == 1 && !messages);
    assert(!strcmp(Cvar_VariableString("cl_maxfps"), "1"));
    Invoke(CL_FpsUp_f, "-fps", "1", "2147483647");
    assert(writes == 1 && retimes == 1 && !messages);
    assert(!strcmp(Cvar_VariableString("cl_maxfps"), "2147483647"));
    Invoke(CL_FpsShortcut_f, "f20", NULL, NULL);
    assert(writes == 1 && retimes == 1 && !strcmp(Cvar_VariableString("cl_maxfps"), "20"));
    Invoke(CL_FpsShortcut_f, "f120", NULL, NULL);
    assert(writes == 1 && retimes == 1 && !strcmp(Cvar_VariableString("cl_maxfps"), "120"));
}

int main(void)
{
    TestDefaults();
    TestSlots();
    TestPairsAndShortcuts();
    puts("fps-commands: defaults, saved slots, strict inputs and frame updates passed");
    return 0;
}
