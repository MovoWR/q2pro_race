/* Real packet sequence decoding and graph history updates; no sockets or UI. */
#undef NDEBUG
#include "../src/client/screen.c"
#include "../src/common/net/chan.c"
#include <assert.h>

client_state_t cl;
client_static_t cls;
unsigned com_localTime, com_localTime2, com_framenum;
cvar_t *sh_netmeter;
static cvar_t debug_var, time_var, netgraph_var, netmeter_var, color_var;
static unsigned notifications, notified_ping;

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
}

const char *NET_AdrToString(const netadr_t *address)
{
    return "fixture";
}

void SH_NetMeter_Sample(unsigned ping)
{
    notifications++;
    notified_ping = ping;
}

/* Fail if an unrelated rendering, command or socket boundary is reached. */
cmd_macro_t *Cmd_FindMacro(const char *name) { abort(); }
void Cmd_Macro_g(genctx_t *ctx) { abort(); }
void Cmd_Register(const cmdreg_t *reg) { abort(); }
void Cmd_Deregister(const cmdreg_t *reg) { abort(); }
int Cmd_Argc(void) { abort(); }
char *Cmd_Argv(int arg) { abort(); }
void Cvar_Variable_g(genctx_t *ctx) { abort(); }
cvar_t *Cvar_Get(const char *var_name, const char *value, int flags) { abort(); }
cvar_t *Cvar_WeakGet(const char *var_name) { abort(); }
void Cvar_SetByVar(cvar_t *var, const char *value, from_t from) { abort(); }
void Cvar_SetInteger(cvar_t *var, int value, from_t from) { abort(); }
int Cvar_ClampInteger(cvar_t *var, int min, int max) { abort(); }
float Cvar_ClampValue(cvar_t *var, float min, float max) { abort(); }
void Z_Free(void *ptr) { abort(); }
void *Z_Malloc(size_t size) { abort(); }
void *Z_TagMalloc(size_t size, memtag_t tag) { abort(); }
bool NET_SendPacket(netsrc_t sock, const void *data, size_t len, const netadr_t *to) { abort(); }
void Prompt_AddMatch(genctx_t *ctx, const char *s) { abort(); }
qhandle_t R_RegisterImage(const char *name, imagetype_t type, imageflags_t flags) { abort(); }
void R_SetSky(const char *name, float rotate, bool autorotate, const vec3_t axis) { abort(); }
void R_RenderFrame(const refdef_t *fd) { abort(); }
void R_ClearColor(void) { abort(); }
void R_SetAlpha(float clpha) { abort(); }
void R_SetColor(uint32_t color) { abort(); }
float R_ClampScale(cvar_t *var) { abort(); }
void R_SetScale(float scale) { abort(); }
void R_DrawChar(int x, int y, int flags, int ch, qhandle_t font) { abort(); }
int R_DrawString(int x, int y, int flags, size_t maxChars, const char *string, qhandle_t font) { abort(); }
bool R_GetPicSize(int *w, int *h, qhandle_t pic) { abort(); }
void R_DrawPic(int x, int y, qhandle_t pic) { abort(); }
void R_DrawStretchPic(int x, int y, int w, int h, qhandle_t pic) { abort(); }
void R_TileClear(int x, int y, int w, int h, qhandle_t pic) { abort(); }
void R_DrawFill8(int x, int y, int w, int h, int c) { abort(); }
void R_BeginFrame(void) { abort(); }
void R_EndFrame(void) { abort(); }
#if USE_MVD_CLIENT
bool MVD_GetDemoStatus(float *progress, bool *paused, int *framenum) { abort(); }
#endif
unsigned Sys_Milliseconds(void) { abort(); }
void CL_SetSky(void) { abort(); }
bool shc_ParseColorCvar(const char *cvarValue, uint32_t *outUint32, color_t *outColor) { abort(); }
void IN_Activate(void) { abort(); }
int Key_IsDown(int key) { abort(); }
const char *Key_GetBinding(const char *binding) { abort(); }
void S_StopAllSounds(void) { abort(); }
void cl_timeout_changed(cvar_t *self) { abort(); }
void V_RenderView(void) { abort(); }
void Con_DrawConsole(void) { abort(); }
void Con_ClearNotify_f(void) { abort(); }
void Con_CheckResize(void) { abort(); }
void SH_NetMeter_Clear(void) { abort(); }
void SH_NetMeter_Draw(void) { abort(); }
void SCR_DrawCinematic(void) { abort(); }
void HUD_LayoutInit(void) { abort(); }
void HUD_LayoutFrame(void) { abort(); }
int HUD_LayoutObject(const char *name) { abort(); }
void HUD_LayoutSetScaleAnchor(int id, float x, float y) { abort(); }
void HUD_LayoutBegin(int id) { abort(); }
void HUD_LayoutEnd(void) { abort(); }
bool HUD_LayoutMatch(const char *layout) { abort(); }
int HUD_LayoutToken(const char *layout, const char *position) { abort(); }
void HUD_LayoutSetBounds(int id, vrect_t bounds) { abort(); }
void SCR_BindRemindersInit(void) { abort(); }
void SCR_DrawBindReminders(float hud_alpha) { abort(); }
void StrafeHelper_UpdateEfficiency(void) { abort(); }
void StrafeHelper_Draw(const struct StrafeHelperParams *params, float hud_width, float hud_height, int font_pic) { abort(); }
void StrafeHelper_DrawPreview(const struct StrafeHelperParams *params, float hud_width, float hud_height, int font_pic) { abort(); }
void SH_Ups_Draw(float hud_width, float hud_height, float hud_scale, int font_pic) { abort(); }
void OriginUpdate(void) { abort(); }
char *SH_NerdStats_Draw(float hud_width, float hud_height, int font_pic) { abort(); }
unsigned SH_NetMeter_GetAvgPing(void) { abort(); }
#if USE_UI
void UI_ModeChanged(void) { abort(); }
void UI_Draw(unsigned realtime) { abort(); }
bool UI_IsTransparent(void) { abort(); }
bool UI_IsMenuActive(const char *name) { abort(); }
bool HUD_EditorActive(void) { abort(); }
bool HUD_EditorViewport(vrect_t *viewport) { abort(); }
#endif
refcfg_t r_config;
cvar_t *cl_paused;
cvar_t *sv_running;
cvar_t *sv_paused;
cvar_t *cl_drawStrafeHelper;
cvar_t *cl_strafeHelperCenter;
cvar_t *cl_strafeHelperCenterMarker;
cvar_t *cl_strafeHelperHeight;
cvar_t *cl_strafeHelperScale;
cvar_t *cl_strafeHelperY;
cvar_t *sh_lagometer_x;
cvar_t *sh_lagometer_y;
cvar_t *sh_netmeter_min_ms;
cvar_t *sh_netmeter_max_ms;
cvar_t *sh_netmeter_adaptive;
cvar_t *sh_netgraph_height;
cvar_t *sh_netgraph_alpha;
cvar_t *sh_netgraph_color_normal;
cvar_t *sh_netgraph_color_loss_s2c;

static void Seed(unsigned head, unsigned dropped, unsigned suppressed)
{
    memset(&cl, 0, sizeof(cl));
    memset(&cls, 0, sizeof(cls));
    debug_var.integer = time_var.integer = 0;
    netgraph_var.integer = 0;
    netmeter_var.integer = 3;
    color_var.integer = 42;
    scr_debuggraph = &debug_var;
    scr_timegraph = &time_var;
    scr_netgraph = &netgraph_var;
    scr_graphcolor = &color_var;
    sh_netmeter = &netmeter_var;
#if USE_DEBUG
    showdrop = showpackets = &debug_var;
#endif
    graph.current = lag.head = head;
    for (unsigned i = 0; i < GRAPH_SAMPLES; i++) {
        graph.values[i] = 1000 + i;
        graph.colors[i] = i & 255;
    }
    for (unsigned i = 0; i < LAG_WIDTH; i++)
        lag.samples[i] = 10000 + i;
    cls.netchan.dropped = dropped;
    cls.netchan.incoming_acknowledged = CMD_BACKUP + 3;
    cls.realtime = 1123;
    cl.history[3].sent = 1000;
    cl.history[3].cmdNumber = 7;
    cl.suppress_count = suppressed;
    msg_read.cursize = 800;
    notifications = notified_ping = 0;
}

/* Deliberately simple sequential oracles, used only for bounded small gaps. */
static void ReferenceGraph(debuggraph_t *expected, unsigned dropped, unsigned suppressed)
{
    for (unsigned i = 0; i < dropped + suppressed + 1; i++) {
        unsigned slot = expected->current & GRAPH_MASK;
        expected->values[slot] = i < dropped + suppressed ? 30 : 4;
        expected->colors[slot] = i < dropped ? 0x40 : i < dropped + suppressed ? 0xdf : 42;
        expected->current++;
    }
}

static void ReferenceLag(lagometer_t *expected, unsigned dropped, bool suppressed)
{
    for (unsigned i = 0; i < dropped + 1; i++) {
        unsigned slot = expected->head % LAG_WIDTH;
        expected->samples[slot] = 123 | (i < dropped ? LAG_CRIT_BIT : suppressed ? LAG_WARN_BIT : 0);
        expected->head++;
    }
}

static void AssertGraph(const debuggraph_t *expected)
{
    assert(graph.current == expected->current);
    assert(!memcmp(graph.values, expected->values, sizeof(graph.values)));
    assert(!memcmp(graph.colors, expected->colors, sizeof(graph.colors)));
}

static void AssertLag(const lagometer_t *expected)
{
    assert(lag.head == expected->head);
    assert(!memcmp(lag.samples, expected->samples, sizeof(lag.samples)));
}

static void CheckSmallGap(unsigned head, unsigned dropped, unsigned suppressed)
{
    debuggraph_t expected_graph;
    lagometer_t expected_lag;

    Seed(head, dropped, suppressed);
    expected_graph = graph;
    expected_lag = lag;
    ReferenceGraph(&expected_graph, dropped, suppressed);
    cl.frameflags = suppressed ? FF_SUPPRESSED : 0;
    ReferenceLag(&expected_lag, dropped, suppressed != 0);
    SCR_AddNetgraph();
    SCR_LagSample();
    AssertGraph(&expected_graph);
    AssertLag(&expected_lag);
    assert(cls.netchan.dropped == dropped);
    assert(cl.history[3].rcvd == cls.realtime);
    assert(notifications == 1 && notified_ping == 123);
}

static void CheckSmallGaps(void)
{
    static const unsigned gaps[] = {
        0, 1, LAG_WIDTH - 1, LAG_WIDTH, LAG_WIDTH + 1,
        2 * LAG_WIDTH - 1, 2 * LAG_WIDTH, 2 * LAG_WIDTH + 1,
        GRAPH_SAMPLES - 1, GRAPH_SAMPLES, GRAPH_SAMPLES + 1,
        2 * GRAPH_SAMPLES - 1, 2 * GRAPH_SAMPLES, 2 * GRAPH_SAMPLES + 1
    };
    static const unsigned heads[] = { 0, 1, 47, 4095, UINT_MAX - 70, UINT_MAX - 20, UINT_MAX };
    for (unsigned i = 0; i < q_countof(heads); i++)
        for (unsigned j = 0; j < q_countof(gaps); j++)
            for (unsigned suppressed = 0; suppressed < 3; suppressed++)
                CheckSmallGap(heads[i], gaps[j], suppressed);

    /* Width 48 does not divide UINT_MAX + 1: test every short wrap shape. */
    for (unsigned gap = LAG_WIDTH; gap < 2 * LAG_WIDTH; gap++)
        for (unsigned before_wrap = 0; before_wrap < gap; before_wrap++)
            CheckSmallGap(UINT_MAX - before_wrap, gap, gap & 1);
}

static void CheckHugeGap(unsigned head, unsigned dropped)
{
    unsigned final_loss_head = head + dropped;
    unsigned graph_slot = final_loss_head & GRAPH_MASK;
    unsigned lag_slot = final_loss_head % LAG_WIDTH;

    Seed(head, dropped, 2);
    cl.frameflags = FF_SUPPRESSED;
    SCR_AddNetgraph();
    SCR_LagSample();
    assert(graph.current == final_loss_head + 3);
    assert(lag.head == final_loss_head + 1);
    for (unsigned i = 0; i < GRAPH_SAMPLES; i++) {
        unsigned offset = (i - graph_slot) & GRAPH_MASK;
        assert(graph.values[i] == (offset == 2 ? 4 : 30));
        assert(graph.colors[i] == (offset < 2 ? 0xdf : offset == 2 ? 42 : 0x40));
    }
    for (unsigned i = 0; i < LAG_WIDTH; i++)
        assert(lag.samples[i] == (123u | (i == lag_slot ? LAG_WARN_BIT : LAG_CRIT_BIT)));
    assert(cls.netchan.dropped == dropped);
    assert(notifications == 1 && notified_ping == 123);
}

static void CheckModes(void)
{
    debuggraph_t expected;
    for (unsigned mode = 0; mode < 4; mode++) {
        Seed(37, 5, 2);
        expected = graph;
        if (mode == 0)
            debug_var.integer = 1;
        else if (mode == 1)
            time_var.integer = 1;
        else {
            expected.values[37] = mode == 2 ? 20 : 4;
            expected.colors[37] = mode == 2 ? 224 : 42;
            expected.current++;
            if (mode == 2)
                netgraph_var.integer = 2;
            else
                netmeter_var.integer = 2;
        }
        SCR_AddNetgraph();
        AssertGraph(&expected);
        assert(cls.netchan.dropped == 5);
    }

    /* Packet-size mode 3 retains loss and suppressed markers. */
    Seed(37, 5, 2);
    expected = graph;
    ReferenceGraph(&expected, 5, 2);
    expected.values[44] = 20;
    expected.colors[44] = 224;
    netgraph_var.integer = 3;
    SCR_AddNetgraph();
    AssertGraph(&expected);

    Seed(0, 0, 0);
    netgraph_var.integer = 3;
    static const unsigned sizes[] = { 0, 199, 200, 499, 500, 799, 800, 1199, 1200, MAX_MSGLEN };
    static const byte colors[] = { 61, 61, 59, 59, 57, 57, 224, 224, 242, 242 };
    for (unsigned i = 0; i < q_countof(sizes); i++) {
        msg_read.cursize = sizes[i];
        SCR_AddNetgraph();
        assert(graph.colors[i] == colors[i]);
        assert(graph.values[i] == min(sizes[i] / 40, 30));
    }
}

static void CheckInvalidHistory(void)
{
    lagometer_t expected;
    Seed(UINT_MAX, 999999899, 0);
    expected = lag;
    cl.history[3].cmdNumber = 0;
    SCR_LagSample();
    AssertLag(&expected);
    assert(cl.history[3].rcvd == cls.realtime);
    assert(notifications == 0);

    cl.history[3].cmdNumber = 1;
    cl.history[3].sent = cls.realtime + 1;
    SCR_LagSample();
    AssertLag(&expected);
    assert(notifications == 0);
}

static void CheckProtocolGap(bool newer)
{
    const unsigned previous = 100, sequence = 1000000000;
    sizebuf_t packet;
    Seed(UINT_MAX - 50, 0, 0);
    cls.netchan.sock = NS_CLIENT;
    cls.netchan.incoming_sequence = previous;
    cls.netchan.protocol = newer ? PROTOCOL_VERSION_Q2PRO : PROTOCOL_VERSION_R1Q2;
    SZ_InitWrite(&packet, msg_read_buffer, sizeof(msg_read_buffer));
    SZ_WriteLong(&packet, sequence | REL_BIT);
    SZ_WriteLong(&packet, 3);
    SZ_InitRead(&msg_read, msg_read_buffer, packet.cursize);
    assert(newer ? NetchanNew_Process(&cls.netchan) : NetchanOld_Process(&cls.netchan));
    assert(msg_read.readcount == 8);
    assert(cls.netchan.dropped == sequence - previous - 1);
    assert(cls.netchan.total_dropped == sequence - previous - 1);
    assert(cls.netchan.total_received == sequence - previous);
    assert(cls.netchan.incoming_sequence == sequence);
    assert(cls.netchan.incoming_acknowledged == 3);
    SCR_AddNetgraph();
    SCR_LagSample();
    assert(graph.current == (UINT_MAX - 50) + (sequence - previous));
    assert(lag.head == graph.current);
    assert(cls.netchan.dropped == sequence - previous - 1);
    assert(cls.netchan.total_dropped == sequence - previous - 1);
    assert(cls.netchan.total_received == sequence - previous);
    assert(notifications == 1 && notified_ping == 123);
}

int main(void)
{
    MSG_Init();
    CheckSmallGaps();
    CheckHugeGap(0, 999999899);
    CheckHugeGap(UINT_MAX - 50, 999999899);
    CheckHugeGap(UINT_MAX - 50, UINT_MAX);
    CheckModes();
    CheckInvalidHistory();
    CheckProtocolGap(false);
    CheckProtocolGap(true);
    puts("netgraph history: passed");
    return 0;
}
