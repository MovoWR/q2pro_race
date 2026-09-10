
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

The angle bar follows the final command of local movement prediction, including
the 30 UPS projection cap on air-acceleration servers. It clears when prediction
is disabled or unavailable, including demo playback. Earlier replayed commands and
listen-server movement do not update the local helper. Smoothing uses elapsed
client time and resets when the strafe side changes.

The `sh hud ypos` command uses the same numeric validation as the UPS controls.
It accepts exactly one finite number from -4000 to 4000. Malformed values,
numeric overflow or underflow, and extra arguments leave the setting unchanged.

| Cvar | Default | Description |
| --- | --- | --- |
| `sh_draw` | `1` | Enables the strafe helper HUD bar. |
| `sh_center` | `1` | Centers the display on horizontal velocity; `0` uses view yaw and wraps at ±180 degrees. |
| `sh_centermarker` | `1` | Draws the center marker. |
| `sh_height` | `15` | Height of the strafe helper bar. |
| `sh_scale` | `1.500000` | Visual scale of the helper. |
| `sh_y` | `100` | Offset from screen center; negative moves upward. Console/editor support -4000 to 4000; the menu slider covers -320 to 320. |
| `sh_alpha` | `0.500000` | Overall helper alpha multiplier. |
| `sh_bar_style` | `gradient` | Bar rendering style. |
| `sh_center_width` | `2` | Width of the center marker/zone. |
| `sh_optimal_width` | `2` | Width of the optimal acceleration zone. |
| `sh_optimal_outline` | `1` | Draws the optimal zone as an outline. |
| `sh_smoothing` | `0` | Smooths the angle bar (0-10); `0` disables smoothing. |
| `sh_smoothing_mode` | `1` | Smoothing curve/mode. Nonlinear modes `4` and `5` retain some sensitivity to frame timing. |

## Strafe Helper Colors and NerdStats

| Cvar | Default | Description |
| --- | --- | --- |
| `sh_color_accelerating` | `0 128 0 128` | RGBA color used while accelerating. |
| `sh_color_optimal` | `255 215 0 255` | RGBA color for the optimal acceleration zone. |
| `sh_color_centermarker` | `255 255 255 255` | RGBA color for the center marker. |
| `sh_color_nerdstats` | `50 0 50 100` | RGBA background/color value used by NerdStats. |
| `sh_nerdstats` | `0` | Enables detailed strafe helper NerdStats display, including the command's Wishspeed in UPS. |

## Efficiency Display

Shows the current airborne strafe command's horizontal acceleration efficiency
(0-100% of the theoretical maximum gain) near the strafe helper bar. Configure
in-game via the `shefficiency` menu or the `sh eff` console commands.

Efficiency smoothing uses elapsed time between live display updates, with a
time constant of `0.05 * sh_efficiency_smoothing` seconds. A zero setting snaps
to the current value; repeated draws at the same time do not advance an enabled
filter. Menu and editor previews use separate display values and do not change
the live reading, smoothing, or hold lifetime. The threshold marker stays visible
over the fill.

| Cvar | Default | Description |
| --- | --- | --- |
| `sh_efficiency` | `0` | Enables the efficiency display. |
| `sh_efficiency_style` | `bar` | Display style: `bar`, `text` (percentage readout), `both`, or `none` (helper tint only). |
| `sh_efficiency_tint` | `off` | Tints strafe helper elements by current efficiency: `optimal` (optimal-angle marker), `zone` (accelerating zone), or `both`. Reverts to the configured helper colors when no data is live. |
| `sh_efficiency_tint_strength` | `0.75` | How strongly the tint overrides the helper's configured colors (0-1); element alpha is always preserved. |
| `sh_efficiency_width` | `80` | Bar width in HUD units (8-4000). |
| `sh_efficiency_height` | `4` | Bar height in HUD units (1-64). |
| `sh_efficiency_x` | `0` | Horizontal offset from screen center in HUD units (-4000 to 4000). |
| `sh_efficiency_y` | `3` | Gap below the helper bar in HUD units (-400 to 400); negative values place the display above it. |
| `sh_efficiency_border` | `1` | Draws a 1 px outline around the bar. |
| `sh_efficiency_marker` | `0.9` | Threshold marker position on the bar (0-1); `0` hides it. |
| `sh_efficiency_color_mode` | `dynamic` | `dynamic` colors by value via the bad/mid/good gradient; `static` always uses the good color. |
| `sh_efficiency_color_good` | `80 220 90 255` | RGBA color near 100% efficiency; also the static color. |
| `sh_efficiency_color_mid` | `235 200 60 255` | RGBA color at the gradient midpoint. |
| `sh_efficiency_color_bad` | `235 70 60 255` | RGBA color near 0% efficiency. |
| `sh_efficiency_color_bg` | `40 40 40 200` | RGBA background color of the bar track. |
| `sh_efficiency_color_midpoint` | `0.5` | Efficiency value (0.05-0.95) where the mid color sits; raise it to make the top band more discriminating. |
| `sh_efficiency_smoothing` | `0` | Smooths the displayed value (0-10, same scale as `sh_smoothing`); `0` disables. |
| `sh_efficiency_hold_ms` | `150` | Keeps the last value on screen this many milliseconds after data stops (0-2000), avoiding flicker. |
| `sh_efficiency_text_scale` | `1` | Scale of the percentage text (0.25-8). |

## UPS Display

Speed uses local predicted velocity when prediction is allowed; otherwise it
uses the current server snapshot, including when the server sets
`PMF_NO_PREDICTION`. Gradient color derives acceleration from elapsed display
time and resets its history when the speed source changes.

| Cvar | Default | Description |
| --- | --- | --- |
| `sh_ups` | `1` | Enables the centered UPS display. |
| `sh_ups_scale` | `1.250000` | Text scale for the UPS display (0.25-8). |
| `sh_ups_x` | `0` | Horizontal offset from screen center for the UPS display, in HUD units (-4000-4000). |
| `sh_ups_y` | `-5` | Vertical offset for the UPS display (-4000-4000). |
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

Netgraph and histogram heights use fixed limits of 1-4000 HUD pixels; custom
histogram width uses 10-4000. Drawing clips to the current viewport without
replacing saved dimensions, so a larger viewport restores the configured size.
The legacy detailed graph (`scr_netgraph 2`) keeps its 10-200 pixel drawing range
without changing the shared `sh_netgraph_height` setting.

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
| `sh_netalert` | `1` | Enables network incident text alerts independently of the network graph; alerts also work with `sh_netmeter 0`. |
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

Samples mapped to one column retain the greatest height and highest incident
priority: server-to-client loss, client-to-server loss, spike, jitter, then normal.
The newest sample wins equal-priority ties for color fading. Burst client loss
that exhausts command redundancy is included in loss detection and excluded from
the clean latency/jitter baseline. Jitter averaging retains fractional milliseconds.

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

The `sh` console command supports Tab completion for sections, subcommands, and supported values.

Virtual `--show-if` cvars provided by the UI are `ui_exclusive_fullscreen`, `ui_borderless_or_windowed`, and `ui_histogram_custom_width`.

## Video and Windowing

| Cvar | Default | Description |
| --- | --- | --- |
| `gl_lava_intensity` | `1` | Multiplies opaque lava texture brightness in the shader renderer after the global `intensity` value. Values are clamped to `0.1`-`5`. |
| `vid_noborder` | `0` | Enables borderless fullscreen/window behavior on Windows. |

## Drag-and-drop HUD editor

Open **Jump/Race Setup → Edit HUD layout**, or run `hud_edit`.
The editor works over the current view or from the main menu. Multiplayer
continues while editing; entering the editor does not pause a server.

- Click an overlay or its row in the element list; Tab / Shift+Tab cycle selection.
  The compact toolbox uses normal menu scale, with up to six rows and fewer when
  needed to fit the window. Use the All / Client / Server tabs (keys 1-3) to narrow
  the list. Client includes jump overlays and custom text; Server includes recognized
  status-bar groups, server menus/scores, and unrecognized server HUD. Mouse wheel changes
  selection; PgUp/PgDn move by the current page size. Clicking a HUD element
  outside the current category switches back to All. Rows show ON, OFF, or LOCK.
  Long names end with an ellipsis; hover their row to see the full name.
- Drag the title bar to move the toolbox. Undo, Redo, and Hide are in the header;
  Visible, Lock, and Focus remain directly below the list. The selected element
  has a name tag on the canvas. Apply and Cancel remain visible at the bottom of
  the toolbox; Apply * indicates unsaved changes. Hover controls for shortcut hints.
- Arrange and Reset start collapsed, and only one section can be open at a time.
  Arrange contains position/size readouts, four nudge buttons, Snap, and screen
  alignment in two rows of three buttons. Nudge by one HUD unit, or ten with Shift.
  Hover the readout for move/resize guidance. Reset contains Reset selected element
  and Restore all defaults. Expanding or collapsing a section changes only the view.
- Drag to move. The strafe helper remains horizontally centered, and efficiency
  keeps its position relative to the helper. Netgraph mode remains full width.
- Drag the selected lower-right handle to resize supported jump overlays. The lagometer
  and the additional server/client groups have position controls only.
  Resizing a histogram horizontally selects its custom-width mode.
- A toggles Arrange; D toggles Reset. Both sections start collapsed on opening.
- Arrow keys move by one HUD unit; Shift uses ten. Ctrl+arrows resize.
  UPS and text-only efficiency resize horizontally with proportional text scaling;
  the helper's horizontal resize changes angle scale. Efficiency in bar/both mode
  and the network meter resize their bar width and height.
- Snap aligns to screen edges/center and other visible HUD elements' edges/centers.
  Cyan guides mark the current snap while dragging. Toggle it with S or hold Alt
  to bypass it. In Arrange, the Align to screen buttons place the selection at the
  left, horizontal center, right, top, vertical center, or bottom. Existing movement
  constraints still apply.
- V / Visible changes the selected element's gameplay visibility. Disabled elements
  still have editor previews. If NetMeter is disabled, its preview uses histogram mode.
- R / Reset selected element restores the selected group's default layout and
  visibility in the draft; the button is inside Reset.
- Restore all defaults / Shift+R restores default layout and visibility for unlocked editable
  HUD elements across all categories, including custom text offsets. Locked and
  removed editor elements are untouched. Apply saves the reset; Cancel discards it.
- Undo / Ctrl+Z and Redo / Ctrl+Y (also Ctrl+Shift+Z) retain up to 64 editing actions
  during the current session. One completed drag or resize is one action; nudges,
  visibility changes, alignment, resets, and lock changes are also undoable. A new
  edit after Undo replaces the redo branch. Navigation, focus, and opening or closing
  sections do not consume history. Keyboard shortcuts work with sections collapsed.
- Lock / L protects the selected element from movement, resizing, alignment,
  visibility changes, and resets. Locked elements remain selectable through the
  list. Locking efficiency also protects the strafe helper that positions it.
  Locks are saved on Apply as archived `hud_lock_<key>` cvars (default `0`);
  Cancel discards lock changes. Undo can intentionally restore an earlier lock state.
- Focus / F displays only the selected element's preview and outline. Other
  elements retain their measured bounds for snapping. Focus never changes gameplay
  visibility; changing selection updates the focused preview. Focus resets on opening.
- H or Hide hides the toolbox; H restores it. The toolbox also disappears
  automatically while dragging/resizing a HUD element and returns on release.
  There are no full-width toolbars or bottom footer covering the HUD.
- Enter / Apply accepts the draft. Esc / Cancel discards it. Closing the editor by
  opening the console, changing maps, disconnecting, or restarting the UI also discards it.

The original jump overlays are named **Speed (UPS)**, **Strafe helper**,
**Strafe efficiency**, and **Network monitor** in the editor.
Their sample speed, efficiency and graph values are local visual previews.
They do not enter movement measurements or network history. Bounds and preview
positions use HUD coordinates independently of menu scale. Resizing the window
or losing focus ends an active drag.

Apply updates only edited HUD settings; normal archived-cvar saving persists them.
Preview-only clamping of untouched settings is not saved, including after Undo/Redo.
Existing configurations retain their positioning conventions. `sh_ups_x` is the
new horizontal offset from screen center, in HUD units; its default is `0`.
`sh_ups_y` keeps its existing meaning. Both offsets are reported by the UPS status
commands. `sh hud ypos`, `sh ups ypos`, and `sh ups scale` share the numeric
validator used by the efficiency commands, so trailing text, extra arguments, and non-finite values are
rejected without changing the current value. Console/editor limits and menu-slider
ranges are described in the cvar tables; the `sh_y` menu slider uses a narrower
range than the console/editor. A smaller viewport clips efficiency, UPS, and network-meter drawing
without replacing saved dimensions or offsets; returning to a larger viewport
restores the configured layout.

The editor includes the four original jump overlays plus 32 additional groups,
and each registered `draw` / `draw_dynamic` text object (up to 88 custom objects).
Custom text registered while editing becomes available after reopening the editor.
No server or protocol changes are required.

### Server HUD groups

The recognized server `ctf_statusbar` contains 19 independently movable groups:

| Editor name | Contents | Cvar key |
| --- | --- | --- |
| Health | Health value and label | `health` |
| Weapon icon | Selected weapon icon | `item` |
| Speed (server) | Current speed and Speed label | `speed` |
| Target player name | Player identification | `target` |
| Run timer | Seconds, decimal point, tenths, Time label | `timer` |
| Input keys | Directional, jump/crouch, attack icons | `inputs` |
| Reported player FPS | Player's reported FPS setting or recorded replay FPS, and label | `server_fps` |
| Vote information | Four voting lines together | `vote` |
| Available maps | Number of available server maps and Maps label | `mapcount` |
| Team / mode | Easy / Hard / Observer status | `status1` |
| Run info line 1 | Dynamic race, replay, checkpoint or lap information | `status2` |
| Run info line 2 | Dynamic race, replay, checkpoint or lap information | `status3` |
| Run info line 3 | Dynamic race, replay, checkpoint or lap information | `status4` |
| Current map | Current map name | `map` |
| Previous map 1 | Most recent previous map | `prevmap1` |
| Previous map 2 | Second previous map | `prevmap2` |
| Previous map 3 | Third previous map | `prevmap3` |
| Map time adjustment | Positive or negative adjustment to map time | `addedtime` |
| Map time remaining | Remaining map time and its Time label | `timeleft` |

Recognition matches the complete token structure of this server status bar,
allowing whitespace and map-name changes. It does not identify a mod from a stat
number alone. Unrecognized status bars use **Other server HUD**, moving the entire
original program together. **Scoreboard / server menus** is a separate whole-panel
control; its individual rows, selection logic and contents are preserved.
Server stats, input image indices, timer precision, conditions, and replay/chase
values are never rewritten. **Reported player FPS** retains the server-reported value
rather than becoming a measured local rendering rate.

### Client HUD groups

Independent position/visibility controls cover **Chat history**, **Center messages**,
**Console messages**, **Chat input**, **Network alerts**, **Ping graph / connection icon**,
**Frame / prediction warnings**, **Inventory**, and **Classic network / debug graph**.
Crosshair, hit marker, debug movement, debug frame stats, nerd stats, loading,
demo progress, and pause are excluded from the editor list, selection, and previews.
Their normal gameplay behavior and existing saved settings are retained.
The full console, ordinary client menus, world-space race lines and other 3D effects
are not HUD layout elements.

**Render FPS** (`render_fps`, measured `r_mfps`) and **Movement rate (MPS)**
(`move_fps`, measured `cl_mmps`) are optional new widgets, off by default. Enable them
with V / Visible. Existing custom FPS/UPS/ping text remains independently editable
under its original macro/cvar name; no `draw` commands are replaced.

### Saved offsets and previews

Each additional group has archived `hud_<key>_x`, `hud_<key>_y` (default `0`) and
`hud_<key>_visible` (default `1`, except the render FPS and movement rate widgets). Offsets are in HUD
units and are added to the original position, including any existing position cvars.
For example, `hud_timer_x -80` moves the complete run timer 80 HUD units left.
Custom draw objects use stable `hud_draw_<name-hash>_*` keys so config command order
can change without assigning a saved position to a different object.

Visibility is an additional local filter. Allowing a group does not override its
normal feature setting or server condition: chat still needs `scr_chathud`, pause
still requires a paused game, and zeroed server input stats remain hidden. Reset
clears the selected group's added offsets/filter; it does not reset its original
feature settings. Demo progress is excluded from editing; any existing saved
demo offsets and its normal viewport reservation are retained.

Additional groups use labeled preview boxes, with the last captured drawing bounds
when available and representative bounds for inactive/unseen content. Original
client position cvars are honored for crosshair, hit marker, chat and network-alert
fallbacks. Custom text positions are read when opening the editor. Selecting a recognized server
group shows the complete reference status bar; other inactive groups appear when
selected. Sample text does not enter live stats, chat history, input state, or timers.
Move the toolbox by its title bar or use H to hide it when working near screen edges. Resizing/color editing
for these additional groups and named layout presets are not implemented.
