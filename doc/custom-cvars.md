
## Input and Movement

| Cvar | Default | Description |
| --- | --- | --- |
| `cl_input_keys` | empty | Read-only string with currently pressed movement/jump keys. |
| `cl_preserve_jump_edges` | `0` | Preserves jump key edges across generated user commands. |
| `m_r1q2` | `0` | Enables R1Q2-style mouse handling behavior. |

## Client Smoothing and FPS Behavior

| Cvar | Default | Description |
| --- | --- | --- |
| `cl_step_smoothing_mode` | `q2pro` | Selects the step smoothing behavior used by the client. |

## Strafe Helper HUD

| Cvar | Default | Description |
| --- | --- | --- |
| `sh_draw` | `0` | Enables the strafe helper HUD bar. |
| `sh_center` | `1` | Draws the center reference zone. |
| `sh_centermarker` | `1` | Draws the center marker. |
| `sh_height` | `20` | Height of the strafe helper bar. |
| `sh_scale` | `1.5` | Visual scale of the helper. |
| `sh_y` | `100` | Vertical position of the helper. |
| `sh_alpha` | `1` | Overall helper alpha multiplier. |
| `sh_bar_style` | `gradient` | Bar rendering style. |
| `sh_center_width` | `2.0` | Width of the center marker/zone. |
| `sh_optimal_width` | `2.0` | Width of the optimal acceleration zone. |
| `sh_optimal_outline` | `0` | Draws the optimal zone as an outline. |
| `sh_smoothing` | `0` | Amount of helper smoothing. |
| `sh_smoothing_mode` | `1` | Smoothing curve/mode. |

## Strafe Helper Colors and NerdStats

| Cvar | Default | Description |
| --- | --- | --- |
| `sh_color_accelerating` | `0 128 255 80` | RGBA color used while accelerating. |
| `sh_color_optimal` | `0 255 0 255` | RGBA color for the optimal acceleration zone. |
| `sh_color_centermarker` | `255 255 255 255` | RGBA color for the center marker. |
| `sh_color_nerdstats` | `50 0 50 100` | RGBA background/color value used by NerdStats. |
| `sh_nerdstats` | `0` | Enables detailed strafe helper NerdStats display. |

## UPS Display

| Cvar | Default | Description |
| --- | --- | --- |
| `sh_ups` | `0` | Enables the centered UPS display. |
| `sh_ups_scale` | `1` | Text scale for the UPS display. |
| `sh_ups_y` | `0` | Vertical offset for the UPS display. |
| `sh_ups_shadow` | `1` | Draws a shadow behind UPS text. |
| `sh_ups_hide_zero` | `0` | Hides the UPS display when rounded speed is zero. |
| `sh_ups_color_mode` | `dynamic` | UPS color mode. |
| `sh_ups_color_gain` | `0 255 0 255` | RGBA color for speed gain. |
| `sh_ups_color_loss` | `255 0 0 255` | RGBA color for speed loss. |
| `sh_ups_color_neutral` | `255 255 255 255` | RGBA neutral UPS color. |
| `sh_ups_format` | `plain` | UPS text format. |
| `sh_ups_3d` | `0` | Uses 3D speed including vertical velocity. |

## Race Line and World Origin

| Cvar | Default | Description |
| --- | --- | --- |
| `race_alpha` | `0.5` | Race trail alpha. |
| `race_color` | `0 255 0` | Race trail RGB color. |
| `race_life` | `500` | Race trail lifetime in milliseconds. |
| `race_width` | `5` | Race trail width. |
| `cl_drawworldorigin` | `0` | Draws the world origin marker. |
| `cl_worldorigin_size` | `4096` | Size of the world origin marker. |
| `cl_worldorigin_linewidth` | `1` | Line width for the world origin marker. |

## Network Meter Core

| Cvar | Default | Description |
| --- | --- | --- |
| `sh_netmeter` | `0` | Selects the network display mode: off, lagometer, netgraph, or histogram. |
| `sh_netmeter_min_ms` | `0` | Bottom of the fixed latency scale. |
| `sh_netmeter_max_ms` | `150` | Top of the fixed latency scale. |
| `sh_netmeter_adaptive` | `1` | Auto-scales network displays against recent latency. |

## Network Warning Thresholds

| Cvar | Default | Description |
| --- | --- | --- |
| `sh_netwarn_ping_adaptive` | `1` | Compares warnings against the rolling ping baseline. |
| `sh_netwarn_spike_ms` | `80` | Absolute spike threshold in milliseconds. |
| `sh_netwarn_spike_pct` | `50` | Adaptive spike threshold as a percentage of rolling ping. |
| `sh_netwarn_jitter_ms` | `40` | Jitter threshold in milliseconds. |

## Network Alerts

| Cvar | Default | Description |
| --- | --- | --- |
| `sh_netalert` | `0` | Enables network incident text alerts. |
| `sh_netalert_x` | `0` | Alert X position. |
| `sh_netalert_y` | `48` | Alert Y position. |
| `sh_netalert_color` | `7` | Alert color or severity-based color mode. |
| `sh_netalert_alpha` | `0.85` | Alert text alpha. |
| `sh_netalert_duration_ms` | `1200` | Alert lifetime in milliseconds. |
| `sh_netalert_loss` | `1` | Enables packet loss alerts. |
| `sh_netalert_jitter` | `1` | Enables jitter alerts. |
| `sh_netalert_spike` | `1` | Enables ping spike alerts. |

## Lagometer Display

| Cvar | Default | Description |
| --- | --- | --- |
| `sh_lagometer_x` | `0` | Lagometer X position. |
| `sh_lagometer_y` | `-1` | Lagometer Y position. |
| `sh_lagometer_color_normal` | `213` | Palette index for normal samples. |
| `sh_lagometer_color_spike` | `220` | Palette index for ping spike samples. |
| `sh_lagometer_color_jitter` | `220` | Palette index for jitter samples. |
| `sh_lagometer_color_loss_s2c` | `242` | Palette index for server-to-client loss. |
| `sh_lagometer_color_loss_c2s` | `224` | Palette index for client-to-server/choke loss. |
| `sh_lagometer_alpha` | `0.20` | Alpha for normal samples. |
| `sh_lagometer_bad_alpha` | `0.75` | Alpha for incident samples. |

## Netgraph Display

| Cvar | Default | Description |
| --- | --- | --- |
| `sh_netgraph_y` | `-1` | Netgraph Y position. |
| `sh_netgraph_height` | `32` | Netgraph height in HUD pixels. |
| `sh_netgraph_alpha` | `0.75` | Netgraph alpha. |
| `sh_netgraph_color_normal` | `213` | Palette index for normal samples. |
| `sh_netgraph_color_spike` | `220` | Palette index for ping spike samples. |
| `sh_netgraph_color_jitter` | `220` | Palette index for jitter samples. |
| `sh_netgraph_color_loss_s2c` | `242` | Palette index for server-to-client loss. |
| `sh_netgraph_color_loss_c2s` | `224` | Palette index for client-to-server/choke loss. |

## Histogram Display

| Cvar | Default | Description |
| --- | --- | --- |
| `sh_histogram_x` | `0` | Histogram X position. |
| `sh_histogram_y` | `-1` | Histogram Y position. |
| `sh_histogram_width` | `160` | Histogram width in HUD pixels. |
| `sh_histogram_height` | `4` | Histogram height in HUD pixels. |
| `sh_histogram_bg_alpha` | `0.20` | Histogram background alpha. |
| `sh_histogram_color_bg` | `0` | Palette index for the histogram background. |
| `sh_histogram_color_normal` | `213` | Palette index for normal samples. |
| `sh_histogram_color_spike` | `220` | Palette index for ping spike samples. |
| `sh_histogram_color_jitter` | `220` | Palette index for jitter samples. |
| `sh_histogram_color_loss_s2c` | `242` | Palette index for server-to-client loss. |
| `sh_histogram_color_loss_c2s` | `224` | Palette index for client-to-server/choke loss. |
| `sh_histogram_alpha` | `0.20` | Alpha for normal samples. |
| `sh_histogram_bad_alpha` | `0.75` | Alpha for incident samples. |
| `sh_histogram_history_ms` | `60000` | Histogram history window in milliseconds. |
| `sh_histogram_ping` | `1` | Draws smoothed ping text near the histogram. |

## MP4 Recording

| Cvar | Default | Description |
| --- | --- | --- |
| `gl_mp4_fps` | `60` | Default MP4 capture frame rate. |
| `gl_mp4_template` | `demoXX` | Default MP4 output filename template. |
| `gl_mp4_bitrate` | `15000` | Default MP4 video bitrate in kbps. |
| `gl_mp4_encoder` | `h264_amf` | Preferred video encoder. |
| `gl_mp4_downscale` | `1` | Downscale factor for MP4 capture. |
| `gl_mp4_h264_preset` | `faster` | H.264 encoder preset. |
| `gl_mp4_h264_tune` | `zerolatency` | H.264 encoder tune. |
| `gl_mp4_h264_crf` | `20` | H.264 CRF value when applicable. |

## UI and Menus

| Cvar | Default | Description |
| --- | --- | --- |
| `ui_colorpicker_r` | `255` | Red component used by the menu color picker. |
| `ui_colorpicker_g` | `255` | Green component used by the menu color picker. |
| `ui_colorpicker_b` | `255` | Blue component used by the menu color picker. |
| `ui_colorpicker_a` | `255` | Alpha component used by the menu color picker. |
| `ui_draw_layers` | `0` | Draws/debugs menu layers. |
| `ui_external_menu` | `0` | Chooses built-in menu data for `game jump`; other games load external `q2pro.menu`. |
| `ui_menu_style` | `0` | Selects menu style: classic, slate, or custom. |
| `cl_menu_cursor` | `ch5` | In-game menu cursor texture/name. |
| `win_menu_cursor` | `arrow` | Windows system cursor used for menus. |

## Video and Windowing

| Cvar | Default | Description |
| --- | --- | --- |
| `vid_noborder` | `0` | Enables borderless fullscreen/window behavior on Windows. |
