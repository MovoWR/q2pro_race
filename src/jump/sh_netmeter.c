#include <src/client/client.h>
#include "sh_netmeter.h"
#include <src/client/ui/ui.h>

typedef enum {
    NETEVENT_SPIKE  = BIT(0),  // Sudden latency increase
    NETEVENT_JITTER = BIT(1),  // Unstable latency variation
    NETEVENT_LOSS   = BIT(2),  // Packet loss detected
    NETEVENT_CHOKE  = BIT(3),  // Updates limited by rate/bandwidth
    NETEVENT_PRED   = BIT(4)   // Client prediction disagrees with server
} netevent_flags_t;

typedef struct {
    unsigned    time;
    unsigned    ping;
    unsigned    flags;
} netmeter_sample_t;

#define NETMETER_SAMPLES  4096
#define NETMETER_MASK     (NETMETER_SAMPLES - 1)

#ifndef LAG_BASE
#define LAG_BASE    0xD5
#define LAG_WARN    0xDC
#define LAG_CRIT    0xF2
#endif

#define NET_COLOR_NORMAL LAG_BASE  // Normal network sample
#define NET_COLOR_WARN   LAG_WARN  // Warning-level network sample
#define NET_COLOR_CRIT   LAG_CRIT  // Critical network sample
#define NET_COLOR_LOSS   224       // Packet-loss sample
#define NET_COLOR_CHOKE  224       // Choked/rate-limited sample

static struct {
    netmeter_sample_t samples[NETMETER_SAMPLES];
    unsigned        head;
    unsigned        avg_ping;
    unsigned        display_ping;
    unsigned        last_ping;
    unsigned        jitter;
    unsigned        jitter_base;
    unsigned        ping_samples;
} netmeter;

void SH_NetMeter_Clear(void)
{
    memset(&netmeter, 0, sizeof(netmeter));
}

unsigned SH_NetMeter_GetAvgPing(void)
{
    return netmeter.avg_ping;
}

static void SH_NetMeter_PushSample(unsigned time, unsigned ping,
                                 unsigned flags)
{
    netmeter_sample_t *sample;

    sample = &netmeter.samples[netmeter.head & NETMETER_MASK];

    sample->time = time;
    sample->ping = ping;
    sample->flags = flags;

    netmeter.head++;
}

void SH_NetMeter_Sample(unsigned ping)
{
    unsigned events = 0;
    unsigned delta, spike_threshold, jitter_threshold;
    int spike, spike_pct, jitter;
    bool has_loss = false;

    if (!cls.netchan.protocol || cls.demo.playback) {
        return;
    }

    spike = Cvar_ClampInteger(sh_netwarn_spike_ms, 0, 999);
    spike_pct = Cvar_ClampInteger(sh_netwarn_spike_pct, 0, 1000);
    jitter = Cvar_ClampInteger(sh_netwarn_jitter_ms, 0, 999);

    if (cls.netchan.dropped || (cl.frameflags & FF_SERVERDROP)) {
        events |= NETEVENT_LOSS;
        has_loss = true;
    }

    if (cl.frameflags & FF_CLIENTDROP) {
        events |= NETEVENT_PRED;
        has_loss = true;
    }

    if (cl.frameflags & FF_SUPPRESSED) {
        events |= NETEVENT_CHOKE;
    }

    if (netmeter.ping_samples == 0) {
        netmeter.display_ping = ping;
    } else {
        netmeter.display_ping = (netmeter.display_ping * 3 + ping + 2) >> 2;
    }

    if (!has_loss) {
        // Calculate thresholds first based on stable average ping
        if (sh_netwarn_ping_adaptive->integer) {
            // Adaptive spike threshold: percent of average ping, with a minimum floor of 20ms to avoid noise
            unsigned spike_floor = spike > 20 ? spike : 20;
            spike_threshold = max(spike_floor, netmeter.avg_ping * (unsigned)spike_pct / 100);

            // Adaptive jitter threshold: with a minimum floor of 10ms to avoid noise
            unsigned jitter_floor = jitter > 10 ? jitter : 10;
            jitter_threshold = max(jitter_floor, netmeter.jitter_base + spike_threshold / 4);
        } else {
            spike_threshold = spike;
            jitter_threshold = jitter;
        }

        // Perform spike & jitter detection checks (only after warm-up phase of 32 samples)
        if (netmeter.ping_samples >= 32) {
            if (spike_threshold && ping > netmeter.avg_ping + spike_threshold) {
                events |= NETEVENT_SPIKE;
            }
            if (jitter_threshold && netmeter.jitter >= jitter_threshold) {
                events |= NETEVENT_JITTER;
            }
        }

        // Update statistics after detection checks so current spike does not inflate average beforehand
        if (netmeter.ping_samples == 0) {
            netmeter.avg_ping = ping;
        } else {
            netmeter.avg_ping = (netmeter.avg_ping * 7 + ping) >> 3;
        }

        if (netmeter.ping_samples > 0) {
            delta = abs((int)ping - (int)netmeter.last_ping);
            netmeter.jitter = (netmeter.jitter * 7 + delta) >> 3;

            if (netmeter.ping_samples == 1) {
                netmeter.jitter_base = netmeter.jitter;
            } else if (netmeter.jitter < netmeter.jitter_base + 2) {
                netmeter.jitter_base = (netmeter.jitter_base * 15 + netmeter.jitter) >> 4;
            }
        }

        netmeter.last_ping = ping;
        netmeter.ping_samples++;
    }

    SH_NetMeter_PushSample(cls.realtime, ping, events);
}

void SH_NetMeter_PredictionError(int len)
{
    // Ignored as client prediction errors are disabled
}

static int SCR_NetMeterColor(int mode, const netmeter_sample_t *sample)
{
    if (mode == 1) {
        if (sample->flags & NETEVENT_LOSS) {
            return Cvar_ClampInteger(sh_lagometer_color_loss_s2c, 0, 255);
        }
        if (sample->flags & NETEVENT_PRED) {
            return Cvar_ClampInteger(sh_lagometer_color_loss_c2s, 0, 255);
        }
        if (sample->flags & NETEVENT_SPIKE) {
            return Cvar_ClampInteger(sh_lagometer_color_spike, 0, 255);
        }
        if (sample->flags & NETEVENT_JITTER) {
            return Cvar_ClampInteger(sh_lagometer_color_jitter, 0, 255);
        }
        return Cvar_ClampInteger(sh_lagometer_color_normal, 0, 255);
    }
    else if (mode == 2) {
        if (sample->flags & NETEVENT_LOSS) {
            return Cvar_ClampInteger(sh_netgraph_color_loss_s2c, 0, 255);
        }
        if (sample->flags & NETEVENT_PRED) {
            return Cvar_ClampInteger(sh_netgraph_color_loss_c2s, 0, 255);
        }
        if (sample->flags & NETEVENT_SPIKE) {
            return Cvar_ClampInteger(sh_netgraph_color_spike, 0, 255);
        }
        if (sample->flags & NETEVENT_JITTER) {
            return Cvar_ClampInteger(sh_netgraph_color_jitter, 0, 255);
        }
        return Cvar_ClampInteger(sh_netgraph_color_normal, 0, 255);
    }
    else if (mode == 3) {
        if (sample->flags & NETEVENT_LOSS) {
            return Cvar_ClampInteger(sh_histogram_color_loss_s2c, 0, 255);
        }
        if (sample->flags & NETEVENT_PRED) {
            return Cvar_ClampInteger(sh_histogram_color_loss_c2s, 0, 255);
        }
        if (sample->flags & NETEVENT_SPIKE) {
            return Cvar_ClampInteger(sh_histogram_color_spike, 0, 255);
        }
        if (sample->flags & NETEVENT_JITTER) {
            return Cvar_ClampInteger(sh_histogram_color_jitter, 0, 255);
        }
        return Cvar_ClampInteger(sh_histogram_color_normal, 0, 255);
    }
    return 0;
}

static bool SCR_NetMeterNoticeEnabled(unsigned flag)
{
    switch (flag) {
    case NETEVENT_LOSS:
    case NETEVENT_PRED:
        return sh_netalert_loss->integer != 0;
    case NETEVENT_JITTER:
        return sh_netalert_jitter->integer != 0;
    case NETEVENT_SPIKE:
        return sh_netalert_spike->integer != 0;
    default:
        return false;
    }
}

static unsigned SCR_NetMeterNoticeFlags(unsigned flags)
{
    unsigned ordered[] = {
        NETEVENT_LOSS,
        NETEVENT_PRED,
        NETEVENT_JITTER,
        NETEVENT_SPIKE
    };
    size_t i;

    for (i = 0; i < q_countof(ordered); i++) {
        if ((flags & ordered[i]) && SCR_NetMeterNoticeEnabled(ordered[i])) {
            return ordered[i];
        }
    }

    return 0;
}

static const char *SCR_NetMeterNotice(unsigned flags)
{
    if (flags & NETEVENT_LOSS) {
        return "PACKET LOSS";
    }
    if (flags & NETEVENT_PRED) {
        return "CLIENT DROP";
    }
    if (flags & NETEVENT_JITTER) {
        return "NETWORK JITTER";
    }
    if (flags & NETEVENT_SPIKE) {
        return "PING SPIKE";
    }
    return NULL;
}

static int SCR_EventPriority(unsigned flag)
{
    if (flag & NETEVENT_LOSS) return 5;
    if (flag & NETEVENT_PRED) return 4;
    if (flag & NETEVENT_CHOKE) return 3;
    if (flag & NETEVENT_JITTER) return 2;
    if (flag & NETEVENT_SPIKE) return 1;
    return 0;
}

static void SCR_NetMeterActiveWarning(unsigned *flags, unsigned *ping, unsigned hold,
                                    bool notice_only)
{
    unsigned count, max_samples, now = cls.realtime;
    int best_priority = 0;

    *flags = 0;
    *ping = netmeter.last_ping;

    max_samples = min(NETMETER_SAMPLES, netmeter.head);
    for (count = 0; count < max_samples; count++) {
        netmeter_sample_t *sample = &netmeter.samples[(netmeter.head - 1 - count) & NETMETER_MASK];
        unsigned sample_flags;

        if (now - sample->time > hold) {
            break;
        }

        sample_flags = notice_only ? SCR_NetMeterNoticeFlags(sample->flags) : sample->flags;
        if (!sample_flags) {
            continue;
        }

        int priority = SCR_EventPriority(sample_flags);
        if (priority > best_priority) {
            best_priority = priority;
            *flags = sample_flags;
            *ping = sample->ping;
        }
    }


}



static void SCR_GetNetAlertColor(int color_index, float alpha, float *r, float *g, float *b)
{
    *r = 255.0f; *g = 255.0f; *b = 255.0f;
    switch (color_index) {
    case 0: // Gray
        *r = 180.0f; *g = 180.0f; *b = 180.0f;
        break;
    case 1: // Red
        *r = 255.0f; *g = 64.0f; *b = 48.0f;
        break;
    case 2: // Green
        *r = 64.0f; *g = 255.0f; *b = 64.0f;
        break;
    case 3: // Yellow
        *r = 255.0f; *g = 210.0f; *b = 64.0f;
        break;
    case 4: // Blue
        *r = 64.0f; *g = 96.0f; *b = 255.0f;
        break;
    case 5: // Cyan
        *r = 64.0f; *g = 255.0f; *b = 255.0f;
        break;
    case 6: // Magenta
        *r = 255.0f; *g = 64.0f; *b = 255.0f;
        break;
    case 7: // White
        *r = 255.0f; *g = 255.0f; *b = 255.0f;
        break;
    case 8: // Orange
        *r = 255.0f; *g = 128.0f; *b = 0.0f;
        break;
    case 9: // Purple
        *r = 128.0f; *g = 0.0f; *b = 255.0f;
        break;
    default:
        break;
    }
}

static void SCR_DrawNetMeterNotice(int bar_y, int bar_h)
{
    unsigned flags, ping;
    const char *notice;
    char buffer[48];
    int x, y, draw_x, draw_y;
    unsigned align_flags = UI_NOSHADOW;
    float alpha, r, g, b;

    bool is_test = false;

    if (!sh_netalert->integer) {
        return;
    }

    if (uis.activeMenu && uis.activeMenu->name && strcmp(uis.activeMenu->name, "jumpnetalerts") == 0) {
        is_test = true;
    }

    if (is_test) {
        flags = NETEVENT_LOSS;
        ping = 99;
        notice = "TEST ALERT";
    } else {
        SCR_NetMeterActiveWarning(&flags, &ping,
                                Cvar_ClampInteger(sh_netalert_duration_ms, 250, 5000),
                                true);
        if (!flags) {
            return;
        }
        notice = SCR_NetMeterNotice(flags);
        if (!notice) {
            return;
        }
    }

    Q_strlcpy(buffer, notice, sizeof(buffer));

    x = sh_netalert_x->integer;
    if (x == 0) {
        draw_x = scr.hud_width / 2;
        align_flags |= UI_CENTER;
    } else if (x < 0) {
        draw_x = scr.hud_width + x + 1;
        align_flags |= UI_RIGHT;
    } else {
        draw_x = x;
    }

    y = sh_netalert_y->integer;
    if (y == 0) {
        draw_y = (scr.hud_height - CHAR_HEIGHT) / 2;
    } else if (y < 0) {
        draw_y = scr.hud_height + y - CHAR_HEIGHT + 1;
    } else {
        draw_y = y;
    }

    if (draw_y + CHAR_HEIGHT > scr.hud_height || draw_y < 0) {
        return;
    }

    alpha = Cvar_ClampValue(sh_netalert_alpha, 0, 1) *
            Cvar_ClampValue(scr_alpha, 0, 1);
    R_SetAlpha(alpha);

    int color_val = Cvar_ClampInteger(sh_netalert_color, 0, 255);
    if (color_val == 7) {
        if (flags & (NETEVENT_LOSS | NETEVENT_PRED)) {
            r = 255.0f; g = 64.0f; b = 48.0f;
        } else {
            r = 255.0f; g = 210.0f; b = 64.0f;
        }
    } else {
        SCR_GetNetAlertColor(color_val, alpha, &r, &g, &b);
    }

    R_SetColor(MakeColor(r, g, b, alpha * 255));
    SCR_DrawString(draw_x, draw_y, align_flags, buffer);
    R_ClearColor();
}

static void SCR_DrawNetMeterLagometer(float global_alpha)
{
    int i, v, c, v_min, v_max, v_range;
    int x, y, draw_x, draw_y, draw_w, draw_h;

    x = sh_lagometer_x->integer;
    y = sh_lagometer_y->integer;
    draw_w = 48;
    draw_h = 48;

    if (x < 0) {
        draw_x = scr.hud_width + x - draw_w + 1;
    } else {
        draw_x = x;
    }
    draw_x = Q_clip(draw_x, 0, scr.hud_width - draw_w);

    if (y == -1) {
        draw_y = scr.hud_height - draw_h + 1;
    } else if (y < 0) {
        draw_y = scr.hud_height + y - draw_h + 1;
    } else {
        draw_y = y;
    }
    draw_y = Q_clip(draw_y, 0, scr.hud_height - draw_h);

    int min_val = sh_netmeter_min_ms->integer;
    int max_val = sh_netmeter_max_ms->integer;
    if (min_val < 0) {
        min_val = 0;
    }
    if (max_val < min_val) {
        max_val = min_val;
    }

    if (sh_netmeter_adaptive->integer) {
        v_min = 0;
        v_max = max((int)netmeter.avg_ping * 2, 150);
    } else {
        v_min = min_val;
        v_max = max_val;
    }

    v_range = v_max - v_min;
    if (v_range < 1) {
        v_range = 1;
    }

    for (i = 0; i < LAG_WIDTH; i++) {
        int idx = (netmeter.head - 1 - i) & NETMETER_MASK;
        netmeter_sample_t *sample = &netmeter.samples[idx];
        if (!sample->time) {
            break;
        }

        int ping_val = sample->ping;
        c = SCR_NetMeterColor(1, sample);

        bool is_bad = (sample->flags & (NETEVENT_LOSS | NETEVENT_PRED | NETEVENT_SPIKE | NETEVENT_JITTER));
        bool is_loss = (sample->flags & (NETEVENT_LOSS | NETEVENT_PRED));

        if (is_loss) {
            v = LAG_HEIGHT;
        } else {
            v = Q_clip((ping_val - v_min) * LAG_HEIGHT / v_range, 0, LAG_HEIGHT);
        }

        float alpha_val = (is_bad ? Cvar_ClampValue(sh_lagometer_bad_alpha, 0, 1) : Cvar_ClampValue(sh_lagometer_alpha, 0, 1)) * global_alpha;
        R_SetAlpha(alpha_val);
        R_DrawFill8(draw_x + LAG_WIDTH - i - 1, draw_y + LAG_HEIGHT - v, 1, v, c);
    }

    // Draw phone jack
    if (cls.netchan.outgoing_sequence - cls.netchan.incoming_acknowledged >= CMD_BACKUP) {
        if ((cls.realtime >> 8) & 3) {
            R_DrawStretchPic(draw_x, draw_y, LAG_WIDTH, LAG_HEIGHT, scr.net_pic);
        }
    }

    SCR_DrawNetMeterNotice(draw_y, draw_h);
    R_SetAlpha(global_alpha);
}

static void SCR_DrawNetMeterNetgraph(float global_alpha)
{
    int a, idx, w, draw_h, y, draw_y, c;
    int v_min, v_max, v_range;
    float graph_alpha;

    w = scr.hud_width;
    draw_h = Cvar_ClampInteger(sh_netgraph_height, 1, scr.hud_height);
    y = sh_netgraph_y->integer;

    if (y < 0) {
        draw_y = scr.hud_height + y - draw_h + 1;
    } else {
        draw_y = y;
    }
    draw_y = Q_clip(draw_y, 0, scr.hud_height - draw_h);

    int min_val = sh_netmeter_min_ms->integer;
    int max_val = sh_netmeter_max_ms->integer;
    if (min_val < 0) {
        min_val = 0;
    }
    if (max_val < min_val) {
        max_val = min_val;
    }

    if (sh_netmeter_adaptive->integer) {
        v_min = 0;
        v_max = max((int)netmeter.avg_ping * 2, 150);
    } else {
        v_min = min_val;
        v_max = max_val;
    }

    v_range = v_max - v_min;
    if (v_range < 1) {
        v_range = 1;
    }

    graph_alpha = Cvar_ClampValue(sh_netgraph_alpha, 0, 1) * global_alpha;

    for (a = 0; a < w && a < NETMETER_SAMPLES; a++) {
        idx = (netmeter.head - 1 - a) & NETMETER_MASK;
        netmeter_sample_t *sample = &netmeter.samples[idx];
        if (!sample->time) {
            break;
        }

        float v = (float)sample->ping;
        c = SCR_NetMeterColor(2, sample);

        bool is_bad = (sample->flags & (NETEVENT_LOSS | NETEVENT_PRED | NETEVENT_SPIKE | NETEVENT_JITTER));
        bool is_loss = (sample->flags & (NETEVENT_LOSS | NETEVENT_PRED));

        int h;
        if (is_loss) {
            h = draw_h;
        } else {
            h = (int)((v - v_min) * draw_h / v_range);
            h = Q_clip(h, 1, draw_h);
        }

        float column_alpha = (is_bad ? global_alpha : graph_alpha);

        R_SetAlpha(column_alpha);
        R_DrawFill8(w - 1 - a, draw_y + draw_h - h, 1, h, c);
    }

    SCR_DrawNetMeterNotice(draw_y, draw_h);
    R_SetAlpha(global_alpha);
}

static void SCR_DrawNetMeterHistogram(float global_alpha, unsigned now)
{
    int x, y, draw_x, draw_y, draw_w, draw_h, history, count, n, h, color;
    int v_min, v_max, v_range;
    unsigned age;
    float alpha, age_scale;

    switch (Cvar_ClampInteger(sh_histogram_width_mode, 0, 2)) {
    case 2:
        draw_w = max(10, scr.hud_width);
        break;
    case 1:
        draw_w = Q_clip(scr.hud_width / 2, 10, max(10, scr.hud_width));
        break;
    default:
        draw_w = Cvar_ClampInteger(sh_histogram_width, 10, max(10, scr.hud_width));
        break;
    }
    draw_h = Cvar_ClampInteger(sh_histogram_height, 1, max(1, scr.hud_height));
    x = sh_histogram_x->integer;
    y = sh_histogram_y->integer;

    if (x < 0) {
        draw_x = scr.hud_width + x - draw_w + 1;
    } else {
        draw_x = x;
    }
    draw_x = Q_clip(draw_x, 0, scr.hud_width - draw_w);

    if (y == -1) {
        draw_y = scr.hud_height - draw_h + 1;
    } else if (y < 0) {
        draw_y = scr.hud_height + y - draw_h + 1;
    } else {
        draw_y = y;
    }
    draw_y = Q_clip(draw_y, 0, scr.hud_height - draw_h);
    history = Cvar_ClampInteger(sh_histogram_history, 500, 120000);

    int min_val = sh_netmeter_min_ms->integer;
    int max_val = sh_netmeter_max_ms->integer;
    if (min_val < 0) {
        min_val = 0;
    }
    if (max_val < min_val) {
        max_val = min_val;
    }

    if (sh_netmeter_adaptive->integer) {
        v_min = 0;
        v_max = max((int)netmeter.avg_ping * 2, 150);
    } else {
        v_min = min_val;
        v_max = max_val;
    }

    v_range = v_max - v_min;
    if (v_range < 1) {
        v_range = 1;
    }

    R_SetAlpha(Cvar_ClampValue(sh_histogram_bg_alpha, 0, 1) * global_alpha);
    R_DrawFill8(draw_x, draw_y, draw_w, draw_h, Cvar_ClampInteger(sh_histogram_color_bg, 0, 255));

    for (count = 0; count < NETMETER_SAMPLES; count++) {
        netmeter_sample_t *sample = &netmeter.samples[(netmeter.head - 1 - count) & NETMETER_MASK];
        if (!sample->time || now - sample->time > (unsigned)history) {
            break;
        }
    }

    for (n = count - 1; n >= 0; n--) {
        netmeter_sample_t *sample = &netmeter.samples[(netmeter.head - 1 - n) & NETMETER_MASK];
        int sample_x, span_end, span_w, fill_w;

        age = now - sample->time;
        if (Cvar_ClampInteger(sh_histogram_spacing_mode, 0, 1) == 1) {
            int sample_pos = count - 1 - n;
            int sample_range = max(1, count - 1);

            sample_x = draw_x + (int)(((uint64_t)sample_pos * (draw_w - 1)) / sample_range);
            if (n > 0) {
                span_end = draw_x + (int)(((uint64_t)(sample_pos + 1) * (draw_w - 1)) / sample_range);
            } else {
                span_end = draw_x + draw_w;
            }
        } else {
            sample_x = draw_x + (int)(((uint64_t)(history - age) * (draw_w - 1)) / history);
            if (n > 0) {
                netmeter_sample_t *next_sample = &netmeter.samples[(netmeter.head - n) & NETMETER_MASK];
                unsigned next_age = now - next_sample->time;
                span_end = draw_x + (int)(((uint64_t)(history - next_age) * (draw_w - 1)) / history);
            } else {
                span_end = draw_x + draw_w;
            }
        }
        span_w = max(1, span_end - sample_x);
        if (Cvar_ClampInteger(sh_histogram_fill_mode, 0, 1) == 1) {
            fill_w = span_w;
        } else {
            int bar_w = span_w <= 2 ? 1 : Q_clip(span_w / 2, 1, 8);
            fill_w = min(bar_w, draw_x + draw_w - sample_x);
            fill_w = max(1, fill_w);
        }
        color = SCR_NetMeterColor(3, sample);
        age_scale = 0.35f + 0.65f * (float)(history - age) / history;

        bool is_bad = (sample->flags & (NETEVENT_LOSS | NETEVENT_PRED | NETEVENT_SPIKE | NETEVENT_JITTER));

        if (is_bad) {
            alpha = Cvar_ClampValue(sh_histogram_bad_alpha, 0, 1) * age_scale;
        } else {
            alpha = Cvar_ClampValue(sh_histogram_alpha, 0, 1) * age_scale;
        }

        bool is_loss = (sample->flags & (NETEVENT_LOSS | NETEVENT_PRED));
        if (is_loss) {
            h = draw_h;
        } else {
            h = Q_clip(((int)sample->ping - v_min) * draw_h / v_range, 1, draw_h);
        }

        R_SetAlpha(alpha * global_alpha);
        R_DrawFill8(sample_x, draw_y + draw_h - h, fill_w, h, color);
    }

    if (sh_histogram_ping && sh_histogram_ping->integer && netmeter.ping_samples > 0) {
        char ping_str[16];
        Q_scnprintf(ping_str, sizeof(ping_str), "%u", netmeter.display_ping);
        R_SetAlpha(global_alpha);
        SCR_DrawString(draw_x, draw_y, UI_LEFT, ping_str);
    }

    SCR_DrawNetMeterNotice(draw_y, draw_h);
    R_SetAlpha(global_alpha);
}


void SH_NetMeter_Draw(void)
{
    int mode;
    float global_alpha;
    unsigned now;

    bool is_test = false;

    if (uis.activeMenu && uis.activeMenu->name && strcmp(uis.activeMenu->name, "jumpnetalerts") == 0) {
        is_test = true;
    }

    mode = Cvar_ClampInteger(sh_netmeter, 0, 4);
    if (!mode || !cls.netchan.protocol || cls.demo.playback) {
        if (is_test && sh_netalert->integer) {
            SCR_DrawNetMeterNotice(0, 0);
        }
        return;
    }

    global_alpha = Cvar_ClampValue(scr_alpha, 0, 1);
    now = cls.realtime;

    switch (mode) {
    case 1:
        SCR_DrawNetMeterLagometer(global_alpha);
        break;
    case 2:
        SCR_DrawNetMeterNetgraph(global_alpha);
        break;
    case 3:
        SCR_DrawNetMeterHistogram(global_alpha, now);
        break;
    default:
        break;
    }
}
