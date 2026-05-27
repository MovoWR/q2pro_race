#include <src/client/client.h>
#include "sh_netbar.h"

static cvar_t   *scr_netbar;
static cvar_t   *scr_netbar_y;
static cvar_t   *scr_netbar_h;
static cvar_t   *scr_netbar_alpha;
static cvar_t   *scr_netbar_bad_alpha;
static cvar_t   *scr_netbar_history;
static cvar_t   *scr_netbar_ping_mode;
static cvar_t   *scr_netbar_labels;
static cvar_t   *scr_netbar_notice;
static cvar_t   *scr_netbar_notice_y;
static cvar_t   *scr_netbar_notice_ms;
static cvar_t   *scr_netbar_notice_alpha;
static cvar_t   *scr_netwarn_highping;
static cvar_t   *scr_netwarn_ping_adaptive;
static cvar_t   *scr_netwarn_spike_ms;
static cvar_t   *scr_netwarn_spike_pct;
static cvar_t   *scr_netwarn_jitter_ms;
static cvar_t   *scr_netwarn_stall_ms;
static cvar_t   *scr_netwarn_stall_crit_ms;
static cvar_t   *scr_netwarn_pred_warn;
static cvar_t   *scr_netwarn_pred_crit;

typedef enum {
    NETBAR_SPIKE   = BIT(0),
    NETBAR_JITTER  = BIT(1),
    NETBAR_LOSS    = BIT(2),
    NETBAR_CHOKE   = BIT(3),
    NETBAR_PRED    = BIT(4),
    NETBAR_FRAME   = BIT(5),
    NETBAR_STALL   = BIT(6),
    NETBAR_PING    = BIT(7)
} netbar_flags_t;

typedef enum {
    NETBAR_SEV_NONE,
    NETBAR_SEV_WARN,
    NETBAR_SEV_CRIT
} netbar_severity_t;

typedef struct {
    unsigned    time;
    unsigned    ping;
    unsigned    flags;
    byte        severity;
} netbar_sample_t;

#define NETBAR_SAMPLES  4096
#define NETBAR_MASK     (NETBAR_SAMPLES - 1)

#define NETBAR_COLOR_NORMAL LAG_BASE
#define NETBAR_COLOR_WARN   LAG_WARN
#define NETBAR_COLOR_CRIT   LAG_CRIT
#define NETBAR_COLOR_CHOKE  224

static struct {
    netbar_sample_t samples[NETBAR_SAMPLES];
    unsigned        head;
    unsigned        avg_ping;
    unsigned        last_ping;
    unsigned        jitter;
    unsigned        jitter_base;
} netbar;

void SH_NetBar_Init(void)
{
    scr_netbar = Cvar_Get("netbar", "0", CVAR_ARCHIVE);
    scr_netbar_y = Cvar_Get("netbar_y", "-8", CVAR_ARCHIVE);
    scr_netbar_h = Cvar_Get("netbar_h", "8", CVAR_ARCHIVE);
    scr_netbar_alpha = Cvar_Get("netbar_alpha", "0.2", CVAR_ARCHIVE);
    scr_netbar_bad_alpha = Cvar_Get("netbar_bad_alpha", "1.0", CVAR_ARCHIVE);
    scr_netbar_history = Cvar_Get("netbar_history", "8000", CVAR_ARCHIVE);
    scr_netbar_ping_mode = Cvar_Get("netbar_ping_mode", "1", CVAR_ARCHIVE);
    scr_netbar_labels = Cvar_Get("netbar_labels", "2", CVAR_ARCHIVE);
    scr_netbar_notice = Cvar_Get("netbar_notice", "1", CVAR_ARCHIVE);
    scr_netbar_notice_y = Cvar_Get("netbar_notice_y", "0", CVAR_ARCHIVE);
    scr_netbar_notice_ms = Cvar_Get("netbar_notice_ms", "2500", CVAR_ARCHIVE);
    scr_netbar_notice_alpha = Cvar_Get("netbar_notice_alpha", "1.0", CVAR_ARCHIVE);

    scr_netwarn_highping = Cvar_Get("netwarn_highping", "0", CVAR_ARCHIVE);
    scr_netwarn_ping_adaptive = Cvar_Get("netwarn_ping_adaptive", "1", CVAR_ARCHIVE);
    scr_netwarn_spike_ms = Cvar_Get("netwarn_spike_ms", "25", CVAR_ARCHIVE);
    scr_netwarn_spike_pct = Cvar_Get("netwarn_spike_pct", "25", CVAR_ARCHIVE);
    scr_netwarn_jitter_ms = Cvar_Get("netwarn_jitter_ms", "15", CVAR_ARCHIVE);
    scr_netwarn_stall_ms = Cvar_Get("netwarn_stall_ms", "500", CVAR_ARCHIVE);
    scr_netwarn_stall_crit_ms = Cvar_Get("netwarn_stall_crit_ms", "1000", CVAR_ARCHIVE);
    scr_netwarn_pred_warn = Cvar_Get("netwarn_pred_warn", "5", CVAR_ARCHIVE);
    scr_netwarn_pred_crit = Cvar_Get("netwarn_pred_crit", "20", CVAR_ARCHIVE);
}

static void SCR_NetBarPush(unsigned time, unsigned ping,
                           unsigned flags, netbar_severity_t severity)
{
    netbar_sample_t *sample = &netbar.samples[netbar.head & NETBAR_MASK];

    sample->time = time;
    sample->ping = ping;
    sample->flags = flags;
    sample->severity = severity;
    netbar.head++;
}

void SH_NetBar_Sample(unsigned ping)
{
    unsigned flags = 0;
    unsigned delta, spike_threshold, jitter_threshold;
    int spike, spike_pct, jitter;
    netbar_severity_t severity = NETBAR_SEV_NONE;

    if (!cls.netchan.protocol || cls.demo.playback) {
        return;
    }

    spike = Cvar_ClampInteger(scr_netwarn_spike_ms, 0, 999);
    spike_pct = Cvar_ClampInteger(scr_netwarn_spike_pct, 0, 1000);
    jitter = Cvar_ClampInteger(scr_netwarn_jitter_ms, 0, 999);

    if (!netbar.avg_ping) {
        netbar.avg_ping = ping;
    } else {
        netbar.avg_ping = (netbar.avg_ping * 7 + ping) >> 3;
    }

    if (netbar.last_ping) {
        delta = abs((int)ping - (int)netbar.last_ping);
        netbar.jitter = (netbar.jitter * 7 + delta) >> 3;
        if (!netbar.jitter_base) {
            netbar.jitter_base = netbar.jitter;
        } else if (netbar.jitter < netbar.jitter_base + 2) {
            netbar.jitter_base = (netbar.jitter_base * 15 + netbar.jitter) >> 4;
        }
    }
    netbar.last_ping = ping;

    if (scr_netwarn_highping->integer > 0) {
        int warn = Cvar_ClampInteger(scr_netwarn_highping, 0, 999);
        if (warn && ping >= (unsigned)warn) {
            flags |= NETBAR_PING;
            severity = max(severity, NETBAR_SEV_WARN);
        }
    }

    spike_threshold = spike;
    if (scr_netwarn_ping_adaptive->integer) {
        spike_threshold = max(spike_threshold, netbar.avg_ping * (unsigned)spike_pct / 100);
    }
    if (spike_threshold && ping > netbar.avg_ping + spike_threshold) {
        flags |= NETBAR_SPIKE;
        severity = max(severity, ping > netbar.avg_ping + spike_threshold * 2 ?
                       NETBAR_SEV_CRIT : NETBAR_SEV_WARN);
    }

    jitter_threshold = jitter;
    if (scr_netwarn_ping_adaptive->integer) {
        jitter_threshold = max(jitter_threshold, netbar.jitter_base + spike_threshold / 4);
    }
    if (jitter_threshold && netbar.jitter >= jitter_threshold) {
        flags |= NETBAR_JITTER;
        severity = max(severity, netbar.jitter >= jitter_threshold * 2 ?
                       NETBAR_SEV_CRIT : NETBAR_SEV_WARN);
    }

    if (!scr_netwarn_ping_adaptive->integer && scr_netwarn_highping->integer > 0 &&
        ping >= (unsigned)Cvar_ClampInteger(scr_netwarn_highping, 0, 999) * 2) {
        flags |= NETBAR_PING;
        severity = max(severity, NETBAR_SEV_CRIT);
    }

    if (cl.frameflags & FF_SUPPRESSED) {
        flags |= NETBAR_CHOKE;
        severity = max(severity, NETBAR_SEV_WARN);
    }

    if (cls.netchan.dropped || (cl.frameflags & (FF_SERVERDROP | FF_CLIENTDROP))) {
        flags |= NETBAR_LOSS;
        severity = NETBAR_SEV_CRIT;
    }

    if (cl.frameflags & FF_CLIENTPRED) {
        flags |= NETBAR_PRED;
        severity = NETBAR_SEV_CRIT;
    }

    if (cl.frameflags & (FF_BADFRAME | FF_OLDFRAME | FF_OLDENT | FF_NODELTA)) {
        flags |= NETBAR_FRAME;
        severity = max(severity, NETBAR_SEV_WARN);
    }

    SCR_NetBarPush(cls.realtime, ping, flags, severity);
}

void SH_NetBar_PredictionError(int len)
{
    int warn, crit;
    netbar_severity_t severity;

    if (!cls.netchan.protocol || cls.demo.playback) {
        return;
    }

    warn = Cvar_ClampInteger(scr_netwarn_pred_warn, 0, 640);
    crit = Cvar_ClampInteger(scr_netwarn_pred_crit, warn, 640);
    if (len < warn) {
        return;
    }

    severity = len >= crit ? NETBAR_SEV_CRIT : NETBAR_SEV_WARN;
    SCR_NetBarPush(cls.realtime, netbar.last_ping, NETBAR_PRED, severity);
}

static int SCR_NetBarColor(const netbar_sample_t *sample)
{
    if (sample->severity >= NETBAR_SEV_CRIT) {
        return NETBAR_COLOR_CRIT;
    }
    if (sample->flags & NETBAR_CHOKE) {
        return NETBAR_COLOR_CHOKE;
    }
    if (sample->severity == NETBAR_SEV_WARN) {
        return NETBAR_COLOR_WARN;
    }
    return NETBAR_COLOR_NORMAL;
}

static const char *SCR_NetBarLabel(unsigned flags)
{
    if (flags & NETBAR_STALL) {
        return "STALL";
    }
    if (flags & NETBAR_LOSS) {
        return "LOSS";
    }
    if (flags & NETBAR_PRED) {
        return "PRED";
    }
    if (flags & NETBAR_CHOKE) {
        return "CHOKE";
    }
    if (flags & NETBAR_FRAME) {
        return "FRAME";
    }
    if (flags & NETBAR_JITTER) {
        return "JIT";
    }
    if (flags & NETBAR_SPIKE) {
        return "SPIKE";
    }
    if (flags & NETBAR_PING) {
        return "PING";
    }
    return NULL;
}

static const char *SCR_NetBarNotice(unsigned flags)
{
    if (flags & NETBAR_STALL) {
        return "NETWORK STALL";
    }
    if (flags & NETBAR_LOSS) {
        return "PACKET LOSS";
    }
    if (flags & NETBAR_PRED) {
        return "PREDICTION MISS";
    }
    if (flags & NETBAR_CHOKE) {
        return "NETWORK CHOKE";
    }
    if (flags & NETBAR_FRAME) {
        return "FRAME RECOVERY";
    }
    if (flags & NETBAR_JITTER) {
        return "NETWORK JITTER";
    }
    if (flags & NETBAR_SPIKE) {
        return "PING SPIKE";
    }
    if (flags & NETBAR_PING) {
        return "HIGH PING";
    }
    return NULL;
}

static void SCR_NetBarActiveWarning(unsigned *flags, unsigned *ping,
                                    netbar_severity_t *severity, unsigned hold)
{
    unsigned count, now = cls.realtime;
    unsigned i, age;

    *flags = 0;
    *ping = netbar.last_ping;
    *severity = NETBAR_SEV_NONE;

    for (count = 0; count < NETBAR_SAMPLES; count++) {
        netbar_sample_t *sample = &netbar.samples[(netbar.head - 1 - count) & NETBAR_MASK];
        if (!sample->time) {
            break;
        }

        age = now - sample->time;
        if (age > hold) {
            break;
        }

        if (sample->severity > *severity) {
            *severity = sample->severity;
            *flags = sample->flags;
            *ping = sample->ping;
        }
    }

    i = Cvar_ClampInteger(scr_netwarn_stall_ms, 0, 10000);
    if (i && cls.netchan.last_received && now - cls.netchan.last_received >= i) {
        *flags = NETBAR_STALL;
        *ping = netbar.last_ping;
        *severity = now - cls.netchan.last_received >=
            (unsigned)Cvar_ClampInteger(scr_netwarn_stall_crit_ms, i, 30000) ?
            NETBAR_SEV_CRIT : NETBAR_SEV_WARN;
    }
}

static void SCR_DrawNetBarLabel(int y)
{
    unsigned flags, ping;
    netbar_severity_t severity;
    const char *label;
    char buffer[32];
    int text_y;
    float alpha;

    if (scr_netbar_labels->integer <= 0) {
        return;
    }

    SCR_NetBarActiveWarning(&flags, &ping, &severity, 900);
    if (severity == NETBAR_SEV_NONE) {
        return;
    }

    label = SCR_NetBarLabel(flags);
    if (!label) {
        return;
    }

    if (scr_netbar_labels->integer > 1 && ping) {
        Q_snprintf(buffer, sizeof(buffer), "%s %u", label, ping);
    } else {
        Q_strlcpy(buffer, label, sizeof(buffer));
    }

    text_y = y - CHAR_HEIGHT - 1;
    if (text_y < 0) {
        text_y = y + Cvar_ClampInteger(scr_netbar_h, 1, 32) + 1;
    }
    if (text_y + CHAR_HEIGHT > scr.hud_height) {
        return;
    }

    alpha = severity == NETBAR_SEV_CRIT ?
        Cvar_ClampValue(scr_netbar_bad_alpha, 0, 1) :
        Cvar_ClampValue(scr_netbar_alpha, 0, 1) * 2.0f;
    if (alpha > 1) {
        alpha = 1;
    }

    R_SetAlpha(alpha * Cvar_ClampValue(scr_alpha, 0, 1));
    SCR_DrawString(scr.hud_width - 2, text_y, UI_RIGHT | UI_ALTCOLOR, buffer);
}

static void SCR_DrawNetBarNotice(int bar_y, int bar_h)
{
    unsigned flags, ping;
    netbar_severity_t severity;
    const char *notice;
    char buffer[48];
    int mode, y;
    float alpha;

    mode = Cvar_ClampInteger(scr_netbar_notice, 0, 2);
    if (!mode) {
        return;
    }

    SCR_NetBarActiveWarning(&flags, &ping, &severity,
                            Cvar_ClampInteger(scr_netbar_notice_ms, 250, 5000));
    if (severity == NETBAR_SEV_NONE) {
        return;
    }

    notice = SCR_NetBarNotice(flags);
    if (!notice) {
        return;
    }

    if (ping && scr_netbar_labels->integer > 1) {
        Q_snprintf(buffer, sizeof(buffer), "%s  %u", notice, ping);
    } else {
        Q_strlcpy(buffer, notice, sizeof(buffer));
    }

    if (mode == 1) {
        y = bar_y - CHAR_HEIGHT - 1;
        if (y < 0) {
            y = bar_y + bar_h + 1;
        }
    } else {
        y = Cvar_ClampInteger(scr_netbar_notice_y, 0, scr.hud_height - CHAR_HEIGHT);
    }
    if (y + CHAR_HEIGHT > scr.hud_height) {
        return;
    }

    alpha = Cvar_ClampValue(scr_netbar_notice_alpha, 0, 1) *
            Cvar_ClampValue(scr_alpha, 0, 1);
    R_SetAlpha(alpha);
    if (severity >= NETBAR_SEV_CRIT) {
        R_SetColor(MakeColor(255, 64, 48, alpha * 255));
    } else {
        R_SetColor(MakeColor(255, 210, 64, alpha * 255));
    }

    SCR_DrawString(scr.hud_width / 2, y, UI_CENTER | UI_NOSHADOW, buffer);
    R_ClearColor();
}

void SH_NetBar_Draw(void)
{
    int mode, ping_mode, height, y, history, count, n, x, h, color, segment_start;
    unsigned now, age, stall_ms, stall_crit_ms;
    float global_alpha, alpha, age_scale;

    mode = Cvar_ClampInteger(scr_netbar, 0, 3);
    if (!mode || !cls.netchan.protocol || cls.demo.playback) {
        return;
    }

    height = Cvar_ClampInteger(scr_netbar_h, 1, 32);
    y = scr_netbar_y->integer;
    if (y < 0) {
        y += scr.hud_height - height + 1;
    }
    y = Q_clip(y, 0, scr.hud_height - height);

    history = Cvar_ClampInteger(scr_netbar_history, 500, 120000);
    ping_mode = Cvar_ClampInteger(scr_netbar_ping_mode, 0, 2);
    if (mode > 1 && !ping_mode) {
        ping_mode = 2;
    }

    global_alpha = Cvar_ClampValue(scr_alpha, 0, 1);
    now = cls.realtime;

    R_SetAlpha(Cvar_ClampValue(scr_netbar_alpha, 0, 1) * 0.35f * global_alpha);
    R_DrawFill8(0, y, scr.hud_width, height, 0);

    for (count = 0; count < NETBAR_SAMPLES; count++) {
        netbar_sample_t *sample = &netbar.samples[(netbar.head - 1 - count) & NETBAR_MASK];
        if (!sample->time) {
            break;
        }
        if (now - sample->time > (unsigned)history) {
            break;
        }
    }

    segment_start = -1;
    for (n = count - 1; n >= 0; n--) {
        netbar_sample_t *sample = &netbar.samples[(netbar.head - 1 - n) & NETBAR_MASK];
        int draw_x, draw_w;

        age = now - sample->time;
        x = (int)(((uint64_t)(history - age) * (scr.hud_width - 1)) / history);
        color = SCR_NetBarColor(sample);
        age_scale = 0.35f + 0.65f * (float)(history - age) / history;

        if (sample->severity >= NETBAR_SEV_CRIT) {
            alpha = Cvar_ClampValue(scr_netbar_bad_alpha, 0, 1) * age_scale;
        } else if (sample->severity == NETBAR_SEV_WARN) {
            alpha = Cvar_ClampValue(scr_netbar_bad_alpha, 0, 1) * 0.65f * age_scale;
        } else {
            alpha = Cvar_ClampValue(scr_netbar_alpha, 0, 1) * age_scale;
        }

        if (ping_mode) {
            int scale = max((int)netbar.avg_ping + max(Cvar_ClampInteger(scr_netwarn_spike_ms, 1, 999), 1) * 2, 1);
            h = Q_clip((int)sample->ping * height / scale, 1, height);
            if (ping_mode == 2 && sample->severity == NETBAR_SEV_NONE) {
                h = max(1, h / 2);
            }
        } else {
            h = height;
        }

        if (segment_start < 0) {
            draw_x = x;
            draw_w = 1;
        } else {
            draw_x = segment_start;
            draw_w = max(1, x - segment_start + 1);
        }
        segment_start = x + 1;
        if (draw_x >= scr.hud_width) {
            continue;
        }
        draw_w = min(draw_w, scr.hud_width - draw_x);

        R_SetAlpha(alpha * global_alpha);
        R_DrawFill8(draw_x, y + height - h, draw_w, h, color);
    }

    stall_ms = Cvar_ClampInteger(scr_netwarn_stall_ms, 0, 10000);
    if (stall_ms && cls.netchan.last_received && now - cls.netchan.last_received >= stall_ms) {
        stall_crit_ms = Cvar_ClampInteger(scr_netwarn_stall_crit_ms, stall_ms, 30000);
        alpha = Cvar_ClampValue(scr_netbar_bad_alpha, 0, 1);
        if (now - cls.netchan.last_received < stall_crit_ms) {
            alpha *= 0.65f;
        }

        R_SetAlpha(alpha * global_alpha);
        R_DrawFill8(scr.hud_width - 3, y, 3, height, NETBAR_COLOR_CRIT);
    }

    SCR_DrawNetBarNotice(y, height);
    SCR_DrawNetBarLabel(y);
    R_SetAlpha(global_alpha);
}
