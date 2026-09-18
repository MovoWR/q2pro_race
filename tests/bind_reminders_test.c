/* Exercise live binding reminders without executing commands or changing bindings. */
#undef NDEBUG
#include "../src/client/bind_reminders.c"
#include <assert.h>

scr_t scr;
static struct {
    cvar_t var;
    char name[64], text[2048], default_text[2048];
} test_vars[48];
static int var_count;
static bool drawing, hidden;
static int begin_count, end_count, clear_count;
static int offset_x, offset_y, submitted_count;
static size_t max_submitted_length;
static float current_alpha;
static uint32_t current_color;
static const char *bindings[256], *labels[256];
static bool key_down[256];
static const char *current_fps;
static struct {
    const char *name, *command;
} aliases[32];
static int alias_count;
static int binding_lookups, binding_enumerations, alias_lookups;
static cvar_t item_visible;
static hud_layout_item_t layout_item;
static struct {
    int x, y;
    float alpha;
    char text[512];
} text_draws[600];
static int text_count;
static bind_reminder_row_t collected_rows[BIND_REMINDER_KEYS + BIND_REMINDER_COUNT + BIND_REMINDER_EXTENDED_COUNT];
static int row_count;
static struct {
    bool fill;
    int x, y, w, h;
    uint32_t color;
    float alpha;
    char text[512];
} primitives[2048];
static int primitive_count, fill_count;

cvar_t *Cvar_FindVar(const char *name)
{
    assert(!drawing);
    for (int i = 0; i < var_count; i++) {
        if (!strcmp(test_vars[i].name, name))
            return &test_vars[i].var;
    }
    return NULL;
}

cvar_t *Cvar_Get(const char *name, const char *value, int flags)
{
    assert(!drawing);
    for (int i = 0; i < var_count; i++) {
        if (!strcmp(test_vars[i].name, name))
            return &test_vars[i].var;
    }
    assert(var_count < q_countof(test_vars));
    assert(flags & CVAR_ARCHIVE);
    int index = var_count++;
    assert(strlen(name) < sizeof(test_vars[index].name));
    assert(strlen(value) < sizeof(test_vars[index].text));
    strcpy(test_vars[index].name, name);
    strcpy(test_vars[index].text, value);
    strcpy(test_vars[index].default_text, value);
    test_vars[index].var = (cvar_t) {
        .name = test_vars[index].name,
        .string = test_vars[index].text,
        .default_string = test_vars[index].default_text,
        .integer = atoi(value), .value = strtof(value, NULL), .flags = flags
    };
    return &test_vars[index].var;
}

/* Reminder rendering has no authority to write cvars, bindings or commands. */
cvar_t *Cvar_Set(const char *name, const char *value)
{
    abort();
}

void Cvar_SetByVar(cvar_t *var, const char *value, from_t from)
{
    assert(!drawing && from == FROM_CODE);
    assert(strlen(value) < sizeof(test_vars[0].text));
    strcpy(var->string, value);
    var->value = strtof(value, NULL);
    var->integer = atoi(value);
}

void Key_SetBinding(int key, const char *binding)
{
    abort();
}

void Cbuf_AddText(cmdbuf_t *buf, const char *text)
{
    abort();
}

void Cmd_ExecuteString(cmdbuf_t *buf, const char *text)
{
    abort();
}

void Cmd_TokenizeString(const char *text, bool expand)
{
    abort();
}

char *Cmd_MacroExpandString(const char *text, bool alias_hack)
{
    abort();
}

const char *Key_BindingForKey(int key)
{
    assert(drawing);
    binding_lookups++;
    return key >= 0 && key < q_countof(bindings) && bindings[key] ? bindings[key] : "";
}

int Key_IsDown(int key)
{
    assert(drawing);
    assert(key >= 0 && key < q_countof(key_down));
    return key_down[key];
}

bool Cmd_Exists(const char *name)
{
    static const char *const commands[] = {
        "set", "seta", "setu", "sets", "toggle", "inc", "dec", "creset",
        "+fps", "-fps", "+fps_hold", "-fps_hold", "echo", "say", "alias",
        "bind", "unbind", "exec", "if", "wait", "store", "recall", "reset"
    };
    assert(drawing);
    for (size_t i = 0; i < q_countof(commands); i++) {
        if (!strcmp(name, commands[i]))
            return true;
    }
    /* Match the actual registered f20..f120 names, not numeric prefixes. */
    for (int fps = 20; fps <= 120; fps++) {
        char command[16];
        snprintf(command, sizeof(command), "f%d", fps);
        if (!strcmp(name, command))
            return true;
    }
    return false;
}

char *Cmd_AliasCommand(const char *name)
{
    assert(drawing);
    alias_lookups++;
    for (int i = 0; i < alias_count; i++) {
        if (!strcmp(name, aliases[i].name))
            return (char *)aliases[i].command;
    }
    return NULL;
}

const char *Cvar_VariableString(const char *name)
{
    assert(drawing);
    if (!strcmp(name, "cl_maxfps"))
        return current_fps;
    for (int i = 0; i < var_count; i++) {
        if (!strcmp(name, test_vars[i].name))
            return test_vars[i].text;
    }
    return "";
}

int Key_EnumBindings(int start, const char *command)
{
    assert(drawing);
    binding_enumerations++;
    for (int key = max(start, 0); key < q_countof(bindings); key++) {
        if (bindings[key] && !Q_stricmp(bindings[key], command))
            return key;
    }
    return -1;
}

const char *Key_KeynumToLabel(int key)
{
    /* Real layout-aware labels may share a static return buffer. */
    static char label[128];
    assert(key >= 0 && key < q_countof(labels));
    snprintf(label, sizeof(label), "%s", labels[key] ? labels[key] : "?");
    return label;
}

const hud_layout_item_t *HUD_LayoutItem(int id)
{
    assert(id == HL_BIND_REMINDERS && drawing);
    return &layout_item;
}

void HUD_LayoutBegin(int id)
{
    assert(id == HL_BIND_REMINDERS && drawing);
    begin_count++;
}

void HUD_LayoutEnd(void)
{
    end_count++;
}

void R_SetAlpha(float alpha)
{
    assert(isfinite(alpha) && alpha >= 0 && alpha <= 1);
    current_alpha = alpha;
}

void R_SetColor(uint32_t color)
{
    current_color = color;
    current_alpha = (color >> 24) / 255.0f;
}

void R_ClearColor(void)
{
    current_color = MakeColor(255, 255, 255, 255);
    current_alpha = 1;
    clear_count++;
}

static void capture_primitive(bool fill, int x, int y, int w, int h,
                              uint32_t color, float alpha, const char *text)
{
    assert(isfinite(alpha) && alpha >= 0 && alpha <= 1);
    assert(w > 0 && h > 0 && primitive_count < q_countof(primitives));
    int index = primitive_count++;
    primitives[index].fill = fill;
    primitives[index].x = x;
    primitives[index].y = y;
    primitives[index].w = w;
    primitives[index].h = h;
    primitives[index].color = color;
    primitives[index].alpha = alpha;
    Q_strlcpy(primitives[index].text, text ? text : "", sizeof(primitives[index].text));
}

void R_DrawFill32(int x, int y, int w, int h, uint32_t color)
{
    assert(drawing && begin_count > end_count && w > 0 && h > 0);
    /* Explicit fill RGBA is independent of the current text alpha. */
    x += offset_x;
    y += offset_y;
    int right = min(x + w, scr.hud_width);
    int bottom = min(y + h, scr.hud_height);
    x = max(x, 0);
    y = max(y, 0);
    if (hidden || right <= x || bottom <= y)
        return;
    fill_count++;
    capture_primitive(true, x, y, right - x, bottom - y,
                      color, (color >> 24) / 255.0f, NULL);
}

int SCR_DrawStringEx(int x, int y, int flags, size_t maxchars, const char *text, qhandle_t font)
{
    assert(drawing && begin_count > end_count);
    size_t length = min(strlen(text), maxchars);
    int end = x + (int)length * CHAR_WIDTH;
    submitted_count++;
    max_submitted_length = max(max_submitted_length, length);
    assert(length < sizeof(text_draws[0].text));

    /* Native rendering applies HUD translation before viewport clipping. */
    x += offset_x;
    y += offset_y;
    if (hidden || y >= scr.hud_height || y + CHAR_HEIGHT <= 0)
        return end;
    size_t first = 0, last = length;
    while (first < last && x + (int)(first + 1) * CHAR_WIDTH <= 0)
        first++;
    while (last > first && x + (int)(last - 1) * CHAR_WIDTH >= scr.hud_width)
        last--;
    if (first == last)
        return end;
    assert(text_count < q_countof(text_draws));
    text_draws[text_count].x = x + (int)first * CHAR_WIDTH;
    text_draws[text_count].y = y;
    text_draws[text_count].alpha = current_alpha;
    memcpy(text_draws[text_count].text, text + first, last - first);
    text_draws[text_count].text[last - first] = 0;
    capture_primitive(false, text_draws[text_count].x, y,
                      (int)(last - first) * CHAR_WIDTH, CHAR_HEIGHT,
                      current_color, current_alpha, text_draws[text_count].text);
    text_count++;
    return end;
}

void Com_Error(error_type_t code, const char *format, ...)
{
    va_list args;
    va_start(args, format);
    vfprintf(stderr, format, args);
    va_end(args);
    abort();
}

void Com_LPrintf(print_type_t type, const char *format, ...)
{
    abort();
}

static cvar_t *setting(const char *name)
{
    for (int i = 0; i < var_count; i++) {
        if (!strcmp(test_vars[i].name, name))
            return &test_vars[i].var;
    }
    assert(!"Unknown reminder setting");
    return NULL;
}

static void set_text(const char *name, const char *value)
{
    cvar_t *var = setting(name);
    assert(strlen(value) < sizeof(test_vars[0].text));
    for (int i = 0; i < var_count; i++) {
        if (&test_vars[i].var == var) {
            strcpy(test_vars[i].text, value);
            break;
        }
    }
    var->value = strtof(value, NULL);
    var->integer = atoi(value);
}

static void setup(void)
{
    drawing = hidden = false;
    current_fps = "60";
    memset(test_vars, 0, sizeof(test_vars));
    memset(key_down, 0, sizeof(key_down));
    for (size_t i = 0; i < q_countof(bindings); i++)
        bindings[i] = labels[i] = NULL;
    memset(&scr, 0, sizeof(scr));
    var_count = 0;
    alias_count = 0;
    item_visible = (cvar_t) { .value = 1, .integer = 1 };
    layout_item = (hud_layout_item_t) { .visible = &item_visible };
    offset_x = offset_y = 0;
    scr.hud_width = 640;
    scr.hud_height = 480;
    SCR_BindRemindersInit();
}

static void setup_manual(void)
{
    setup();
    assert(setting("scr_bindreminders_fps")->integer == 1);
    set_text("scr_bindreminders_fps", "0");
}

static void draw(float alpha)
{
    text_count = begin_count = end_count = clear_count = 0;
    primitive_count = fill_count = 0;
    binding_lookups = binding_enumerations = alias_lookups = 0;
    submitted_count = 0;
    max_submitted_length = 0;
    current_alpha = alpha;
    current_color = MakeColor(255, 255, 255, 255);
    drawing = true;
    SCR_DrawBindReminders(alpha);
    drawing = false;
    assert(begin_count == end_count);
    float restored = isfinite(alpha) ? Q_clipf(alpha, 0, 1) : 0;
    assert(fabsf(current_alpha - restored) < 0.0001f);
}


static void collect(void)
{
    binding_lookups = binding_enumerations = alias_lookups = 0;
    drawing = true;
    row_count = SCR_CollectBindReminders(collected_rows);
    drawing = false;
    assert(row_count >= 0 && row_count <= q_countof(collected_rows));
}

static bool has_text(const char *text)
{
    for (int i = 0; i < row_count; i++) {
        if (strstr(collected_rows[i].keys, text) || strstr(collected_rows[i].caption, text))
            return true;
    }
    return false;
}

static int rendered_text(const char *text)
{
    for (int i = 0; i < text_count; i++)
        if (!strcmp(text_draws[i].text, text))
            return i;
    return -1;
}

static void assert_text_does_not_overlap(void)
{
    for (int i = 0; i < text_count; i++) {
        for (int j = i + 1; j < text_count; j++) {
            int right_i = text_draws[i].x + (int)strlen(text_draws[i].text) * CHAR_WIDTH;
            int right_j = text_draws[j].x + (int)strlen(text_draws[j].text) * CHAR_WIDTH;
            assert(right_i <= text_draws[j].x || right_j <= text_draws[i].x ||
                   text_draws[i].y + CHAR_HEIGHT <= text_draws[j].y ||
                   text_draws[j].y + CHAR_HEIGHT <= text_draws[i].y);
        }
    }
}



static void test_defaults_and_unbound_rows(void)
{
    setup_manual();
    assert(var_count == 22);
    assert(fabsf(setting("scr_bindreminders_alpha")->value - 0.6f) < 0.0001f);
    assert(!strcmp(setting("scr_bindreminders_command_1")->string, "toggle cl_maxfps 30 120"));
    assert(!strcmp(setting("scr_bindreminders_command_8")->string, ""));
    collect();
    assert(row_count == 4);
    assert(has_text("30 / 120") && has_text("Store"));
    assert(has_text("Recall") && has_text("Reset"));
    for (int i = 0; i < row_count; i++) {
        assert(!strcmp(collected_rows[i].keys, "--"));
        assert(collected_rows[i].fps == (i == 0));
    }
    draw(0.7f);
    assert(text_count == 8 && begin_count == 1 && end_count == 1 && clear_count == 1);
    assert(rendered_text("FPS") < 0 && rendered_text("ACTIONS") < 0);
    assert(rendered_text("--") >= 0 && fill_count == 10);
    assert(fabsf(text_draws[rendered_text("30 / 120")].alpha - 0.42f) < 0.0001f);
    assert_text_does_not_overlap();
}

static void test_live_bindings_and_layout_labels(void)
{
    setup_manual();
    bindings[10] = "TOGGLE CL_MAXFPS 30 120";
    labels[10] = "Z";
    collect();
    assert(has_text("Z"));

    bindings[10] = NULL;
    bindings[20] = "toggle cl_maxfps 30 120";
    labels[20] = "A";
    collect();
    assert(has_text("A") && !has_text("Z"));

    bindings[30] = bindings[40] = bindings[20];
    labels[30] = "B";
    labels[40] = "C";
    collect();
    assert(has_text("A / B +") && !has_text("C"));

    /* Partial or compound bindings must not be mistaken for an exact action. */
    bindings[20] = "toggle cl_maxfps 30 120; say changed";
    bindings[30] = bindings[40] = NULL;
    collect();
    assert(!strcmp(collected_rows[0].keys, "--"));
}


static void test_configured_rows_and_captions(void)
{
    setup_manual();
    set_text("scr_bindreminders_command_1", "");
    set_text("scr_bindreminders_label_2", "");
    set_text("scr_bindreminders_label_3", "Save\npoint\tA\001");
    set_text("scr_bindreminders_command_5", "say hello");
    set_text("scr_bindreminders_label_5", "Greeting");
    bindings[1] = "say hello";
    labels[1] = "G";
    collect();
    assert(row_count == 4 && !has_text("30 / 120"));
    assert(has_text("store") && has_text("Save\npoint\tA\001") && has_text("Greeting"));
    draw(1);
    assert(rendered_text("FPS") < 0 && rendered_text("ACTIONS") < 0);
    assert(rendered_text("Save point A ") >= 0);
    assert(!strcmp(setting("scr_bindreminders_label_3")->string, "Save\npoint\tA\001"));
    for (int i = 0; i < text_count; i++)
        for (const unsigned char *p = (const unsigned char *)text_draws[i].text; *p; p++)
            assert(*p >= 32 && *p != 127);
    assert_text_does_not_overlap();
    for (int i = 1; i <= 8; i++) {
        char name[64];
        snprintf(name, sizeof(name), "scr_bindreminders_command_%d", i);
        set_text(name, "");
    }
    set_text("scr_bindreminders_store", "0");
    collect();
    assert(row_count == 0);
    draw(1);
    assert(text_count == 0 && fill_count == 0 && primitive_count == 0);
}


static void test_alpha_and_hidden_groups(void)
{
    setup_manual();
    cvar_t *alpha = setting("scr_bindreminders_alpha");
    char original[32];
    Q_strlcpy(original, alpha->string, sizeof(original));
    const float values[] = { -0.5f, 2.0f, NAN, INFINITY };
    const float expected[] = { 0, 0.6f, 0, 0 };
    for (size_t i = 0; i < q_countof(values); i++) {
        alpha->value = values[i];
        draw(0.6f);
        for (int j = 0; j < text_count; j++)
            assert(fabsf(text_draws[j].alpha - expected[i]) < 0.0001f);
        for (int j = 0; j < primitive_count; j++)
            assert(isfinite(primitives[j].alpha) && primitives[j].alpha <= expected[i]);
        assert(!strcmp(alpha->string, original));
        assert(isnan(values[i]) ? isnan(alpha->value) : alpha->value == values[i]);
    }
    alpha->value = 0.6f;
    draw(NAN);
    assert(current_alpha == 0);
    hidden = true;
    draw(0.6f);
    assert(text_count == 0 && primitive_count == 0 && begin_count == 1 && end_count == 1);
}



static void test_viewport_and_text_limits(void)
{
    setup_manual();
    char long_caption[1000], long_key[128];
    memset(long_caption, 'x', sizeof(long_caption) - 1);
    long_caption[sizeof(long_caption) - 1] = 0;
    memset(long_key, 'K', sizeof(long_key) - 1);
    long_key[sizeof(long_key) - 1] = 0;
    set_text("scr_bindreminders_label_1", long_caption);
    bindings[10] = "toggle cl_maxfps 30 120";
    labels[10] = long_key;
    collect();
    assert(strlen(collected_rows[0].caption) == 127 && strlen(collected_rows[0].keys) == 47);
    draw(1);
    assert(rendered_text("KKKKK...") >= 0);
    assert(rendered_text("xxxxxxxxxxxxxxx...") >= 0);
    assert(max_submitted_length == 18);
    assert_text_does_not_overlap();
    assert(!strcmp(setting("scr_bindreminders_label_1")->string, long_caption));
    assert(!strcmp(labels[10], long_key));
    scr.hud_width = 8;
    draw(1);
    assert(text_count == 0 && fill_count > 0 && submitted_count == 8);
    for (int i = 0; i < primitive_count; i++)
        assert(primitives[i].x >= 0 && primitives[i].x + primitives[i].w <= scr.hud_width);
    scr.hud_width = 640;
    scr.hud_height = 1;
    draw(1);
    assert(text_count == 0 && fill_count > 0 && submitted_count == 8);
    for (int i = 0; i < primitive_count; i++)
        assert(primitives[i].y >= 0 && primitives[i].y + primitives[i].h <= scr.hud_height);
}


static void test_translation_precedes_clipping(void)
{
    setup_manual();
    for (int i = 1; i <= 8; i++) {
        char name[64], value[16];
        snprintf(name, sizeof(name), "scr_bindreminders_command_%d", i);
        snprintf(value, sizeof(value), "action%d", i);
        set_text(name, value);
        snprintf(name, sizeof(name), "scr_bindreminders_label_%d", i);
        set_text(name, value);
    }
    scr.hud_height = 60;
    draw(1);
    assert(text_count == 8 && rendered_text("action8") < 0 && submitted_count == 16);
    offset_y = -56;
    draw(1);
    assert(text_count == 8 && submitted_count == 16);
    assert(rendered_text("action1") < 0 && rendered_text("action8") >= 0);
    assert(text_draws[rendered_text("action5")].y == 6);
    assert(text_draws[rendered_text("action8")].y == 48);
    assert_text_does_not_overlap();
    offset_y = 0;
    scr.hud_height = 180;
    draw(1);
    assert(text_count == 16 && submitted_count == 16);

    setup_manual();
    set_text("scr_bindreminders_label_1", "0123456789TAIL");
    draw(1);
    int caption = rendered_text("0123456789TAIL");
    assert(caption >= 0);
    int caption_relative_x = text_draws[caption].x - primitives[0].x;
    scr.hud_width = 80;
    draw(1);
    assert(rendered_text("TAIL") < 0);
    offset_x = -primitives[0].x - caption_relative_x - 10 * CHAR_WIDTH;
    draw(1);
    assert(rendered_text("TAIL") >= 0);
}

static const char *key_row(const char *label)
{
    for (int i = 0; i < row_count; i++)
        if (!strcmp(collected_rows[i].keys, label))
            return collected_rows[i].caption;
    return NULL;
}

static void add_alias(const char *name, const char *command)
{
    assert(alias_count < q_countof(aliases));
    aliases[alias_count].name = name;
    aliases[alias_count++].command = command;
}

static void test_automatic_default_and_live_rebind(void)
{
    setup();
    assert(setting("scr_bindreminders_fps")->integer == 1);
    collect();
    assert(row_count == 3 && !has_text("30 / 120"));

    bindings[10] = "f20";
    labels[10] = "AUTO_A";
    collect();
    assert(row_count == 4 && strstr(key_row("AUTO_A"), "20 FPS"));
    bindings[10] = "f120";
    collect();
    assert(row_count == 4 && strstr(key_row("AUTO_A"), "120 FPS"));
    bindings[20] = bindings[10];
    labels[20] = "AUTO_B";
    collect();
    assert(row_count == 5 && key_row("AUTO_A") && key_row("AUTO_B"));
    bindings[10] = bindings[20] = NULL;
    collect();
    assert(row_count == 3 && !key_row("AUTO_A") && !key_row("AUTO_B"));
    set_text("scr_bindreminders_fps", "0");
    collect();
    assert(row_count == 4 && has_text("30 / 120"));
}

static void test_every_registered_shortcut(void)
{
    setup();
    char commands[101][8], keynames[101][16];
    scr.hud_height = 2048;
    for (int fps = 20; fps <= 120; fps++) {
        int index = fps - 20;
        snprintf(commands[index], sizeof(commands[index]), "f%d", fps);
        snprintf(keynames[index], sizeof(keynames[index]), "AUTO_%03d", fps);
        bindings[fps] = commands[index];
        labels[fps] = keynames[index];
    }
    collect();
    assert(row_count == 104);
    for (int i = 0; i < q_countof(commands); i++)
        assert(key_row(keynames[i]));

    const char *false_positives[] = {
        "f19", "f121", "f125", "f020", "f20junk", "F20", "r_maxfps 30",
        "echo f20", "say \"f20; cl_maxfps 30\"", "bind x f20", "alias future f20",
        "cl_maxfps", "set cl_maxfps", "+fps 30", "+fps_hold 0", "+fps_hold 13",
        "toggle r_maxfps 30 120", "cl_maxfps_extra 30", "CL_MAXFPS 30"
    };
    for (size_t i = 0; i < q_countof(false_positives); i++) {
        setup();
        bindings[1] = false_positives[i];
        labels[1] = "REJECT";
        collect();
        if (key_row("REJECT"))
            fprintf(stderr, "Unexpected FPS discovery: %s\n", false_positives[i]);
        assert(row_count == 3 && !key_row("REJECT"));
    }
}

static void test_direct_fps_patterns(void)
{
    static const struct {
        const char *command, *caption;
    } cases[] = {
        { "cl_maxfps 30", "30 FPS" },
        { "  cl_maxfps \"40\"  ", "40 FPS" },
        { "set cl_maxfps 50", "50 FPS" },
        { "seta cl_maxfps 60", "60 FPS" },
        { "setu cl_maxfps 70", "70 FPS" },
        { "sets cl_maxfps 80", "80 FPS" },
        { "toggle cl_maxfps 20 40 60 80 120", "20 / 40 / 60 / 80 / 120" },
        { "toggle cl_maxfps", "FPS" },
        { "inc cl_maxfps", "+1 FPS" },
        { "inc cl_maxfps 10", "+10 FPS" },
        { "dec cl_maxfps", "-1 FPS" },
        { "dec cl_maxfps 5", "-5 FPS" },
        { "creset cl_maxfps", "FPS reset" },
        { "+fps 30 120", "30 > 120" },
        { "-fps 30 120", "120 FPS" },
        { "+fps $hold $release", "$hold > $release" },
        { "cl_maxfps $chosen", "$chosen FPS" },
        { "+fps_hold $slot", "FPS" },
    };
    for (size_t i = 0; i < q_countof(cases); i++) {
        setup();
        bindings[1] = cases[i].command;
        labels[1] = "MATCH";
        collect();
        const char *row = key_row("MATCH");
        if (!row || !strstr(row, cases[i].caption))
            fprintf(stderr, "Missing FPS discovery/caption: %s\n", cases[i].command);
        assert(row_count == 4 && row && strstr(row, cases[i].caption));
    }
}

static void test_empty_toggle_captions(void)
{
    static const char *const commands[] = {
        "toggle cl_maxfps \"\"",
        "toggle cl_maxfps \"\" \"\"",
        "toggle cl_maxfps \"   \"",
        "toggle cl_maxfps \" \t \" \"  \""
    };
    for (size_t i = 0; i < q_countof(commands); i++) {
        setup();
        bindings[1] = commands[i];
        labels[1] = "EMPTY";
        collect();
        const char *caption = key_row("EMPTY");
        assert(row_count == 4 && caption && !strcmp(caption, "FPS"));
        draw(1);
        assert(rendered_text("EMPTY") >= 0 && rendered_text("FPS") >= 0);
    }
}

static void test_live_hold_slot_values(void)
{
    setup();
    char commands[12][32], keynames[12][16];
    for (int slot = 1; slot <= 12; slot++) {
        char name[64], value[16];
        snprintf(name, sizeof(name), "fps_hold_%d", slot);
        snprintf(value, sizeof(value), "%d", 20 + slot);
        Cvar_Get(name, value, CVAR_ARCHIVE);
        snprintf(name, sizeof(name), "fps_release_%d", slot);
        Cvar_Get(name, "120", CVAR_ARCHIVE);
        snprintf(commands[slot - 1], sizeof(commands[0]), "+fps_hold %d", slot);
        snprintf(keynames[slot - 1], sizeof(keynames[0]), "SLOT_%02d", slot);
        bindings[slot] = commands[slot - 1];
        labels[slot] = keynames[slot - 1];
    }
    collect();
    assert(row_count == 15);
    assert(strstr(key_row("SLOT_01"), "21 > 120"));
    assert(strstr(key_row("SLOT_12"), "32 > 120"));
    set_text("fps_hold_12", "60");
    set_text("fps_release_12", "90");
    collect();
    assert(strstr(key_row("SLOT_12"), "60 > 90"));
    bindings[12] = "-fps_hold 12";
    collect();
    assert(!strcmp(key_row("SLOT_12"), "90 FPS"));
}

static void test_aliases_and_scripts_without_execution(void)
{
    setup();
    add_alias("slow", "f20");
    add_alias("chain", "slow");
    add_alias("parameter", "cl_maxfps $1");
    add_alias("+releaseonly", "echo ready");
    add_alias("-releaseonly", "f120");
    add_alias("loop_a", "loop_b");
    add_alias("loop_b", "loop_a");
    add_alias("alias_definition", "alias later \"f20\"");
    add_alias("chat_only", "say \"f20; cl_maxfps 30\"");
    add_alias("echo", "f20"); /* Registered command takes precedence. */
    static const struct {
        const char *command;
        bool fps;
    } cases[] = { { "slow", true },
                  { "chain", true },
                  { "parameter 60", true },
                  { "+releaseonly", true },
                  { "loop_a", false },
                  { "Slow", false },
                  { "alias_definition", false },
                  { "chat_only", false },
                  { "echo", false },
                  { "echo \"f120; cl_maxfps 30\"; f20", true },
                  { "echo ready\nf20", true },
                  { "alias later \"echo f20\"; f30", true },
                  { "f20; wait; f120", true },
                  { "say \"f20; cl_maxfps 30\"", false },
                  { "echo \"unfinished; f20", false } };
    for (size_t i = 0; i < q_countof(cases); i++) {
        bindings[1] = cases[i].command;
        labels[1] = "SCRIPT";
        collect();
        if (!!key_row("SCRIPT") != cases[i].fps)
            fprintf(stderr, "Unexpected script discovery: %s\n", cases[i].command);
        assert(!!key_row("SCRIPT") == cases[i].fps);
        assert(row_count == (cases[i].fps ? 4 : 3));
    }
}

static void test_conditional_scripts_without_evaluation(void)
{
    setup();
    add_alias("slow", "f20");
    add_alias("fast", "f120");
    add_alias("conditional", "if $cl_maxfps == 30 then slow else fast");
    add_alias("nested", "conditional");
    add_alias("conditional_chat", "if 1 == 1 \"say f20\" else \"echo cl_maxfps 30\"");
    static const struct {
        const char *command;
        bool fps;
    } cases[] = { { "if 1 == 1 then f20 else f120", true },
                  { "if 0 == 1 f20 else echo ready", true },
                  { "if 1 == 1 echo ready else f120", true },
                  { "if 1 == 1 then echo ready else f120", true },
                  { "if 1 == 1 THEN f20 ELSE f120", true },
                  { "if 1 == 1 f20", true },
                  { "if 1 == 1 \"f20\" else \"f120\"", true },
                  { "if 1 == 1 then \"echo ready; f20\" else \"echo ready\"", true },
                  { "if 1 == 1 \"echo ready\" else \"cl_maxfps 120\"", true },
                  { "if 1 == 1 slow else fast", true },
                  { "conditional", true },
                  { "nested", true },
                  { "if 1 == 1 then nested else echo ready", true },
                  { "if 1 == 1 \"say f20\" else \"echo cl_maxfps 30\"", false },
                  { "if 1 == 1 \"say f20; echo cl_maxfps 30\"", false },
                  { "if f20 == f120 say ready else echo ready", false },
                  { "if 1 == 1 echo elsewhere f20", false },
                  { "conditional_chat", false },
                  { "if", false },
                  { "if 1 == 1", false } };
    for (size_t i = 0; i < q_countof(cases); i++) {
        bindings[1] = cases[i].command;
        labels[1] = "CONDITIONAL";
        collect();
        const char *row = key_row("CONDITIONAL");
        if (!!row != cases[i].fps)
            fprintf(stderr, "Unexpected conditional discovery: %s\n", cases[i].command);
        assert(!!row == cases[i].fps);
        assert(row_count == (cases[i].fps ? 4 : 3));
        if (row)
            assert(!strcmp(row, "FPS"));
    }
}

static void test_disabled_item_skips_discovery(void)
{
    setup();
    add_alias("nested_fps", "f20");
    bindings[1] = "nested_fps";
    labels[1] = "FPS_KEY";
    bindings[2] = "store";
    labels[2] = "STORE";
    /* Visibility uses the actual float value, independently of cached integer. */
    item_visible.value = 0;
    draw(0.6f);
    assert(binding_lookups == 0 && binding_enumerations == 0 && alias_lookups == 0);
    assert(text_count == 0 && submitted_count == 0);
    assert(begin_count == 0 && end_count == 0 && clear_count == 0);
    assert(current_alpha == 0.6f);

    set_text("scr_bindreminders_fps", "0");
    draw(0.6f);
    assert(binding_lookups == 0 && binding_enumerations == 0 && alias_lookups == 0);
    assert(begin_count == 0 && clear_count == 0 && current_alpha == 0.6f);

    item_visible.value = 1;
    set_text("scr_bindreminders_fps", "1");
    draw(0.6f);
    assert(binding_lookups == 256 && binding_enumerations > 0 && alias_lookups > 0);
    assert(rendered_text("FPS_KEY") >= 0 && rendered_text("STORE") >= 0);
    assert(begin_count == 1 && end_count == 1 && clear_count == 1);
}

static void test_alias_depth_boundary(void)
{
    setup();
    char names[ALIAS_LOOP_COUNT + 1][32];
    for (int i = 0; i < q_countof(names); i++)
        snprintf(names[i], sizeof(names[i]), "depth_%d", i);
    for (int i = 0; i < q_countof(names); i++)
        add_alias(names[i], i + 1 < q_countof(names) ? names[i + 1] : "f20");
    labels[1] = "DEEP_ALIAS";

    bindings[1] = names[1]; /* Exactly the engine's 16-alias allowance. */
    collect();
    assert(key_row("DEEP_ALIAS") && row_count == 4);
    bindings[1] = names[0];
    collect();
    assert(!key_row("DEEP_ALIAS") && row_count == 3);

    aliases[ALIAS_LOOP_COUNT].command = "if 1 == 1 then f20 else echo ready";
    bindings[1] = names[1];
    collect();
    assert(key_row("DEEP_ALIAS") && row_count == 4);
}

static void test_manual_fps_suppression_and_caption_override(void)
{
    setup();
    bindings[1] = "toggle cl_maxfps 30 120";
    labels[1] = "FPS_KEY";
    collect();
    assert(row_count == 4 && strstr(key_row("FPS_KEY"), "30 / 120"));
    set_text("scr_bindreminders_label_1", "FPS 30 / 120");
    collect();
    assert(!strcmp(key_row("FPS_KEY"), "30 / 120"));
    assert(!strcmp(setting("scr_bindreminders_label_1")->string, "FPS 30 / 120"));
    set_text("scr_bindreminders_label_1", "Switch jump FPS");
    collect();
    assert(row_count == 4 && strstr(key_row("FPS_KEY"), "Switch jump FPS"));

    set_text("scr_bindreminders_command_5", "f20");
    set_text("scr_bindreminders_label_5", "Precision FPS");
    bindings[2] = "f20";
    labels[2] = "PRECISE";
    collect();
    assert(row_count == 5 && strstr(key_row("PRECISE"), "Precision FPS"));
    set_text("scr_bindreminders_label_5", "");
    collect();
    assert(row_count == 5 && strstr(key_row("PRECISE"), "20 FPS"));
    bindings[2] = NULL;
    collect();
    assert(row_count == 4 && !has_text("Precision FPS"));
    set_text("scr_bindreminders_fps", "0");
    collect();
    assert(row_count == 5 && has_text("20 FPS"));
}

static void test_truthful_fps_captions(void)
{
    setup();
    Cvar_Get("fps_hold_1", "30", CVAR_ARCHIVE);
    Cvar_Get("fps_release_1", "120", CVAR_ARCHIVE);
    add_alias("low", "f20");
    add_alias("nested_low", "low");
    add_alias("nested_hold", "+fps 30 120");
    add_alias("nested_slot", "+fps_hold 1");
    add_alias("nested_dynamic", "+fps_hold $slot");
    add_alias("+release_only", "echo ready");
    add_alias("-release_only", "f120");
    static const struct {
        const char *command, *caption;
    } cases[] = { { "low", "20 FPS" },          { "nested_low", "20 FPS" },   { "nested_hold", "30 FPS" },
                  { "nested_slot", "30 FPS" },  { "nested_dynamic", "FPS" },  { "+release_only", "120 FPS up" },
                  { "-fps 30 120", "120 FPS" }, { "-fps_hold 1", "120 FPS" }, { "+fps_hold $slot", "FPS" },
                  { "-fps_hold $slot", "FPS" }, { "f20; wait; f120", "FPS" }, { "if 1 == 1 f20 else f120", "FPS" } };
    labels[1] = "CAPTION";
    for (size_t i = 0; i < q_countof(cases); i++) {
        bindings[1] = cases[i].command;
        collect();
        const char *caption = key_row("CAPTION");
        assert(caption && !strcmp(caption, cases[i].caption));
    }
}

static void representative_bindings(void)
{
    setup();
    bindings[1] = "f20";
    labels[1] = "1";
    bindings[2] = "toggle cl_maxfps 30 120";
    labels[2] = "MOUSE4";
    bindings[3] = "+fps 30 120";
    labels[3] = "MOUSE5";
    bindings[4] = "store";
    labels[4] = "F";
    bindings[5] = "recall";
    labels[5] = "R";
    bindings[6] = "reset";
    labels[6] = "T";
}

static void draw_editor_preview(void)
{
    text_count = begin_count = end_count = clear_count = 0;
    primitive_count = fill_count = submitted_count = 0;
    binding_lookups = binding_enumerations = alias_lookups = 0;
    current_color = MakeColor(255, 255, 255, 255);
    current_alpha = 1;
    drawing = true;
    SCR_PreviewBindReminders();
    drawing = false;
    assert(begin_count == 1 && end_count == 1 && clear_count == 1);
    assert(current_alpha == 1);
}

static void test_editor_alignment_geometry(void)
{
    const int counts[] = { 7, 3, 16 };

    for (int i = 0; i < q_countof(counts); i++) {
        setup();
        scr.hud_height = 360;
        for (int key = 0; key < counts[i] - 3; key++) {
            bindings[key] = "f20";
            labels[key] = i == 2 ? "RIGHTALT" : "MOUSE4";
        }
        draw(1);
        assert(text_count == 2 * counts[i]);
        int live_x = primitives[0].x, live_y = primitives[0].y;
        int live_width = primitives[0].w, live_height = primitives[0].h;

        draw_editor_preview();
        assert(primitives[0].h == live_height);
        assert(primitives[0].w == live_width);
        assert(primitives[0].x == live_x && primitives[0].y == live_y);
        assert(text_count == 2 * counts[i]);

        /* Apply the editor's left/bottom alignment offsets to the live panel. */
        offset_x = -primitives[0].x;
        offset_y = scr.hud_height - primitives[0].y - primitives[0].h;
        draw(1);
        assert(primitives[0].x == 0 && primitives[0].w == live_width);
        assert(primitives[0].h == live_height);
        assert(primitives[0].y + primitives[0].h == scr.hud_height);
        assert(text_count == 2 * counts[i]);
        for (int row = 0; row < text_count; row++) {
            assert(text_draws[row].x >= 0);
            assert(text_draws[row].y + CHAR_HEIGHT <= scr.hud_height);
        }
    }
}

static void test_empty_editor_preview(void)
{
    setup();
    for (int i = 0; i < BIND_REMINDER_COUNT; i++)
        set_text(reminder_commands[i]->name, "");
    set_text("scr_bindreminders_store", "0");
    draw(1);
    assert(primitive_count == 0);

    draw_editor_preview();
    assert(text_count == 12 && primitives[0].w == 110 && primitives[0].h == 90);
    assert(rendered_text("20 FPS") >= 0 && rendered_text("Store") >= 0);
    assert_text_does_not_overlap();
}


static void test_panel_and_editor_preview(void)
{
    representative_bindings();
    draw(0.7f);
    assert(text_count == 12 && fill_count == 14 && primitive_count == 26);
    assert(primitives[0].fill && primitives[0].color == MakeColor(14, 18, 23, (int)(0.42f * 204)));
    assert(primitives[0].w == 110 && primitives[0].h == 90);
    assert(rendered_text("FPS") < 0 && rendered_text("ACTIONS") < 0);
    int first = rendered_text("20 FPS");
    int second = rendered_text("30 / 120");
    int third = rendered_text("30 > 120");
    assert(first >= 0 && second >= 0 && third >= 0);
    assert(text_draws[first].y == Q_rint(scr.hud_height * 0.525f) + 6);
    assert(text_draws[second].y - text_draws[first].y == 14);
    assert(text_draws[third].y - text_draws[second].y == 14);
    assert(rendered_text("M4") >= 0 && rendered_text("M5") >= 0);
    assert_text_does_not_overlap();
    for (int i = 0; i < text_count; i++) {
        assert(text_draws[i].x >= primitives[0].x);
        assert(text_draws[i].x + (int)strlen(text_draws[i].text) * CHAR_WIDTH <=
               primitives[0].x + primitives[0].w);
        assert(text_draws[i].y >= primitives[0].y);
        assert(text_draws[i].y + CHAR_HEIGHT <= primitives[0].y + primitives[0].h);
    }
    /* Disabling the live panel still allows an accurate editor preview. */
    item_visible.value = 0;
    draw_editor_preview();
    assert(text_count == 12 && fill_count == 14 && primitive_count == 26);
    assert(binding_lookups == BIND_REMINDER_KEYS);
    assert(primitives[0].w == 110 && primitives[0].h == 90);
    assert(primitives[0].x + primitives[0].w == scr.hud_width - 8);
    assert(primitives[0].y == Q_rint(scr.hud_height * 0.525f));
    assert(rendered_text("30 / 120") >= 0 && rendered_text("M4") >= 0);
    assert_text_does_not_overlap();
}

static void test_short_mouse_labels_and_row_order(void)
{
    setup();
    char names[8][16];
    for (int i = 1; i <= 8; i++) {
        snprintf(names[i - 1], sizeof(names[0]), "MOUSE%d", i);
        bindings[i] = "f20";
        labels[i] = names[i - 1];
    }
    bindings[9] = bindings[10] = "f20";
    labels[9] = "MWHEELUP";
    labels[10] = "MWHEELDOWN";
    collect();
    assert(row_count == 13 && key_row("MOUSE1") && key_row("MWHEELDOWN"));
    draw(1);
    for (int i = 1; i <= 8; i++) {
        char expected[8];
        snprintf(expected, sizeof(expected), "M%d", i);
        assert(rendered_text(expected) >= 0 && rendered_text(names[i - 1]) < 0);
        assert(!strcmp(labels[i], names[i - 1]));
    }
    assert(rendered_text("MWUP") >= 0 && rendered_text("MWDN") >= 0);
    assert(!strcmp(labels[9], "MWHEELUP") && !strcmp(labels[10], "MWHEELDOWN"));
    assert_text_does_not_overlap();

    const char *unchanged[] = { "MOUSE9", "MOUSE12", "MOUSE1X", "MOUSEPAD" };
    for (size_t i = 0; i < q_countof(unchanged); i++) {
        setup();
        bindings[1] = "f20";
        labels[1] = unchanged[i];
        draw(1);
        assert(rendered_text(unchanged[i]) >= 0);
    }

    /* Shorten each label before joining a manual action's multiple bindings. */
    setup_manual();
    bindings[1] = bindings[2] = "store";
    labels[1] = "MOUSE4";
    labels[2] = "MOUSE5";
    collect();
    assert(row_count == 4 && key_row("M4 / M5"));
    assert(!strcmp(key_row("M4 / M5"), "Store"));
    assert(!has_text("MOUSE"));
    draw(1);
    assert(rendered_text("M4 / M5") >= 0);
    for (int i = 0; i < text_count; i++)
        assert(!strstr(text_draws[i].text, "MOUSE"));
    assert(!strcmp(labels[1], "MOUSE4") && !strcmp(labels[2], "MOUSE5"));
    assert_text_does_not_overlap();

    /* Render collection order directly, even when an action precedes FPS. */
    setup_manual();
    set_text("scr_bindreminders_command_1", "store");
    set_text("scr_bindreminders_label_1", "First action");
    set_text("scr_bindreminders_command_2", "f20");
    set_text("scr_bindreminders_label_2", "");
    set_text("scr_bindreminders_command_3", "");
    set_text("scr_bindreminders_command_4", "");
    draw(1);
    int action = rendered_text("First action"), fps = rendered_text("20 FPS");
    assert(action >= 0 && fps >= 0 && text_draws[fps].y - text_draws[action].y == 14);
    assert(text_count == 4 && fill_count == 6);
    assert_text_does_not_overlap();
}

static bool active_key(const char *key)
{
    for (int i = 0; i < row_count; i++)
        if (!strcmp(collected_rows[i].keys, key))
            return collected_rows[i].active;
    assert(!"Missing collected key");
    return false;
}

static void test_raw_button_release_metadata(void)
{
    setup();
    Cvar_Get("fps_hold_1", "30", CVAR_ARCHIVE);
    Cvar_Get("fps_release_1", "120", CVAR_ARCHIVE);
    add_alias("hold", "+fps 30 120");
    add_alias("hold_chain", "hold");
    add_alias("slot", "+fps_hold 1");
    static const struct {
        const char *command, *caption;
        bool down_active, up_active;
    } cases[] = {
        { "+fps 30 120", "30 > 120", true, true },
        { "  +fps 30 120", "30 FPS", true, false },
        { "\t+fps 30 120", "30 FPS", true, false },
        { "\"+fps\" 30 120", "30 FPS", true, false },
        { "+fps_hold 1", "30 > 120", true, true },
        { "  +fps_hold 1", "30 FPS", true, false },
        { "\t+fps_hold 1", "30 FPS", true, false },
        { "\"+fps_hold\" 1", "30 FPS", true, false },
        { "hold", "30 FPS", true, false },
        { "hold_chain", "30 FPS", true, false },
        { "slot", "30 FPS", true, false },
        { "if 1 == 1 +fps 30 120", "FPS", false, false },
        { "if 1 == 1 +fps_hold 1 else hold_chain", "FPS", false, false },
        { "echo ready; +fps 30 120", "FPS", false, false },
        { "+fps 30 120; echo ready", "FPS", false, false },
    };
    labels[1] = "BUTTON";
    for (size_t i = 0; i < q_countof(cases); i++) {
        bindings[1] = cases[i].command;
        for (int up = 0; up < 2; up++) {
            current_fps = up ? "120" : "30";
            key_down[1] = !up;
            collect();
            const char *caption = key_row("BUTTON");
            bool expected = up ? cases[i].up_active : cases[i].down_active;
            if (!caption || strcmp(caption, cases[i].caption) || active_key("BUTTON") != expected)
                fprintf(stderr, "Incorrect button metadata: %s, current=%s, caption=%s, active=%d\n",
                        cases[i].command, current_fps, caption ? caption : "<none>", active_key("BUTTON"));
            assert(caption && !strcmp(caption, cases[i].caption));
            assert(active_key("BUTTON") == expected);
        }
    }
}

static void test_fps_pair_validation(void)
{
    setup();
    add_alias("invalid_pair", "+fps 30junk 120");
    add_alias("nested_invalid_pair", "invalid_pair");
    static const char *const invalid[] = {
        "+fps 0 120", "-fps 0 120", "+fps 30 0", "-fps 30 0",
        "+fps -30 120", "-fps 30 -120", "+fps 30.0 120", "-fps 30 120.0",
        "+fps 30junk 120", "-fps 30 120junk", "+fps \"\" 120", "-fps 30 \"\"",
        "+fps \" 30\" 120", "-fps 30 \"120 \"", "+fps 1e2 120", "-fps 30 0x78",
        "invalid_pair", "nested_invalid_pair", "if 1 == 1 invalid_pair else echo ready",
    };
    labels[1] = "PAIR";
    for (size_t i = 0; i < q_countof(invalid); i++) {
        bindings[1] = invalid[i];
        for (int up = 0; up < 2; up++) {
            current_fps = up ? "120" : "30";
            collect();
            if (key_row("PAIR"))
                fprintf(stderr, "Rejected FPS pair discovered: %s, current=%s\n", invalid[i], current_fps);
            assert(!key_row("PAIR"));
        }
    }
    bindings[1] = "if 1 == 1 invalid_pair else f120";
    collect();
    assert(!strcmp(key_row("PAIR"), "FPS") && !active_key("PAIR"));

    /* Either unresolved operand makes the whole pair's numeric metadata unknown. */
    static const char *const dynamic[] = {
        "+fps $hold $release", "+fps $hold 120", "+fps 30 $release",
        "-fps $hold 120", "-fps 30 $release", "+fps 3$suffix 120",
    };
    for (size_t i = 0; i < q_countof(dynamic); i++) {
        bindings[1] = dynamic[i];
        for (int up = 0; up < 2; up++) {
            current_fps = up ? "120" : "30";
            collect();
            assert(key_row("PAIR") && !active_key("PAIR"));
        }
    }
    bindings[1] = "+fps \"30\" \"120\"";
    collect();
    assert(!strcmp(key_row("PAIR"), "30 > 120") && active_key("PAIR"));
}

static void test_strict_fps_slot_metadata(void)
{
    setup();
    Cvar_Get("fps_hold_1", "30", CVAR_ARCHIVE);
    Cvar_Get("fps_release_1", "120", CVAR_ARCHIVE);
    labels[1] = "SLOT";
    static const char *const invalid[] = {
        "1junk", "1.0", "+1", "-1", "0", "13", "2147483648", "4294967297"
    };
    for (size_t i = 0; i < q_countof(invalid); i++) {
        char command[64];
        snprintf(command, sizeof(command), "+fps_hold %s", invalid[i]);
        bindings[1] = command;
        collect();
        assert(!key_row("SLOT"));
    }
    bindings[1] = "+fps_hold 1";
    set_text("fps_hold_1", "30junk");
    current_fps = "120";
    collect();
    assert(!strcmp(key_row("SLOT"), "120 FPS up") && active_key("SLOT"));
    set_text("fps_hold_1", "30");
    set_text("fps_release_1", "0");
    current_fps = "30";
    collect();
    assert(!active_key("SLOT"));
    key_down[1] = true;
    collect();
    assert(!strcmp(key_row("SLOT"), "30 FPS") && active_key("SLOT"));
    set_text("fps_hold_1", "2147483648");
    collect();
    assert(!key_row("SLOT"));
    bindings[1] = "-fps_hold 1";
    set_text("fps_release_1", "120.0");
    collect();
    assert(!key_row("SLOT"));
    bindings[1] = "+fps_hold 1$slot";
    collect();
    assert(!strcmp(key_row("SLOT"), "FPS") && !active_key("SLOT"));
    bindings[1] = "+fps 4294967297 120";
    collect();
    assert(!key_row("SLOT"));
}

static void test_paired_alias_metadata(void)
{
    setup();
    add_alias("+pair", "f30");
    add_alias("-pair", "f120");
    add_alias("nested_pair", "+pair");
    add_alias("+down", "f30");
    add_alias("-down", "echo ready");
    add_alias("+up", "echo ready");
    add_alias("-up", "f120");
    add_alias("+sequence", "f30; wait; f60");
    add_alias("-sequence", "f120");
    labels[1] = "ALIAS";
    static const struct {
        const char *command, *caption;
        bool down_active, up_active;
    } cases[] = {
        { "+pair", "FPS", true, true },
        { "  +pair", "30 FPS", true, false },
        { "\"+pair\"", "30 FPS", true, false },
        { "nested_pair", "30 FPS", true, false },
        { "+down", "30 FPS", true, false },
        { "+up", "120 FPS up", false, true },
        { "+sequence", "FPS", false, true },
        { "+fps 30 120", "30 > 120", true, true },
    };
    for (size_t i = 0; i < q_countof(cases); i++) {
        bindings[1] = cases[i].command;
        for (int up = 0; up < 2; up++) {
            current_fps = up ? "120" : "30";
            key_down[1] = !up;
            collect();
            assert(key_row("ALIAS") && !strcmp(key_row("ALIAS"), cases[i].caption));
            assert(active_key("ALIAS") == (up ? cases[i].up_active : cases[i].down_active));
        }
    }
    /* Paired aliases only describe the phase selected by their actual key. */
    bindings[1] = "+pair";
    key_down[1] = true;
    current_fps = "120";
    collect();
    assert(!active_key("ALIAS"));
    key_down[1] = false;
    current_fps = "30";
    collect();
    assert(!active_key("ALIAS"));
    bindings[1] = "+up";
    key_down[1] = true;
    current_fps = "120";
    collect();
    assert(!active_key("ALIAS"));
    /* A down-only action has no release FPS to replace its selected value. */
    bindings[1] = "+down";
    key_down[1] = false;
    current_fps = "30";
    collect();
    assert(active_key("ALIAS"));
}

static void test_complete_setter_metadata(void)
{
    setup();
    add_alias("multiple_values", "seta cl_maxfps 30 junk");
    labels[1] = "SETTER";
    current_fps = "30";
    static const struct {
        const char *command, *caption;
        bool active;
    } cases[] = {
        { "cl_maxfps 30", "30 FPS", true },
        { "cl_maxfps 30 junk", "FPS", false },
        { "cl_maxfps \"30 junk\"", "30 junk FPS", false },
        { "set cl_maxfps 30", "30 FPS", true },
        { "set cl_maxfps 30 u", "30 FPS", true },
        { "set cl_maxfps 30 s", "30 FPS", true },
        { "set cl_maxfps 30 \"u\"", "30 FPS", true },
        { "set cl_maxfps 30 U", "FPS", false },
        { "set cl_maxfps 30 junk", "FPS", false },
        { "set cl_maxfps 30 u junk", "FPS", false },
        { "set cl_maxfps 30 u \"\"", "FPS", false },
        { "seta cl_maxfps 30", "30 FPS", true },
        { "setu cl_maxfps 30", "30 FPS", true },
        { "sets cl_maxfps 30", "30 FPS", true },
        { "seta cl_maxfps 30 junk", "FPS", false },
        { "setu cl_maxfps 30 junk", "FPS", false },
        { "sets cl_maxfps 30 junk", "FPS", false },
        { "seta cl_maxfps 30 u", "FPS", false },
        { "multiple_values", "FPS", false },
    };
    for (size_t i = 0; i < q_countof(cases); i++) {
        bindings[1] = cases[i].command;
        collect();
        assert(key_row("SETTER") && !strcmp(key_row("SETTER"), cases[i].caption));
        assert(active_key("SETTER") == cases[i].active);
    }
}

static void test_responsive_anchor(void)
{
    const int viewports[][2] = { { 640, 480 }, { 960, 540 }, { 800, 600 } };
    for (size_t i = 0; i < q_countof(viewports); i++) {
        representative_bindings();
        scr.hud_width = viewports[i][0];
        scr.hud_height = viewports[i][1];
        draw(1);
        assert(primitives[0].x + primitives[0].w == scr.hud_width - 8);
        assert(primitives[0].y == Q_rint(scr.hud_height * 0.525f));
        int old_x = primitives[0].x, old_y = primitives[0].y, old_width = primitives[0].w;
        set_text("scr_bindreminders_label_1", "A longer FPS label");
        draw(1);
        assert(primitives[0].w > old_width && primitives[0].x < old_x);
        assert(primitives[0].x + primitives[0].w == scr.hud_width - 8);
        assert(primitives[0].y == old_y);
        set_text("scr_bindreminders_label_1", "30 / 120");
        labels[1] = "RIGHTALT";
        draw(1);
        assert(primitives[0].w > old_width);
        assert(primitives[0].x + primitives[0].w == scr.hud_width - 8);
        assert_text_does_not_overlap();
    }
    representative_bindings();
    scr.hud_height = 120;
    draw(1);
    assert(primitives[0].y + primitives[0].h == scr.hud_height - 8);

    representative_bindings();
    draw(1);
    int live_x = primitives[0].x, live_y = primitives[0].y;
    int live_width = primitives[0].w, live_height = primitives[0].h;
    offset_x = -12;
    offset_y = 7;
    draw_editor_preview();
    assert(primitives[0].x == live_x - 12 && primitives[0].y == live_y + 7);
    assert(primitives[0].w == live_width && primitives[0].h == live_height);
}

static void test_current_fps_metadata(void)
{
    setup();
    Cvar_Get("fps_hold_1", "30", CVAR_ARCHIVE);
    Cvar_Get("fps_release_1", "120", CVAR_ARCHIVE);
    add_alias("slow", "f30");
    add_alias("chain", "slow");
    add_alias("nested_hold", "+fps 30 120");
    add_alias("nested_slot", "+fps_hold 1");
    add_alias("dynamic_slot", "+fps_hold $slot");
    add_alias("conditional_fps", "if 1 == 1 f30 else f120");
    add_alias("sequence_fps", "f30; wait; f120");
    add_alias("+release_only", "echo ready");
    add_alias("-release_only", "f120");
    static const struct {
        const char *command, *current;
        bool active, held;
    } cases[] = { { "f30", "30", true },
                  { "f30", "30.0", true },
                  { "f30", "30.5", false },
                  { "cl_maxfps 30", "30", true },
                  { "cl_maxfps 30.0", "30", true },
                  { "cl_maxfps \" 30 \"", " 30.0\t", true },
                  { "set cl_maxfps 30", "30", true },
                  { "seta cl_maxfps 30", "30", true },
                  { "setu cl_maxfps 30", "30", true },
                  { "sets cl_maxfps 30", "30", true },
                  { "toggle cl_maxfps 20 30 60 120", "30", true },
                  { "toggle cl_maxfps 20 30 60 120", "60", true },
                  { "toggle cl_maxfps 20 30 60 120", "120", true },
                  { "toggle cl_maxfps 20 30 60 120", "90", false },
                  { "toggle cl_maxfps", "30", false },
                  { "toggle cl_maxfps \"\"", "30", false },
                  { "+fps 30 120", "30", true, true },
                  { "+fps 30 120", "30", false, false },
                  { "+fps 30 120", "120", false, true },
                  { "+fps 30 120", "120", true },
                  { "+fps 30 120", "60", false },
                  { "-fps 30 120", "30", false },
                  { "-fps 30 120", "120", true },
                  { "+fps_hold 1", "30", true, true },
                  { "+fps_hold 1", "30", false, false },
                  { "+fps_hold 1", "120", false, true },
                  { "+fps_hold 1", "120", true },
                  { "-fps_hold 1", "30", false },
                  { "-fps_hold 1", "120", true },
                  { "slow", "30", true },
                  { "chain", "30", true },
                  { "nested_hold", "30", true },
                  { "nested_hold", "120", false },
                  { "nested_slot", "30", true },
                  { "nested_slot", "120", false },
                  { "+release_only", "120", true },
                  { "+release_only", "30", false },
                  { "conditional_fps", "30", false },
                  { "sequence_fps", "30", false },
                  { "if 1 == 1 f30 else f120", "30", false },
                  { "f30; wait; f120", "30", false },
                  { "+fps_hold $slot", "30", false },
                  { "dynamic_slot", "30", false },
                  { "cl_maxfps $target", "30", false },
                  { "+fps $hold $release", "30", false },
                  { "inc cl_maxfps 30", "30", false },
                  { "dec cl_maxfps 30", "30", false },
                  { "creset cl_maxfps", "30", false },
                  { "cl_maxfps 30junk", "30", false },
                  { "cl_maxfps \"30 60\"", "30", false },
                  { "cl_maxfps nan", "nan", false },
                  { "cl_maxfps inf", "inf", false },
                  { "cl_maxfps -inf", "-inf", false },
                  { "cl_maxfps \"\"", "30", false },
                  { "cl_maxfps 1e999", "30", false },
                  { "f30", "", false },
                  { "f30", "30junk", false },
                  { "f30", "nan", false },
                  { "f30", "inf", false } };
    labels[1] = "ACTIVE";
    for (size_t i = 0; i < q_countof(cases); i++) {
        bindings[1] = cases[i].command;
        current_fps = cases[i].current;
        key_down[1] = cases[i].held;
        collect();
        assert(row_count == 4 && key_row("ACTIVE"));
        if (active_key("ACTIVE") != cases[i].active)
            fprintf(stderr, "Unexpected active target: %s, current=%s\n", cases[i].command, current_fps);
        assert(active_key("ACTIVE") == cases[i].active);
    }

    char long_target[256];
    memset(long_target, ' ', sizeof(long_target));
    memcpy(long_target, "cl_maxfps \"30", 13);
    memcpy(long_target + sizeof(long_target) - 6, "junk\"", 6);
    bindings[1] = long_target;
    current_fps = "30";
    collect();
    assert(key_row("ACTIVE") && !active_key("ACTIVE"));

    /* Captions are presentation only; metadata follows the bound command. */
    bindings[1] = "f30";
    set_text("scr_bindreminders_command_1", "f30");
    set_text("scr_bindreminders_label_1", "999 FPS");
    collect();
    assert(!strcmp(key_row("ACTIVE"), "999 FPS") && active_key("ACTIVE"));
    bindings[1] = "f60";
    set_text("scr_bindreminders_command_1", "f60");
    set_text("scr_bindreminders_label_1", "30 FPS");
    collect();
    assert(!strcmp(key_row("ACTIVE"), "30 FPS") && !active_key("ACTIVE"));
    set_text("scr_bindreminders_fps", "0");
    current_fps = "60";
    collect();
    assert(active_key("ACTIVE"));
    bindings[1] = NULL;
    collect();
    for (int i = 0; i < row_count; i++)
        assert(!collected_rows[i].active);
}

static void test_shared_hold_value_key_phases(void)
{
    static const struct {
        const char *current;
        bool held_m1, held_m2, active_m1, active_m2;
    } cases[] = {
        { "20", false, false, false, false },
        { "20", true, false, true, false },
        { "120", false, false, true, false },
        { "20", false, true, false, true },
        { "60", false, false, false, true },
        { "90", false, false, false, false },
        { "20.0", true, false, true, false },
        { "60.0", false, false, false, true },
        { "120", true, false, false, false },
        { "60", false, true, false, false },
        { "20", true, true, true, true },
        { "120", true, true, false, false },
        { "60", true, true, false, false },
        { "90", true, true, false, false },
    };
    for (int slots = 0; slots < 2; slots++) {
        setup();
        if (slots) {
            Cvar_Get("fps_hold_1", "20", CVAR_ARCHIVE);
            Cvar_Get("fps_release_1", "120", CVAR_ARCHIVE);
            Cvar_Get("fps_hold_2", "20", CVAR_ARCHIVE);
            Cvar_Get("fps_release_2", "60", CVAR_ARCHIVE);
        }
        bindings[1] = slots ? "+fps_hold 1" : "+fps 20 120";
        bindings[2] = slots ? "+fps_hold 2" : "+fps 20 60";
        labels[1] = "M1";
        labels[2] = "M2";
        for (size_t i = 0; i < q_countof(cases); i++) {
            current_fps = cases[i].current;
            key_down[1] = cases[i].held_m1;
            key_down[2] = cases[i].held_m2;
            collect();
            assert(!strcmp(key_row("M1"), "20 > 120"));
            assert(!strcmp(key_row("M2"), "20 > 60"));
            assert(active_key("M1") == cases[i].active_m1);
            assert(active_key("M2") == cases[i].active_m2);
        }
    }
}

static void test_manual_hold_keys_include_hidden_keys(void)
{
    setup_manual();
    set_text("scr_bindreminders_command_1", "+fps 20 120");
    set_text("scr_bindreminders_label_1", "Hold");
    set_text("scr_bindreminders_command_2", "");
    set_text("scr_bindreminders_command_3", "");
    set_text("scr_bindreminders_command_4", "");
    bindings[1] = bindings[2] = bindings[3] = "+fps 20 120";
    labels[1] = "M1";
    labels[2] = "M2";
    labels[3] = "M3";
    current_fps = "20";
    collect();
    assert(row_count == 1 && !active_key("M1 / M2 +"));
    key_down[3] = true;
    collect();
    assert(!strcmp(key_row("M1 / M2 +"), "Hold"));
    assert(active_key("M1 / M2 +"));
    key_down[3] = false;
    collect();
    assert(!active_key("M1 / M2 +"));
    current_fps = "120";
    key_down[1] = key_down[2] = true;
    collect();
    assert(active_key("M1 / M2 +"));
    key_down[3] = true;
    collect();
    assert(!active_key("M1 / M2 +"));
    bindings[1] = bindings[2] = bindings[3] = NULL;
    collect();
    assert(row_count == 1 && !active_key("--"));
}

static void test_active_outline(void)
{
    setup();
    bindings[1] = "f30";
    labels[1] = "ACTIVE";
    current_fps = "60";
    draw(0.7f);
    assert(fill_count == 10);
    uint32_t baseline_colors[10];
    int baseline_rects[10][4], base_count = 0;
    for (int i = 0; i < primitive_count; i++) {
        if (!primitives[i].fill)
            continue;
        baseline_colors[base_count] = primitives[i].color;
        baseline_rects[base_count][0] = primitives[i].x;
        baseline_rects[base_count][1] = primitives[i].y;
        baseline_rects[base_count][2] = primitives[i].w;
        baseline_rects[base_count][3] = primitives[i].h;
        base_count++;
    }
    assert(base_count == 10);
    int x = baseline_rects[2][0], y = baseline_rects[2][1];
    int w = baseline_rects[2][2], h = baseline_rects[2][3];
    const int expected_border[4][4] = {
        { x, y, w, 1 }, { x, y + h - 1, w, 1 },
        { x, y + 1, 1, h - 2 }, { x + w - 1, y + 1, 1, h - 2 }
    };
    current_fps = "30";
    draw(0.7f);
    assert(fill_count == 14);
    uint32_t border_color = MakeColor(190, 202, 210, (int)(0.42f * 210));
    int outlines = 0, unchanged = 0;
    for (int i = 0; i < primitive_count; i++) {
        if (!primitives[i].fill)
            continue;
        int rect[4] = { primitives[i].x, primitives[i].y, primitives[i].w, primitives[i].h };
        if (primitives[i].color == border_color) {
            assert(outlines < 4 && !memcmp(rect, expected_border[outlines], sizeof(rect)));
            assert(primitives[i].w == 1 || primitives[i].h == 1);
            outlines++;
        } else {
            assert(unchanged < 10 && primitives[i].color == baseline_colors[unchanged]);
            assert(!memcmp(rect, baseline_rects[unchanged], sizeof(rect)));
            unchanged++;
        }
    }
    assert(outlines == 4 && unchanged == 10);
    assert_text_does_not_overlap();
    bindings[1] = "cl_maxfps 30junk";
    draw(0.7f);
    assert(fill_count == 10);
}

static void json_string(FILE *file, const char *text)
{
    fputc('"', file);
    for (const unsigned char *p = (const unsigned char *)text; *p; p++) {
        if (*p == '"' || *p == '\\') {
            fputc('\\', file);
            fputc(*p, file);
        } else if (*p < 32) {
            fprintf(file, "\\u%04x", *p);
        } else {
            fputc(*p, file);
        }
    }
    fputc('"', file);
}

static int export_preview(const char *path, bool editor, const char *fps)
{
    representative_bindings();
    scr.hud_width = 1280;
    scr.hud_height = 720;
    current_fps = fps;
    if (editor)
        draw_editor_preview();
    else
        draw(0.7f);
    FILE *file = fopen(path, "wb");
    if (!file) {
        perror(path);
        return 1;
    }
    fprintf(file, "{\"width\":%d,\"height\":%d,\"charWidth\":%d,\"charHeight\":%d,"
                  "\"mode\":\"%s\",\"primitives\":[\n",
            scr.hud_width, scr.hud_height, CHAR_WIDTH, CHAR_HEIGHT,
            editor ? "editor" : "live");
    for (int i = 0; i < primitive_count; i++) {
        uint32_t color = LittleLong(primitives[i].color);
#if USE_BGRA
        unsigned red = (color >> 16) & 255, blue = color & 255;
#else
        unsigned red = color & 255, blue = (color >> 16) & 255;
#endif
        fprintf(file, "%s{\"kind\":\"%s\",\"x\":%d,\"y\":%d,\"w\":%d,\"h\":%d,"
                      "\"rgba\":[%u,%u,%u,%u],\"alpha\":%.6f,\"text\":",
                i ? ",\n" : "", primitives[i].fill ? "fill" : "text",
                primitives[i].x, primitives[i].y, primitives[i].w, primitives[i].h,
                red, (color >> 8) & 255, blue, (color >> 24) & 255, primitives[i].alpha);
        json_string(file, primitives[i].text);
        fputc('}', file);
    }
    fputs("\n]}\n", file);
    bool failed = ferror(file) != 0;
    failed |= fclose(file) != 0;
    return failed ? 1 : 0;
}

static void clear_custom_reminders(void)
{
    for (int i = 0; i < BIND_REMINDER_COUNT; i++)
        set_text(reminder_commands[i]->name, "");
}

static void test_extended_options(void)
{
    static const struct {
        const char *setting, *key, *caption, *value;
    } options[] = {
        { "scr_bindreminders_menu", "I", "Menu", "1" },
        { "scr_bindreminders_respawn", "K", "Respawn", "0" },
        { "scr_bindreminders_store", "S", "Store", "1" },
        { "scr_bindreminders_observer", "O", "Observer", "0" }
    };
    setup_manual();
    for (int i = 0; i < q_countof(options); i++) {
        cvar_t *var = setting(options[i].setting);
        assert(!strcmp(var->string, options[i].value));
        assert(!strcmp(var->default_string, options[i].value));
        assert(var->flags & CVAR_ARCHIVE);
    }
    bindings[4] = "inven";
    labels[4] = "I";
    collect();
    assert(row_count == 5 && !strcmp(key_row("I"), "Menu") && has_text("Store"));
    set_text("scr_bindreminders_menu", "0");
    set_text("scr_bindreminders_store", "0");
    collect();
    assert(row_count == 3 && !has_text("Store") && !has_text("Menu"));
    clear_custom_reminders();
    bindings[1] = "kill";
    labels[1] = "K";
    bindings[2] = "store";
    labels[2] = "S";
    bindings[3] = "observer";
    labels[3] = "O";
    /* The client menu key does not stand in for the JumpMod menu binding. */
    bindings[K_ESCAPE] = "say unrelated";
    labels[K_ESCAPE] = "OTHER";
    collect();
    assert(row_count == 0);
    for (int i = 0; i < q_countof(options); i++) {
        set_text(options[i].setting, "1");
        collected_rows[0].fps = collected_rows[0].active = true;
        collect();
        assert(row_count == 1);
        assert(!strcmp(collected_rows[0].keys, options[i].key));
        assert(!strcmp(collected_rows[0].caption, options[i].caption));
        assert(!collected_rows[0].fps && !collected_rows[0].active);
        draw(0.7f);
        assert(text_count == 2 && fill_count == 4);
        assert(rendered_text(options[i].key) >= 0 && rendered_text(options[i].caption) >= 0);
        set_text(options[i].setting, "0");
        collect();
        assert(row_count == 0);
    }
}

static void test_extended_custom_rows(void)
{
    static const struct {
        const char *setting, *command;
    } actions[] = {
        { "scr_bindreminders_menu", "INVEN" },
        { "scr_bindreminders_respawn", "KILL" },
        { "scr_bindreminders_store", "STORE" },
        { "scr_bindreminders_observer", "OBSERVER" }
    };
    for (int i = 0; i < q_countof(actions); i++) {
        setup_manual();
        clear_custom_reminders();
        set_text("scr_bindreminders_menu", "0");
        set_text("scr_bindreminders_store", "0");
        set_text("scr_bindreminders_command_1", actions[i].command);
        set_text("scr_bindreminders_label_1", "My action");
        bindings[1] = actions[i].command;
        labels[1] = "A";
        collect();
        assert(row_count == 0);
        set_text(actions[i].setting, "1");
        collected_rows[0].fps = collected_rows[0].active = true;
        collect();
        assert(row_count == 1 && !strcmp(key_row("A"), "My action"));
        assert(!collected_rows[0].fps && !collected_rows[0].active);
        draw(1);
        assert(text_count == 2 && fill_count == 4);
        bindings[1] = NULL;
        collect();
        assert(row_count == 1 && !strcmp(key_row("--"), "My action"));
        set_text(actions[i].setting, "0");
        collect();
        assert(row_count == 0);
    }
    /* The existing Store slot remains the one visible row, with its custom caption. */
    setup_manual();
    bindings[1] = "sToRe";
    labels[1] = "SAVE";
    set_text("scr_bindreminders_label_2", "Save point");
    collect();
    assert(row_count == 4 && !strcmp(key_row("SAVE"), "Save point"));
    int matching = 0;
    for (int i = 0; i < row_count; i++)
        matching += !strcmp(collected_rows[i].keys, "SAVE");
    assert(matching == 1);
}

static void test_extended_live_bindings(void)
{
    setup_manual();
    clear_custom_reminders();
    set_text("scr_bindreminders_menu", "1");
    set_text("scr_bindreminders_respawn", "1");
    set_text("scr_bindreminders_observer", "1");
    collect();
    assert(row_count == 0); /* Presets skip actions with no bound key. */
    bindings[1] = "KiLl";
    labels[1] = "A";
    bindings[2] = "STORE";
    labels[2] = "B";
    bindings[3] = "ObSeRvEr";
    labels[3] = "C";
    bindings[5] = "InVeN";
    labels[5] = "E";
    collect();
    assert(row_count == 4);
    assert(!strcmp(key_row("A"), "Respawn"));
    assert(!strcmp(key_row("B"), "Store"));
    assert(!strcmp(key_row("C"), "Observer"));
    assert(!strcmp(key_row("E"), "Menu"));
    bindings[5] = NULL;
    bindings[6] = "inven";
    labels[6] = "F";
    collect();
    assert(row_count == 4 && !key_row("E") && !strcmp(key_row("F"), "Menu"));
    bindings[6] = NULL;
    collect();
    assert(row_count == 3 && !has_text("Menu"));
    bindings[1] = NULL;
    bindings[4] = "kill";
    labels[4] = "D";
    collect();
    assert(row_count == 3 && !key_row("A") && !strcmp(key_row("D"), "Respawn"));
    bindings[4] = NULL;
    collect();
    assert(row_count == 2 && !has_text("Respawn"));
    /* Arguments, compounds and aliases do not count as exact action binds. */
    bindings[1] = "kill; say done";
    bindings[2] = "store extra";
    bindings[3] = " observer";
    bindings[4] = "save_alias";
    add_alias("save_alias", "store");
    bindings[5] = "inven; say menu";
    bindings[6] = "inven extra";
    bindings[7] = "menu_alias";
    add_alias("menu_alias", "inven");
    collect();
    assert(row_count == 0);
}

static void test_extended_capacity_and_hidden(void)
{
    setup();
    set_text("scr_bindreminders_menu", "1");
    set_text("scr_bindreminders_respawn", "1");
    set_text("scr_bindreminders_observer", "1");
    char keys[256][16], commands[8][32];
    for (int key = 0; key < q_countof(bindings); key++) {
        snprintf(keys[key], sizeof(keys[key]), "KEY%03d", key);
        labels[key] = keys[key];
        bindings[key] = "f20";
    }
    bindings[252] = "inven";
    bindings[253] = "kill";
    bindings[254] = "store";
    bindings[255] = "observer";
    for (int i = 0; i < BIND_REMINDER_COUNT; i++) {
        snprintf(commands[i], sizeof(commands[i]), "say custom%d", i);
        set_text(reminder_commands[i]->name, commands[i]);
        set_text(reminder_labels[i]->name, commands[i]);
    }
    collect();
    assert(row_count == BIND_REMINDER_KEYS + BIND_REMINDER_COUNT);
    assert(!strcmp(key_row("KEY252"), "Menu"));
    assert(!strcmp(key_row("KEY253"), "Respawn"));
    assert(!strcmp(key_row("KEY254"), "Store"));
    assert(!strcmp(key_row("KEY255"), "Observer"));
    for (int i = 0; i < row_count; i++)
        assert(!collected_rows[i].active);
    scr.hud_width = 1024;
    scr.hud_height = 8192;
    draw(1);
    assert(text_count == 528 && fill_count == 530);
    assert_text_does_not_overlap();
    draw_editor_preview();
    assert(text_count == 528 && fill_count == 530);
    item_visible.value = 0;
    draw(1);
    assert(binding_lookups == 0 && binding_enumerations == 0 && alias_lookups == 0);
    assert(text_count == 0 && primitive_count == 0 && begin_count == 0);
}

static void test_extended_saved_custom_rows(void)
{
    for (int explicit_off = 0; explicit_off < 2; explicit_off++) {
        setup();
        var_count = 0;
        Cvar_Get("scr_bindreminder_command_1", "KiLl", CVAR_ARCHIVE);
        Cvar_Get("scr_bindreminder_label_1", "My respawn", CVAR_ARCHIVE);
        Cvar_Get("scr_bindreminders_command_5", "OBSERVER", CVAR_ARCHIVE);
        Cvar_Get("scr_bindreminders_label_5", "My observer", CVAR_ARCHIVE);
        Cvar_Get("scr_bindreminders_command_6", "INVEN", CVAR_ARCHIVE);
        Cvar_Get("scr_bindreminders_label_6", "My menu", CVAR_ARCHIVE);
        if (explicit_off) {
            Cvar_Get("scr_bindreminders_respawn", "0", CVAR_ARCHIVE);
            Cvar_Get("scr_bindreminders_observer", "0", CVAR_ARCHIVE);
            Cvar_Get("scr_bindreminders_menu", "1", CVAR_ARCHIVE);
            set_text("scr_bindreminders_menu", "0");
        }
        SCR_BindRemindersInit();
        cvar_t *respawn = setting("scr_bindreminders_respawn");
        cvar_t *observer = setting("scr_bindreminders_observer");
        cvar_t *menu = setting("scr_bindreminders_menu");
        assert(respawn->integer == !explicit_off && observer->integer == !explicit_off);
        assert(!strcmp(respawn->default_string, "0") && !strcmp(observer->default_string, "0"));
        assert((respawn->flags & CVAR_ARCHIVE) && (observer->flags & CVAR_ARCHIVE));
        assert(menu->integer == !explicit_off && !strcmp(menu->default_string, "1"));
        assert(menu->flags & CVAR_ARCHIVE);
        bindings[1] = "kill";
        labels[1] = "K";
        bindings[2] = "observer";
        labels[2] = "O";
        bindings[3] = "inven";
        labels[3] = "I";
        collect();
        if (explicit_off) {
            assert(row_count == 3 && !key_row("K") && !key_row("O") && !key_row("I"));
        } else {
            assert(row_count == 6);
            assert(!strcmp(key_row("K"), "My respawn"));
            assert(!strcmp(key_row("O"), "My observer"));
            assert(!strcmp(key_row("I"), "My menu"));
        }
        Cvar_Reset(respawn);
        Cvar_Reset(observer);
        Cvar_Reset(menu);
        SCR_BindRemindersInit();
        assert(respawn->integer == 0 && observer->integer == 0 && menu->integer == 1);
        collect();
        assert(row_count == 4 && !key_row("K") && !key_row("O"));
        assert(!strcmp(key_row("I"), "My menu"));
    }
    /* Existing compound custom commands do not change exact-action defaults. */
    setup();
    var_count = 0;
    Cvar_Get("scr_bindreminders_command_1", "kill; say ready", CVAR_ARCHIVE);
    Cvar_Get("scr_bindreminders_command_5", "observer extra", CVAR_ARCHIVE);
    Cvar_Get("scr_bindreminders_command_6", "inven extra", CVAR_ARCHIVE);
    SCR_BindRemindersInit();
    assert(setting("scr_bindreminders_respawn")->integer == 0);
    assert(setting("scr_bindreminders_observer")->integer == 0);
    assert(setting("scr_bindreminders_menu")->integer == 1);
}

static void test_saved_cvar_names(void)
{
    drawing = false;
    var_count = 0;
    Cvar_Get("scr_bindreminder_command_1", "", CVAR_ARCHIVE);
    Cvar_Get("scr_bindreminder_label_2", "", CVAR_ARCHIVE);
    Cvar_Get("scr_bindreminder_command_5", "f75", CVAR_ARCHIVE);
    Cvar_Get("scr_bindreminder_label_5", "Precision", CVAR_ARCHIVE);
    Cvar_Get("scr_bindreminder_command_8", "f20", CVAR_ARCHIVE);
    Cvar_Get("scr_bindreminder_label_8", "Old", CVAR_ARCHIVE);
    Cvar_Get("scr_bindreminders_command_8", "f60", CVAR_ARCHIVE);
    Cvar_Get("scr_bindreminders_label_8", "", CVAR_ARCHIVE);
    SCR_BindRemindersInit();
    assert(!strcmp(reminder_commands[0]->string, ""));
    assert(!strcmp(reminder_labels[1]->string, ""));
    assert(!strcmp(reminder_commands[4]->string, "f75"));
    assert(!strcmp(reminder_labels[4]->string, "Precision"));
    assert(!strcmp(reminder_commands[7]->string, "f60"));
    assert(!strcmp(reminder_labels[7]->string, ""));
    assert(!strcmp(reminder_commands[0]->default_string, "toggle cl_maxfps 30 120"));
    assert(!strcmp(reminder_labels[1]->default_string, "Store"));
    assert(!strcmp(reminder_commands[4]->default_string, ""));
    assert(reminder_commands[4]->flags & CVAR_ARCHIVE);
    Cvar_Reset(reminder_commands[0]);
    Cvar_Reset(reminder_labels[1]);
    Cvar_Reset(reminder_commands[4]);
    Cvar_Reset(reminder_labels[4]);
    SCR_BindRemindersInit();
    assert(!strcmp(reminder_commands[0]->string, "toggle cl_maxfps 30 120"));
    assert(!strcmp(reminder_labels[1]->string, "Store"));
    assert(!strcmp(reminder_commands[4]->string, ""));
    assert(!strcmp(reminder_labels[4]->string, ""));
    assert(!strcmp(Cvar_FindVar("scr_bindreminder_command_5")->string, "f75"));

    var_count = 0;
    Cvar_Get("scr_bindreminder_command_1", "f20", CVAR_ARCHIVE | CVAR_WEAK);
    SCR_BindRemindersInit();
    assert(!strcmp(reminder_commands[0]->string, "toggle cl_maxfps 30 120"));
}

int main(int argc, char **argv)
{
    if ((argc == 3 || argc == 4) && (!strcmp(argv[1], "--preview") || !strcmp(argv[1], "--editor-preview")))
        return export_preview(argv[2], !strcmp(argv[1], "--editor-preview"), argc == 4 ? argv[3] : "30");
    if (argc != 1) {
        fprintf(stderr, "Usage: %s [--preview|--editor-preview output.json [current-fps]]\n", argv[0]);
        return 1;
    }
    test_saved_cvar_names();
    test_defaults_and_unbound_rows();
    test_live_bindings_and_layout_labels();
    test_configured_rows_and_captions();
    test_alpha_and_hidden_groups();
    test_viewport_and_text_limits();
    test_translation_precedes_clipping();
    test_automatic_default_and_live_rebind();
    test_every_registered_shortcut();
    test_direct_fps_patterns();
    test_empty_toggle_captions();
    test_live_hold_slot_values();
    test_aliases_and_scripts_without_execution();
    test_conditional_scripts_without_evaluation();
    test_disabled_item_skips_discovery();
    test_alias_depth_boundary();
    test_manual_fps_suppression_and_caption_override();
    test_truthful_fps_captions();
    test_raw_button_release_metadata();
    test_fps_pair_validation();
    test_strict_fps_slot_metadata();
    test_paired_alias_metadata();
    test_complete_setter_metadata();
    test_editor_alignment_geometry();
    test_empty_editor_preview();
    test_panel_and_editor_preview();
    test_short_mouse_labels_and_row_order();
    test_responsive_anchor();
    test_current_fps_metadata();
    test_shared_hold_value_key_phases();
    test_manual_hold_keys_include_hidden_keys();
    test_active_outline();
    test_extended_options();
    test_extended_custom_rows();
    test_extended_live_bindings();
    test_extended_capacity_and_hidden();
    test_extended_saved_custom_rows();
    puts("bind-reminders: all checks passed");
    return 0;
}
