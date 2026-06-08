/*
 * SH_Init() — central registration for all jump-mod cvars.
 *
 * Consolidates Cvar_Get() calls that were previously scattered across
 * src/client/main.c (strafe helper / race line) and
 * src/client/screen.c → src/jump/sh_netmeter.c (network meter).
 *
 * Called once from CL_Init() in src/client/main.c.
 */

#include "strafe_helper.h"
#include "sh_netmeter.h"

// ========================================================================
// Strafe helper cvars
// ========================================================================

cvar_t *cl_drawStrafeHelper;
cvar_t *cl_strafeHelperCenter;
cvar_t *cl_strafeHelperCenterMarker;
cvar_t *cl_strafeHelperHeight;
cvar_t *cl_strafeHelperScale;
cvar_t *cl_strafeHelperY;
cvar_t *cl_strafehelperUps;
cvar_t *cl_strafehelperUpsScale;
cvar_t *cl_strafehelperUpsY;
cvar_t *cl_strafehelperUpsShadow;
cvar_t *cl_strafehelperUpsHideZero;
cvar_t *cl_strafehelperUpsColorMode;
cvar_t *cl_strafehelperUpsColorGain;
cvar_t *cl_strafehelperUpsColorLoss;
cvar_t *cl_strafehelperUpsColorNeutral;
cvar_t *cl_strafehelperUpsFormat;
cvar_t *cl_strafehelperUps3D;
cvar_t *cl_strafehelperAlpha;
cvar_t *cl_strafehelperBarStyle;
cvar_t *cl_strafehelperSmoothing;
cvar_t *cl_strafehelperSmoothingMode;
cvar_t *cl_strafehelperNerdStats;

// width
cvar_t *cl_strafehelper_center_width;
cvar_t *cl_strafehelper_optimal_width;
cvar_t *cl_strafehelper_optimal_outline;

// color settings
cvar_t *cl_strafehelper_color_accelerating;
cvar_t *cl_strafehelper_color_optimal;
cvar_t *cl_strafehelper_color_centermarker;
cvar_t *cl_strafehelper_color_nerdstats;

// race line
cvar_t *cl_race_color;
cvar_t *cl_race_life;
cvar_t *cl_race_width;
cvar_t *cl_race_alpha;

cvar_t *cl_input_keys;

// ========================================================================
// Netmeter cvars
// ========================================================================

cvar_t *sh_netmeter;

cvar_t *sh_netwarn_ping_adaptive;
cvar_t *sh_netwarn_spike_ms;
cvar_t *sh_netwarn_spike_pct;
cvar_t *sh_netwarn_jitter_ms;

cvar_t *sh_netalert;
cvar_t *sh_netalert_x;
cvar_t *sh_netalert_y;
cvar_t *sh_netalert_color;
cvar_t *sh_netalert_alpha;
cvar_t *sh_netalert_duration_ms;
cvar_t *sh_netalert_loss;
cvar_t *sh_netalert_jitter;
cvar_t *sh_netalert_spike;

cvar_t *sh_lagometer_x;
cvar_t *sh_lagometer_y;
cvar_t *sh_netmeter_min_ms;
cvar_t *sh_netmeter_max_ms;
cvar_t *sh_netmeter_adaptive;
cvar_t *sh_lagometer_color_normal;
cvar_t *sh_lagometer_color_spike;
cvar_t *sh_lagometer_color_jitter;
cvar_t *sh_lagometer_color_loss_s2c;
cvar_t *sh_lagometer_color_loss_c2s;
cvar_t *sh_lagometer_alpha;
cvar_t *sh_lagometer_bad_alpha;

cvar_t *sh_netgraph_y;
cvar_t *sh_netgraph_height;
cvar_t *sh_netgraph_alpha;
cvar_t *sh_netgraph_color_normal;
cvar_t *sh_netgraph_color_spike;
cvar_t *sh_netgraph_color_jitter;
cvar_t *sh_netgraph_color_loss_s2c;
cvar_t *sh_netgraph_color_loss_c2s;

cvar_t *sh_histogram_x;
cvar_t *sh_histogram_y;
cvar_t *sh_histogram_width_mode;
cvar_t *sh_histogram_width;
cvar_t *sh_histogram_height;
cvar_t *sh_histogram_fill_mode;
cvar_t *sh_histogram_spacing_mode;
cvar_t *sh_histogram_bg_alpha;
cvar_t *sh_histogram_color_bg;
cvar_t *sh_histogram_color_normal;
cvar_t *sh_histogram_color_spike;
cvar_t *sh_histogram_color_jitter;
cvar_t *sh_histogram_color_loss_s2c;
cvar_t *sh_histogram_color_loss_c2s;
cvar_t *sh_histogram_alpha;
cvar_t *sh_histogram_bad_alpha;
cvar_t *sh_histogram_history;
cvar_t *sh_histogram_ping;

// ========================================================================
// SH_Init — register all jump-mod cvars
// ========================================================================

void SH_Init(void)
{
    // Strafe helper
    cl_drawStrafeHelper = Cvar_Get("sh_draw", "1", CVAR_ARCHIVE);
    cl_strafeHelperCenter = Cvar_Get("sh_center", "1", CVAR_ARCHIVE);
    cl_strafeHelperCenterMarker = Cvar_Get("sh_centermarker", "1", CVAR_ARCHIVE);
    cl_strafeHelperHeight = Cvar_Get("sh_height", "15", CVAR_ARCHIVE);
    cl_strafeHelperScale = Cvar_Get("sh_scale", "1.500000", CVAR_ARCHIVE);
    cl_strafeHelperY = Cvar_Get("sh_y", "100", CVAR_ARCHIVE);
    cl_strafehelperUps = Cvar_Get("sh_ups", "1", CVAR_ARCHIVE);
    cl_strafehelperUpsScale = Cvar_Get("sh_ups_scale", "1", CVAR_ARCHIVE);
    cl_strafehelperUpsY = Cvar_Get("sh_ups_y", "-5", CVAR_ARCHIVE);
    cl_strafehelperUpsShadow = Cvar_Get("sh_ups_shadow", "1", CVAR_ARCHIVE);
    cl_strafehelperUpsHideZero = Cvar_Get("sh_ups_hide_zero", "1", CVAR_ARCHIVE);
    cl_strafehelperUpsColorMode = Cvar_Get("sh_ups_color_mode", "dynamic", CVAR_ARCHIVE);
    cl_strafehelperUpsColorGain = Cvar_Get("sh_ups_color_gain", "0 255 0 255", CVAR_ARCHIVE);
    cl_strafehelperUpsColorLoss = Cvar_Get("sh_ups_color_loss", "255 0 0 255", CVAR_ARCHIVE);
    cl_strafehelperUpsColorNeutral = Cvar_Get("sh_ups_color_neutral", "255 255 255 255", CVAR_ARCHIVE);
    cl_strafehelperUpsFormat = Cvar_Get("sh_ups_format", "plain", CVAR_ARCHIVE);
    cl_strafehelperUps3D = Cvar_Get("sh_ups_3d", "0", CVAR_ARCHIVE);
    cl_strafehelperAlpha = Cvar_Get("sh_alpha", "0.500000", CVAR_ARCHIVE);
    cl_strafehelperBarStyle = Cvar_Get("sh_bar_style", "gradient", CVAR_ARCHIVE);
    cl_strafehelperSmoothing = Cvar_Get("sh_smoothing", "0", CVAR_ARCHIVE);
    cl_strafehelperSmoothingMode = Cvar_Get("sh_smoothing_mode", "1", CVAR_ARCHIVE);
    cl_strafehelper_center_width = Cvar_Get("sh_center_width", "2", CVAR_ARCHIVE);
    cl_strafehelper_optimal_width = Cvar_Get("sh_optimal_width", "2", CVAR_ARCHIVE);
    cl_strafehelper_optimal_outline = Cvar_Get("sh_optimal_outline", "1", CVAR_ARCHIVE);
    cl_strafehelper_color_accelerating = Cvar_Get("sh_color_accelerating", "0 128 0 128", CVAR_ARCHIVE);
    cl_strafehelper_color_optimal = Cvar_Get("sh_color_optimal", "255 215 0 255", CVAR_ARCHIVE);
    cl_strafehelper_color_centermarker = Cvar_Get("sh_color_centermarker", "255 255 255 255", CVAR_ARCHIVE);
    cl_strafehelper_color_nerdstats = Cvar_Get("sh_color_nerdstats", "50 0 50 100", CVAR_ARCHIVE);
    cl_strafehelperNerdStats = Cvar_Get("sh_nerdstats", "0", CVAR_ARCHIVE);

    // Race line
    cl_race_width = Cvar_Get("race_width", "5", CVAR_ARCHIVE);
    cl_race_color = Cvar_Get("race_color", "0 255 0", CVAR_ARCHIVE);
    cl_race_alpha = Cvar_Get("race_alpha", "0.5", CVAR_ARCHIVE);
    cl_race_life = Cvar_Get("race_life", "500", CVAR_ARCHIVE);

    cl_input_keys = Cvar_Get("cl_input_keys", "", 0);

    // Netmeter core
    sh_netmeter = Cvar_Get("sh_netmeter", "3", CVAR_ARCHIVE);

    sh_netwarn_ping_adaptive = Cvar_Get("sh_netwarn_ping_adaptive", "1", CVAR_ARCHIVE);
    sh_netwarn_spike_ms = Cvar_Get("sh_netwarn_spike_ms", "300", CVAR_ARCHIVE);
    sh_netwarn_spike_pct = Cvar_Get("sh_netwarn_spike_pct", "100", CVAR_ARCHIVE);
    sh_netwarn_jitter_ms = Cvar_Get("sh_netwarn_jitter_ms", "100", CVAR_ARCHIVE);

    sh_netalert = Cvar_Get("sh_netalert", "1", CVAR_ARCHIVE);
    sh_netalert_x = Cvar_Get("sh_netalert_x", "0", CVAR_ARCHIVE);
    sh_netalert_y = Cvar_Get("sh_netalert_y", "353", CVAR_ARCHIVE);
    sh_netalert_color = Cvar_Get("sh_netalert_color", "1", CVAR_ARCHIVE);
    sh_netalert_alpha = Cvar_Get("sh_netalert_alpha", "1", CVAR_ARCHIVE);
    sh_netalert_duration_ms = Cvar_Get("sh_netalert_duration_ms", "2000", CVAR_ARCHIVE);
    sh_netalert_loss = Cvar_Get("sh_netalert_loss", "1", CVAR_ARCHIVE);
    sh_netalert_jitter = Cvar_Get("sh_netalert_jitter", "1", CVAR_ARCHIVE);
    sh_netalert_spike = Cvar_Get("sh_netalert_spike", "1", CVAR_ARCHIVE);

    sh_lagometer_x = Cvar_Get("sh_lagometer_x", "166", CVAR_ARCHIVE);
    sh_lagometer_y = Cvar_Get("sh_lagometer_y", "-1", CVAR_ARCHIVE);
    sh_netmeter_min_ms = Cvar_Get("sh_netmeter_min_ms", "0", CVAR_ARCHIVE);
    sh_netmeter_max_ms = Cvar_Get("sh_netmeter_max_ms", "150", CVAR_ARCHIVE);
    sh_netmeter_adaptive = Cvar_Get("sh_netmeter_adaptive", "1", CVAR_ARCHIVE);
    sh_lagometer_color_normal = Cvar_Get("sh_lagometer_color_normal", "212", CVAR_ARCHIVE);
    sh_lagometer_color_spike = Cvar_Get("sh_lagometer_color_spike", "225", CVAR_ARCHIVE);
    sh_lagometer_color_jitter = Cvar_Get("sh_lagometer_color_jitter", "220", CVAR_ARCHIVE);
    sh_lagometer_color_loss_s2c = Cvar_Get("sh_lagometer_color_loss_s2c", "231", CVAR_ARCHIVE);
    sh_lagometer_color_loss_c2s = Cvar_Get("sh_lagometer_color_loss_c2s", "243", CVAR_ARCHIVE);
    sh_lagometer_alpha = Cvar_Get("sh_lagometer_alpha", "0.300000", CVAR_ARCHIVE);
    sh_lagometer_bad_alpha = Cvar_Get("sh_lagometer_bad_alpha", "1", CVAR_ARCHIVE);

    sh_netgraph_y = Cvar_Get("sh_netgraph_y", "-1", CVAR_ARCHIVE);
    sh_netgraph_height = Cvar_Get("sh_netgraph_height", "15", CVAR_ARCHIVE);
    sh_netgraph_alpha = Cvar_Get("sh_netgraph_alpha", "0.75", CVAR_ARCHIVE);
    sh_netgraph_color_normal = Cvar_Get("sh_netgraph_color_normal", "213", CVAR_ARCHIVE);
    sh_netgraph_color_spike = Cvar_Get("sh_netgraph_color_spike", "208", CVAR_ARCHIVE);
    sh_netgraph_color_jitter = Cvar_Get("sh_netgraph_color_jitter", "220", CVAR_ARCHIVE);
    sh_netgraph_color_loss_s2c = Cvar_Get("sh_netgraph_color_loss_s2c", "233", CVAR_ARCHIVE);
    sh_netgraph_color_loss_c2s = Cvar_Get("sh_netgraph_color_loss_c2s", "11", CVAR_ARCHIVE);

    sh_histogram_x = Cvar_Get("sh_histogram_x", "0", CVAR_ARCHIVE);
    sh_histogram_y = Cvar_Get("sh_histogram_y", "-1", CVAR_ARCHIVE);
    sh_histogram_width_mode = Cvar_Get("sh_histogram_width_mode", "2", CVAR_ARCHIVE);
    sh_histogram_width = Cvar_Get("sh_histogram_width", "1280", CVAR_ARCHIVE);
    sh_histogram_height = Cvar_Get("sh_histogram_height", "15", CVAR_ARCHIVE);
    sh_histogram_fill_mode = Cvar_Get("sh_histogram_fill_mode", "0", CVAR_ARCHIVE);
    sh_histogram_spacing_mode = Cvar_Get("sh_histogram_spacing_mode", "0", CVAR_ARCHIVE);
    sh_histogram_bg_alpha = Cvar_Get("sh_histogram_bg_alpha", "0.150000", CVAR_ARCHIVE);
    sh_histogram_color_bg = Cvar_Get("sh_histogram_color_bg", "0", CVAR_ARCHIVE);
    sh_histogram_color_normal = Cvar_Get("sh_histogram_color_normal", "209", CVAR_ARCHIVE);
    sh_histogram_color_spike = Cvar_Get("sh_histogram_color_spike", "220", CVAR_ARCHIVE);
    sh_histogram_color_jitter = Cvar_Get("sh_histogram_color_jitter", "215", CVAR_ARCHIVE);
    sh_histogram_color_loss_s2c = Cvar_Get("sh_histogram_color_loss_s2c", "227", CVAR_ARCHIVE);
    sh_histogram_color_loss_c2s = Cvar_Get("sh_histogram_color_loss_c2s", "241", CVAR_ARCHIVE);
    sh_histogram_alpha = Cvar_Get("sh_histogram_alpha", "1", CVAR_ARCHIVE);
    sh_histogram_bad_alpha = Cvar_Get("sh_histogram_bad_alpha", "1", CVAR_ARCHIVE);
    sh_histogram_history = Cvar_Get("sh_histogram_history_ms", "52000", CVAR_ARCHIVE);
    sh_histogram_ping = Cvar_Get("sh_histogram_ping", "1", CVAR_ARCHIVE);
}
