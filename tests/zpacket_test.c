/* Exercise the real compressed-packet and server-message parsers offline. */
#undef NDEBUG
#include "../src/client/parse.c"
#include <assert.h>
#include <setjmp.h>

client_state_t cl;
client_static_t cls;
static jmp_buf error_jump;
static error_type_t error_code;
static char error_text[MAX_STRING_CHARS];
static bool expect_error;

void Com_Error(error_type_t code, const char *format, ...)
{
    va_list args;
    va_start(args, format);
    vsnprintf(error_text, sizeof(error_text), format, args);
    va_end(args);
    error_code = code;
    if (expect_error)
        longjmp(error_jump, 1);
    fprintf(stderr, "Unexpected error: %s\n", error_text);
    abort();
}

void Com_LPrintf(print_type_t type, const char *format, ...)
{
}

/* Unrelated client actions are boundaries, never part of these packets. */
cmdbuf_t cl_cmdbuf;
cvar_t *sv_running, *fs_game, *cl_chat_notify, *cl_chat_sound, *cl_chat_filter;
cvar_t *cl_shownet;

void Cbuf_AddText(cmdbuf_t *buf, const char *text) { abort(); }
void Cbuf_Execute(cmdbuf_t *buf) { abort(); }
void Cmd_ExecTrigger(const char *text) { abort(); }
void Com_SetColor(color_index_t color) { abort(); }
cvar_t *Cvar_UserSet(const char *name, const char *value) { abort(); }
void Netchan_Close(netchan_t *chan) { abort(); }
void PmoveInit(pmoveParams_t *pmp) { abort(); }
void PmoveEnableQW(pmoveParams_t *pmp) { abort(); }
void PmoveEnableExt(pmoveParams_t *pmp) { abort(); }
void CL_Disconnect(error_type_t type) { abort(); }
void Con_SkipNotify(bool skip) { abort(); }
void Con_Printf(const char *format, ...) { abort(); }
void S_ParseStartSound(void) { abort(); }
void S_StartLocalSoundOnce(const char *name) { abort(); }
void CL_CheckForResend(void) { abort(); }
void CL_ClearState(void) { abort(); }
bool CL_CheckForIgnore(const char *text) { abort(); }
void CL_UpdateConfigstring(int index) { abort(); }
void CL_HandleDownload(const byte *data, int size, int percent, int decompressed_size) { abort(); }
void CL_DeltaFrame(void) { abort(); }
void CL_ParseTEnt(void) { abort(); }
void CL_MuzzleFlash(void) { abort(); }
void CL_MuzzleFlash2(void) { abort(); }
void SCR_CenterPrint(const char *text, bool typewrite) { abort(); }
void SCR_AddToChatHUD(const char *text) { abort(); }
void SCR_PlayCinematic(const char *name) { abort(); }
#if USE_CLIENT_GTV
void CL_GTV_WriteMessage(const byte *data, size_t len) { }
#endif

static byte demo_buffer[MAX_MSGLEN];

static void ResetReader(size_t size)
{
    SZ_InitRead(&msg_read, msg_read_buffer, size);
    SZ_InitWrite(&cls.demo.buffer, demo_buffer, sizeof(demo_buffer));
    cls.demo.recording = true;
    cls.demo.paused = false;
    error_text[0] = 0;
}

static void ParsePacket(const char *error)
{
    expect_error = true;
    if (!setjmp(error_jump)) {
        CL_ParseServerMessage();
        assert(!error);
    } else {
        assert(error);
        assert(error_code == ERR_DROP);
        assert(strstr(error_text, error));
    }
    expect_error = false;
}

#if USE_ZLIB
static void MakePacket(const byte *data, size_t size, unsigned declared)
{
    z_stream encoder = { 0 };
    byte compressed[MAX_MSGLEN];
    sizebuf_t packet;

    assert(deflateInit2(&encoder, Z_DEFAULT_COMPRESSION, Z_DEFLATED,
                        -MAX_WBITS, 8, Z_DEFAULT_STRATEGY) == Z_OK);
    encoder.next_in = (byte *)data;
    encoder.avail_in = size;
    encoder.next_out = compressed;
    encoder.avail_out = sizeof(compressed);
    assert(deflate(&encoder, Z_FINISH) == Z_STREAM_END);
    SZ_InitWrite(&packet, msg_read_buffer, sizeof(msg_read_buffer));
    SZ_WriteByte(&packet, svc_zpacket);
    SZ_WriteShort(&packet, encoder.total_out);
    SZ_WriteShort(&packet, declared);
    SZ_Write(&packet, compressed, encoder.total_out);
    assert(deflateEnd(&encoder) == Z_OK);
    ResetReader(packet.cursize);
}

static void CheckPacketLengths(void)
{
    const byte payload[] = { svc_nop, svc_nop };
    sizebuf_t outer;

    /* Reproduction: reject before even the first decoded command is copied. */
    MakePacket(payload, 1, 32);
    outer = msg_read;
    ParsePacket("invalid decompressed length");
    assert(cls.z.total_out == 1);
    assert(cls.demo.buffer.cursize == 0);
    assert(msg_read.data == outer.data);
    assert(msg_read.cursize == outer.cursize);

    MakePacket(payload, sizeof(payload), sizeof(payload));
    msg_read_buffer[msg_read.cursize++] = svc_nop;
    msg_read.maxsize = msg_read.cursize;
    outer = msg_read;
    ParsePacket(NULL);
    assert(msg_read.data == outer.data);
    assert(msg_read.cursize == outer.cursize);
    assert(msg_read.maxsize == outer.maxsize);
    assert(msg_read.readcount == outer.cursize);
    assert(!msg_read.allowunderflow);
    assert(cls.demo.buffer.cursize == 3);
    assert(demo_buffer[0] == svc_nop && demo_buffer[1] == svc_nop && demo_buffer[2] == svc_nop);

    MakePacket(payload, sizeof(payload), 1);
    ParsePacket("inflate() failed");
    assert(cls.demo.buffer.cursize == 0);

    MakePacket(payload, 1, MAX_MSGLEN + 1);
    ParsePacket("invalid output length");
    assert(cls.demo.buffer.cursize == 0);

    /* The current decoder accepts an empty stream with an empty declaration. */
    MakePacket(payload, 0, 0);
    ParsePacket(NULL);
    assert(msg_read.readcount == msg_read.cursize);
    assert(cls.demo.buffer.cursize == 0);

    MakePacket(payload, 0, 1);
    ParsePacket("invalid decompressed length");
    assert(cls.demo.buffer.cursize == 0);

    MakePacket(payload, 1, 0);
    ParsePacket("inflate() failed");
    assert(cls.demo.buffer.cursize == 0);
}

static void CheckMalformedStreams(void)
{
    const byte payload[] = { svc_nop };
    const byte nested[] = { svc_zpacket };

    MakePacket(payload, sizeof(payload), sizeof(payload));
    msg_read.cursize--;
    ParsePacket("read past end");
    assert(cls.demo.buffer.cursize == 0);

    MakePacket(payload, sizeof(payload), sizeof(payload));
    msg_read.cursize--;
    msg_read_buffer[1]--; /* Keep the envelope consistent, truncate deflate. */
    ParsePacket("inflate() failed");
    assert(cls.demo.buffer.cursize == 0);

    MakePacket(payload, sizeof(payload), sizeof(payload));
    msg_read_buffer[5] = 7; /* Reserved deflate block type. */
    ParsePacket("inflate() failed");
    assert(cls.demo.buffer.cursize == 0);

    MakePacket(nested, sizeof(nested), sizeof(nested));
    ParsePacket("recursively entered");
    assert(cls.demo.buffer.cursize == 0);
}
#endif

int main(void)
{
    static cvar_t shownet;
    cl_shownet = &shownet;
    MSG_Init();
    cls.serverProtocol = PROTOCOL_VERSION_Q2PRO;
#if USE_ZLIB
    assert(inflateInit2(&cls.z, -MAX_WBITS) == Z_OK);
    CheckPacketLengths();
    CheckMalformedStreams();
    assert(inflateEnd(&cls.z) == Z_OK);
#else
    msg_read_buffer[0] = svc_zpacket;
    ResetReader(1);
    ParsePacket("no zlib support linked in");
    assert(cls.demo.buffer.cursize == 0);
#endif
    puts("zpacket: passed");
    return 0;
}
