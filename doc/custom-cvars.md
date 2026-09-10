
## Sound

| Cvar | Default | Description |
| --- | --- | --- |
| `s_mute_player_jump_sounds` | `0` | Suppresses `*jump1.wav` voice sounds from all players, regardless of model. Other player voice sounds are unaffected. |

## Input and Movement

| Cvar | Default | Description |
| --- | --- | --- |
| `cl_input_keys` | empty | Read-only string with currently pressed movement/jump keys. |
| `m_r1q2` | `0` | Enables R1Q2-style mouse handling behavior. |

## Client Smoothing and FPS Behavior

| Cvar | Default | Description |
| --- | --- | --- |
| `cl_step_smoothing_mode` | `q2pro` | Selects the step smoothing behavior used by the client. |
| `fps_default_hold` | `30` | Default FPS value assigned to new `fps_hold_*` slot cvars. |
| `fps_default_release` | `120` | Default FPS value assigned to new `fps_release_*` slot cvars. |

FPS defaults seed missing slots at client startup, after normal config/autoexec
loading and command-line `+set` overrides. Explicitly configured or saved slots
keep their values. Changing a default at runtime or through a late `+exec` does
not reset existing slots.

## Strafe Helper HUD

| Cvar | Default | Description |
| --- | --- | --- |
| `sh_draw` | `1` | Enables the strafe helper HUD bar. |
| `sh_center` | `1` | Draws the center reference zone. |
| `sh_centermarker` | `1` | Draws the center marker. |
| `sh_height` | `15` | Height of the strafe helper bar. |
| `sh_scale` | `1.500000` | Visual scale of the helper. |
| `sh_y` | `100` | Vertical position of the helper. |
| `sh_alpha` | `0.500000` | Overall helper alpha multiplier. |
| `sh_bar_style` | `gradient` | Bar rendering style. |
| `sh_center_width` | `2` | Width of the center marker/zone. |
| `sh_optimal_width` | `2` | Width of the optimal acceleration zone. |
| `sh_optimal_outline` | `1` | Draws the optimal zone as an outline. |
| `sh_smoothing` | `0` | Amount of helper smoothing. |
| `sh_smoothing_mode` | `1` | Smoothing curve/mode. |

## Strafe Helper Colors and NerdStats

| Cvar | Default | Description |
| --- | --- | --- |
| `sh_color_accelerating` | `0 128 0 128` | RGBA color used while accelerating. |
| `sh_color_optimal` | `255 215 0 255` | RGBA color for the optimal acceleration zone. |
| `sh_color_centermarker` | `255 255 255 255` | RGBA color for the center marker. |
| `sh_color_nerdstats` | `50 0 50 100` | RGBA background/color value used by NerdStats. |
| `sh_nerdstats` | `0` | Enables detailed strafe helper NerdStats display. |

## UPS Display

| Cvar | Default | Description |
| --- | --- | --- |
| `sh_ups` | `1` | Enables the centered UPS display. |
| `sh_ups_scale` | `1.250000` | Text scale for the UPS display. |
| `sh_ups_y` | `-5` | Vertical offset for the UPS display. |
| `sh_ups_shadow` | `1` | Draws a shadow behind UPS text. |
| `sh_ups_hide_zero` | `1` | Hides the UPS display when rounded speed is zero. |
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
| `sh_netmeter` | `3` | Selects the network display mode: off, lagometer, netgraph, or histogram. |
| `sh_netmeter_min_ms` | `0` | Bottom of the fixed latency scale. |
| `sh_netmeter_max_ms` | `150` | Top of the fixed latency scale. |
| `sh_netmeter_adaptive` | `1` | Auto-scales network displays against recent latency. |

## Network Warning Thresholds

| Cvar | Default | Description |
| --- | --- | --- |
| `sh_netwarn_ping_adaptive` | `1` | Compares warnings against the rolling ping baseline. |
| `sh_netwarn_spike_ms` | `300` | Absolute spike threshold in milliseconds. |
| `sh_netwarn_spike_pct` | `100` | Adaptive spike threshold as a percentage of rolling ping. |
| `sh_netwarn_jitter_ms` | `100` | Jitter threshold in milliseconds. |

## Network Alerts

| Cvar | Default | Description |
| --- | --- | --- |
| `sh_netalert` | `1` | Enables network incident text alerts. |
| `sh_netalert_x` | `0` | Alert X position. |
| `sh_netalert_y` | `353` | Alert Y position. |
| `sh_netalert_color` | `1` | Alert color or severity-based color mode. |
| `sh_netalert_alpha` | `1` | Alert text alpha. |
| `sh_netalert_duration_ms` | `2000` | Alert lifetime in milliseconds. |
| `sh_netalert_loss` | `1` | Enables packet loss and client-drop alerts. |
| `sh_netalert_jitter` | `1` | Enables jitter alerts. |
| `sh_netalert_spike` | `1` | Enables ping spike alerts. |

## Lagometer Display

| Cvar | Default | Description |
| --- | --- | --- |
| `sh_lagometer_x` | `166` | Lagometer X position. |
| `sh_lagometer_y` | `-1` | Lagometer Y position. |
| `sh_lagometer_color_normal` | `212` | Palette index for normal samples. |
| `sh_lagometer_color_spike` | `225` | Palette index for ping spike samples. |
| `sh_lagometer_color_jitter` | `220` | Palette index for jitter samples. |
| `sh_lagometer_color_loss_s2c` | `231` | Palette index for server-to-client loss. |
| `sh_lagometer_color_loss_c2s` | `243` | Palette index for client-to-server/choke loss. |
| `sh_lagometer_alpha` | `0.300000` | Alpha for normal samples. |
| `sh_lagometer_bad_alpha` | `1` | Alpha for incident samples. |

## Netgraph Display

| Cvar | Default | Description |
| --- | --- | --- |
| `sh_netgraph_y` | `-1` | Netgraph Y position. |
| `sh_netgraph_height` | `15` | Netgraph height in HUD pixels. |
| `sh_netgraph_alpha` | `0.75` | Netgraph alpha. |
| `sh_netgraph_color_normal` | `213` | Palette index for normal samples. |
| `sh_netgraph_color_spike` | `208` | Palette index for ping spike samples. |
| `sh_netgraph_color_jitter` | `220` | Palette index for jitter samples. |
| `sh_netgraph_color_loss_s2c` | `233` | Palette index for server-to-client loss. |
| `sh_netgraph_color_loss_c2s` | `11` | Palette index for client-to-server/choke loss. |

## Histogram Display

| Cvar | Default | Description |
| --- | --- | --- |
| `sh_histogram_x` | `0` | Histogram X position. |
| `sh_histogram_y` | `-1` | Histogram Y position. |
| `sh_histogram_width_mode` | `2` | Histogram width mode: `0` custom, `1` half screen, `2` full screen. |
| `sh_histogram_width` | `1280` | Custom histogram width in HUD pixels. |
| `sh_histogram_height` | `15` | Histogram height in HUD pixels. |
| `sh_histogram_fill_mode` | `0` | Histogram fill mode: `0` separated bars, `1` spans between samples. |
| `sh_histogram_spacing_mode` | `0` | Histogram spacing mode: `0` sample time, `1` evenly spaced samples. |
| `sh_histogram_bg_alpha` | `0.150000` | Histogram background alpha. |
| `sh_histogram_color_bg` | `0` | Palette index for the histogram background. |
| `sh_histogram_color_normal` | `209` | Palette index for normal samples. |
| `sh_histogram_color_spike` | `220` | Palette index for ping spike samples. |
| `sh_histogram_color_jitter` | `215` | Palette index for jitter samples. |
| `sh_histogram_color_loss_s2c` | `227` | Palette index for server-to-client loss. |
| `sh_histogram_color_loss_c2s` | `241` | Palette index for client-to-server/choke loss. |
| `sh_histogram_alpha` | `1` | Alpha for normal samples. |
| `sh_histogram_bad_alpha` | `1` | Alpha for incident samples. |
| `sh_histogram_history_ms` | `52000` | Histogram history window in milliseconds. |
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
| `cl_menu_cursor` | `ch1` | Texture cursor used by the in-game menu in exclusive fullscreen. Accepts `none`, `cross`, `dot`, `angle`, `ch4`, or `ch5`. |
| `ui_colorpicker_r` | `255` | Red component used by the menu color picker. |
| `ui_colorpicker_g` | `255` | Green component used by the menu color picker. |
| `ui_colorpicker_b` | `255` | Blue component used by the menu color picker. |
| `ui_colorpicker_a` | `255` | Alpha component used by the menu color picker. |
| `ui_draw_layers` | `0` | Draws/debugs menu layers. |
| `ui_external_menu` | `0` | When `0`, `game jump` uses the compiled built-in menu. When `1`, or when using another game directory, the UI loads external `q2pro.menu`. |
| `ui_menu_density` | `2` | Controls menu row spacing. Named values are `compact` (0), `normal` (1), and `spacious` (2); numeric offsets are clamped from `-4` to `16`. |
| `ui_menu_focus_width` | `full` | Focus highlight width style: `full` or `content`. |
| `ui_menu_focus_padding_x` | `14` | Horizontal padding added around content-width focus highlights. |
| `ui_menu_focus_padding_y` | `3` | Vertical padding added around focus highlights. |
| `ui_menu_bar_image` | `q2jump_background` | Background image for menu side bars. |
| `ui_menu_bar_image_alpha` | `0.5` | Alpha/transparency for menu bar background image. |
| `ui_menu_bar_image_mode` | `screen` | Menu bar image rendering mode (e.g. `screen` or `stretch`). |
| `ui_menu_model` | `1` | Enables the animated player model behind top-level menus. |
| `ui_menu_model_x` | `0.8125` | Horizontal center position of the menu background model, from `0` to `1`. |
| `ui_menu_model_y` | `0.5` | Vertical center position of the menu background model, from `0` to `1`. |
| `ui_menu_model_scale` | `1` | Size multiplier for the menu background model viewport. |
| `ui_menu_model_yaw` | `200` | Yaw angle used by the menu background model. |
| `ui_menu_model_distance` | `40` | Distance of the menu background model from the preview camera. Higher values make it smaller. |
| `ui_menu_model_position` | `bottom` | Preset position for the menu background model. Selecting a preset updates `ui_menu_model_x` and `ui_menu_model_y`; moving either slider switches this to `custom`. |
| `ui_menu_model_orbit` | `1` | Allows mouse movement over the model viewport to orbit the preview. |
| `ui_menu_anim` | `1` | Enables menu focus highlight animation. |
| `ui_menu_anim_focus_ms` | `70` | Duration in ms for menu focus highlight animation. |
| `ui_menu_title_top_padding` | `12` | Top padding for menu title text. |
| `ui_menu_title_item_gap` | `0` | Gap between menu title and first item. |
| `win_menu_cursor` | `arrow` | Windows system cursor used for menus. |

### Menu color cvars

All menu color cvars are archived RGBA strings in the form `R G B A`. The built-in q2jump menu exposes them through **Menu Setup -> Menu Colors**.

| Cvar | Default | Description |
| --- | --- | --- |
| `ui_menu_color_panel_bg` | `8 12 18 160` | Main menu panel background. |
| `ui_menu_color_title_text` | `225 112 124 255` | Menu title text. |
| `ui_menu_color_text` | `210 216 224 255` | Default text. |
| `ui_menu_color_item_text` | `210 216 224 255` | Selectable item text. |
| `ui_menu_color_label_text` | `160 168 178 255` | Secondary labels and left-column text. |
| `ui_menu_color_focus_text` | `255 255 255 255` | Text on focused controls. |
| `ui_menu_color_selection_bg` | `44 78 112 200` | Selected list row background. |
| `ui_menu_color_focus_bg` | `0 0 0 0` | Focus highlight background. |
| `ui_menu_color_focus_edge` | `90 130 170 100` | Focus highlight edge/border. |
| `ui_menu_color_disabled_text` | `86 90 98 255` | Disabled item text. |
| `ui_menu_color_list_header_bg` | `82 40 50 235` | List column header background. |
| `ui_menu_color_scrollbar_track` | `26 36 48 255` | Scrollbar track. |
| `ui_menu_color_hint_bg` | `5 7 10 235` | Bottom hint/status bar background. |
| `ui_menu_color_hint_text` | `185 195 205 255` | Bottom hint/status bar text. |
| `ui_menu_color_focus_marker` | `170 70 83 220` | Focus marker/triangle accent. |
| `ui_menu_color_value_text` | `220 224 230 255` | Value column text. |
| `ui_menu_color_value_active_text` | `255 255 255 255` | Focused value column text. |
| `ui_menu_color_value_changed_text` | `225 112 124 255` | Modified value text before commit. |
| `ui_menu_color_slider_track` | `26 36 48 255` | Slider track. |
| `ui_menu_color_slider_fill` | `48 86 124 220` | Slider filled range. |
| `ui_menu_color_slider_thumb` | `160 168 178 255` | Slider thumb. |
| `ui_menu_color_slider_border` | `95 143 190 180` | Slider thumb border. |
| `ui_menu_color_sorted_header_bg` | `95 45 56 235` | Sorted list header background. |
| `ui_menu_color_tab_text` | `185 195 205 255` | Inactive server-browser tab text. |
| `ui_menu_color_tab_active_text` | `255 255 255 255` | Active server-browser tab text. |
| `ui_menu_color_tab_active_bg` | `44 78 112 200` | Active server-browser tab background. |
| `ui_menu_color_tab_inactive_bg` | `8 12 18 180` | Inactive server-browser tab background. |
| `ui_menu_color_panel_border` | `44 58 72 160` | Panel and swatch border. |
| `ui_menu_color_panel_shadow` | `0 0 0 120` | Panel and swatch drop shadow. |
| `ui_menu_color_tab_underline` | `225 112 124 220` | Active server-browser tab underline. |

### Built-in menu script notes

The q2jump build has a compiled menu string and uses it by default for `game jump`. External menu files still work when `ui_external_menu` is set to `1`.

The menu script supports `style --live` for controls that should write cvars as the user changes them. Individual controls can use `--defer` to stay pending inside a live menu and commit only when the menu is closed. This is used by the video menu so safe settings update live, while refresh-sensitive settings (MSAA, hardware gamma, texture reload, shader backend) apply only after leaving.

The `plaque` command accepts an optional position argument: `left`, `right`, `top`, `bottom`, `center`, `screen-topleft`, `screen-topright`, `screen-bottomleft`, or `screen-bottomright`.

The top-level `style` and `focus` commands apply defaults to all subsequent `begin` menus. They accept the same flags as per-menu `style`/`focus` (`--compact`, `--center`, `--transparent`, `--live`, `--border`, `--inset`, `--height`, etc.). Per-menu `style` or `focus` overrides these defaults.

Actions and bindings support `--align` to set the label text to an alternate status string in the right column.

Custom color pickers work for any `sh_*` (strafe helper), `ui_menu_color_*`, and `race_color` cvars.

The `ui_reset_netmeter_settings` command restores all network bar, alert, and threshold settings to their defaults.

Strafe helper color presets are selected with `sh hud preset <name>` and show colored swatches in the menu list.

Virtual `--show-if` cvars provided by the UI are `ui_exclusive_fullscreen`, `ui_borderless_or_windowed`, and `ui_histogram_custom_width`.

## Video and Windowing

| Cvar | Default | Description |
| --- | --- | --- |
| `gl_lava_intensity` | `1` | Multiplies opaque lava texture brightness in the shader renderer after the global `intensity` value. Values are clamped to `0.1`-`5`. |
| `vid_noborder` | `0` | Enables borderless fullscreen/window behavior on Windows. |
