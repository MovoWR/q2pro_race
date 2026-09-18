# Custom client settings

Defaults below are the values registered by the current source. Saved configs,
command-line overrides, and the documented presets can change active values.
Optional media features are available only when enabled in the build.

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
not reset existing slots. FPS command values must be complete positive decimal
integers from 1 through 2147483647; the existing frame limiter still applies its
mode-specific bounds and rounding. Invalid `+fps`, `-fps` or selected hold/release
slot values leave the current FPS unchanged. Slot numbers must be complete
integers from 1 through 12. Invalid startup defaults use 30 for missing hold slots
and 120 for missing release slots, without overwriting saved slots or defaults.

`toggle cl_maxfps 30 120` cycles between 30 and 120. If the current FPS is
outside the list (for example, 60), the next toggle selects its first value,
30. This recovery applies to all explicit `toggle` value lists.

## Bind reminders

Bind reminders are enabled by default. Configure them in **Jump Setup > HUD & Visuals > Bind Reminders**.
**Detect FPS binds** is on by default: the list reads the current bindings and
aliases loaded by your configs, so no manual FPS reminder entries are needed.
Rebinding/unbinding a key, reloading a config, redefining an alias or changing a
hold/release slot updates the list on the next draw. The reminder menu does not
assign an example FPS binding; use **FPS Preset Bindings** or **Jump Action Bindings** to edit
your keys.

The overlay uses a compact translucent panel with centered keycaps and no section
headings. Rows are spaced 14 HUD pixels apart. Mouse keys use `M1` through `M8`;
wheel keys use `MWUP` / `MWDN`. Key labels longer than 8 characters and captions
longer than 18 characters end in `...`; the stored configuration is preserved. The default opacity is 0.6 before `scr_alpha`
is applied; saved opacity settings remain unchanged.

Automatic discovery recognizes:

- `cl_maxfps VALUE` and `set`, `seta`, `setu`, `sets cl_maxfps VALUE`.
- `toggle cl_maxfps ...`, `inc cl_maxfps`, `dec cl_maxfps` and `creset cl_maxfps`.
- Every registered shortcut from `f20` through `f120`.
- `+fps DOWN UP` / `-fps DOWN UP` and `+fps_hold SLOT` / `-fps_hold SLOT`.
- Invoked aliases and commands separated by semicolons or newlines, including
  `+alias` bindings whose release alias changes FPS.
- Both branches of `if` commands, without evaluating the condition.

Each detected FPS key gets its own row, in key order; the eight custom slots do
not limit automatic discovery. Direct settings show **20 FPS**, toggles show their
values (**30 / 120**), and hold bindings show **30 > 120** (held value, then release
value). Simple aliases inherit the detected action caption. Conditions, scripts
and unresolved presets show **FPS**, without claiming a final value. Release-only
aliases append **up**. A direct `-fps` command shows its release target; a nested `+fps`
command shows its down target without promising an automatic release. Automatic
release metadata requires the binding's first character to be `+`; leading spaces
or a quoted command name show only the down target. Literal `+fps` / `-fps` pairs
must contain two positive integers, matching the command's validation. If either
operand contains an unresolved macro, neither value produces an active outline.
Slot captions read the current
`fps_hold_N` / `fps_release_N` values. Command/alias spelling follows the engine's
case-sensitive dispatch. Raw button aliases that change FPS on both press and
release show `FPS`; their outline follows the current key phase. Setters with multiple value tokens
also show `FPS`; plain `set` with an exact `u` or `s` flag keeps its single-value
caption. Invalid slot numbers are ignored, and slot captions show only valid
press/release values. A valid release with an invalid press value is marked `up`.
Quoted chat text, binding/alias definitions and commands that merely print
`cl_maxfps` are not treated as FPS changes.

**Extended reminders** offers individual switches for **JumpMod menu**, **Respawn**,
**Store**, and **Observer**. Their HUD captions are **Menu**, **Respawn**, **Store**,
and **Observer**. The options find keys whose whole binding is `inven`, `kill`,
`store`, or `observer`, using the same lookup as custom rows. Menu follows the
`inven` binding; it does not fall back to the client menu's Esc key.
They update when keys are rebound and add no row while unbound. Existing matching
custom rows retain their captions and unbound placeholders and are not duplicated;
the corresponding switch also hides those rows. Menu and Store start enabled;
Respawn and Observer start disabled. Saved user settings are preserved. On
startup, an existing custom `kill` or `observer` reminder enables its new switch
unless that switch has already been set; its Reset default remains off. These
options do not assign or execute bindings.

**Extra reminders 1-4** and **Extra reminders 5-8** configure additional actions,
such as store and recall. An exact matching custom command with a nonempty caption
also overrides the caption for a detected FPS binding. Automatic mode suppresses
duplicate manual FPS rows and unbound FPS placeholders. Disabling **Detect FPS binds**
turns off automatic FPS rows; custom and enabled extended reminders remain.

| Cvar | Default | Description |
| --- | --- | --- |
| `scr_bindreminders_visible` | `1` | Shows the reminder list. |
| `scr_bindreminders_fps` | `1` | Discovers loaded FPS bindings automatically. |
| `scr_bindreminders_menu` | `1` | Shows keys bound to `inven` (JumpMod menu). |
| `scr_bindreminders_respawn` | `0` | Shows keys bound to `kill`; also controls matching custom rows. |
| `scr_bindreminders_store` | `1` | Shows keys bound to `store`; also controls matching custom rows. |
| `scr_bindreminders_observer` | `0` | Shows keys bound to `observer`; also controls matching custom rows. |
| `scr_bindreminders_alpha` | `0.6` | Opacity from 0 to 1, multiplied by `scr_alpha`. |
| `scr_bindreminders_x`, `scr_bindreminders_y` | `0` | Position offsets managed by the HUD editor. |
| `scr_bindreminders_scale` | `1` | Visual size multiplier, from `0.25` to `4`. |
| `scr_bindreminders_command_1` ... `_8` | See below | Complete custom binding to look up; an empty command hides its row. |
| `scr_bindreminders_label_1` ... `_8` | See below | Caption beside the key; empty uses the detected FPS action or other command text. |

Older `hud_bind_reminders_x`, `hud_bind_reminders_y` and
`hud_bind_reminders_visible` settings are imported on startup, along with
`scr_bindreminder_command_N` / `scr_bindreminder_label_N` custom slots. Each old
value is used only when its new `scr_bindreminders_*` name has not been set.
Reset still restores the defaults above. Old names are not live aliases: use the
new names in scripts loaded later and in external menus.

| Custom row | Command | Caption |
| --- | --- | --- |
| 1 | `toggle cl_maxfps 30 120` | `30 / 120` |
| 2 | `store` | `Store` |
| 3 | `recall` | `Recall` |
| 4 | `reset` | `Reset` |
| 5-8 | empty | empty |

The older default caption `FPS 30 / 120` also displays as `30 / 120`, without
rewriting its saved setting. Custom rows show up to two matching keys, `+` for more, or `--` when unbound.
Their lookup compares the whole binding, ignoring case but preserving spaces and
arguments. An unmatched compound binding such as `store; say saved` needs its full
text in a custom row. The menu stores up to 255 command and 127 caption characters.

In `hud_edit`, **Bind reminders** is in **Player state**. By default the panel sits
8 HUD pixels from the right edge, with its top at 52.5% of the screen height.
It grows inward as labels widen and moves upward if needed to fit a longer list.
Move, hide or show it with the editor. Text is clipped after applying saved offsets.
Keys use the current keyboard layout. All settings are archived, and existing
bindings are never changed by discovery.

Existing visibility settings and position offsets are preserved. To apply the new
default placement to an older config, reset **Bind reminders** in the HUD editor
and apply, or enter:

```cfg
scr_bindreminders_visible 1
scr_bindreminders_x 0
scr_bindreminders_y 0
```

Disabled reminders skip binding discovery during gameplay. Discovery only
inspects text; it never executes a binding, alias or macro callback.
Alias scanning is bounded by the engine's alias depth limit and 128 commands per
press or release scan. Dynamic command names/targets built with `$` macros and commands loaded
only by a future `exec` cannot be inferred. Such bindings can still be added as
custom rows. The editor uses the current reminder rows, including when disabled,
so positioning and alignment match the live panel's dimensions. An empty list
uses a fixed sample with the same panel and keycap renderer.
A thin, dim keycap outline marks bindings whose known numeric FPS values match
`cl_maxfps`. Direct hold/release bindings compare only the held value while that
key is down and only the release value while it is up. This also applies to
hold/release slots and the known phases of raw button aliases. For example, with
M1 bound to `+fps 20 120` and M2 to `+fps 20 60`, holding only M1 at 20 FPS outlines
only M1. Releasing it at 120 FPS keeps M1 outlined. Manual grouped rows check every
matching key, including keys beyond the two displayed labels.

Ordinary FPS setters and listed toggle values remain value-based. Multiple bindings
can still match their current phases, such as two keys held at the same FPS. Custom
captions do not affect matching. Conditions, multi-command scripts, relative changes
and unresolved values are not guessed; the outline does not track which key was last
pressed.

## Strafe Helper HUD

The angle bar follows the final command of local movement prediction, including
the 30 UPS projection cap on air-acceleration servers. It clears when prediction
is disabled or unavailable, including demo playback. Earlier replayed commands and
listen-server movement do not update the local helper. Smoothing uses elapsed
client time and resets when the strafe side changes.

The angle bar also supports strafing while swimming at waist depth or fully
submerged. It uses the movement code's water speed limit and acceleration,
post-friction velocity, view pitch, and jump/crouch input. Its angles maximize
horizontal acceleration added after friction; water drag can still cause total
UPS to fall. Vertical-only currents and opposing flags that cancel within the
same source are supported. Nonzero horizontal water currents and grounded
conveyors hide the bar at every water depth, including shallow water and dry
ground, because their fixed direction is outside the yaw model.
Ladders, forced water jumps, commands without sidemove, and movement with
no possible horizontal acceleration gain also have no angle bar. The efficiency
percentage remains limited to airborne movement outside liquids.

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
| `sh_y` | `0` | Offset from screen center; negative moves upward. Console/editor support -4000 to 4000; the menu slider covers -320 to 320. |
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
| `sh_efficiency` | `1` | Enables the efficiency display. |
| `sh_efficiency_style` | `text` | Display style: `bar`, `text` (percentage readout), `both`, or `none` (helper tint only). |
| `sh_efficiency_tint` | `off` | Tints strafe helper elements by current efficiency: `optimal` (optimal-angle marker), `zone` (accelerating zone), or `both`. Reverts to the configured helper colors when no data is live. |
| `sh_efficiency_tint_strength` | `0.75` | How strongly the tint overrides the helper's configured colors (0-1); element alpha is always preserved. |
| `sh_efficiency_width` | `80` | Bar width in HUD units (8-4000). |
| `sh_efficiency_height` | `4` | Bar height in HUD units (1-64). |
| `sh_efficiency_x` | `0` | Horizontal offset from screen center in HUD units (-4000 to 4000). |
| `sh_efficiency_y` | `0` | Gap below the helper bar in HUD units (-400 to 400); negative values place the display above it. |
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
| `sh_ups_scale` | `1` | Text scale for the UPS display (0.25-8). |
| `sh_ups_x` | `0` | Horizontal offset from screen center for the UPS display, in HUD units (-4000-4000). |
| `sh_ups_y` | `-15` | Vertical offset for the UPS display (-4000-4000). |
| `sh_ups_shadow` | `1` | Draws a shadow behind UPS text. |
| `sh_ups_hide_zero` | `1` | Hides the UPS display when rounded speed is zero. |
| `sh_ups_color_mode` | `dynamic` | UPS color mode. |
| `sh_ups_color_gain` | `0 255 0 255` | RGBA color for speed gain. |
| `sh_ups_color_loss` | `255 0 0 255` | RGBA color for speed loss. |
| `sh_ups_color_neutral` | `255 255 255 255` | RGBA neutral UPS color. |
| `sh_ups_format` | `plain` | UPS text format. |
| `sh_ups_3d` | `0` | Uses 3D speed including vertical velocity. |

## Race Line and World Origin

Race line settings apply to server-provided beams only while `game` is `jump`.
Other games retain normal BFG beam lifetime, width, palette color and opacity.
Rail cores retain their own color, width, lifetime and fading in every game.
`gl_beamstyle` remains a global rendering option.

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
| `sh_netmeter` | `3` | Selects 0 = off, 1 = lagometer, 2 = netgraph, or 3 = histogram. Values outside 0-3 are clamped to the nearest supported mode when drawn. |
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

`PACKET LOSS` reports server-to-client packet loss. `CLIENT DROP` reports
missing client movement commands, including cases where the server exhausted
backup commands and predicted movement. It does not mean a disconnect.
The other incident labels are `PING SPIKE` and `NETWORK JITTER`.

| Cvar | Default | Description |
| --- | --- | --- |
| `sh_netalert` | `1` | Enables network incident text alerts independently of the network graph; alerts also work with `sh_netmeter 0`. |
| `sh_netalert_x` | `0` | Alert X position. |
| `sh_netalert_y` | `353` | Alert Y position. |
| `sh_netalert_color` | `1` | Alert color selection; `7` selects severity-based colors. |
| `sh_netalert_alpha` | `1` | Alert text alpha. |
| `sh_netalert_duration_ms` | `2000` | Alert lifetime in milliseconds, clamped to 250-5000. |
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
| `sh_lagometer_color_loss_c2s` | `243` | Palette index for reported client movement-command loss. |
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
| `sh_netgraph_color_loss_c2s` | `11` | Palette index for reported client movement-command loss. |

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
| `sh_histogram_color_loss_c2s` | `241` | Palette index for reported client movement-command loss. |
| `sh_histogram_alpha` | `1` | Alpha for normal samples. |
| `sh_histogram_bad_alpha` | `1` | Alpha for incident samples. |
| `sh_histogram_history_ms` | `52000` | Histogram history window in milliseconds. |
| `sh_histogram_ping` | `1` | Draws smoothed ping text near the histogram. |

## MP4 Recording

These cvars and `mp4record [filename]`, `mp4stop`, and `mp4status` require a
build with FFmpeg support (`avcodec`). The configured Windows CI build disables
that feature, so it does not provide these commands. Native `record` / `stop`
demo recording is separate and does not require FFmpeg. MP4 output is written
to the active game directory's `video/` folder; available encoders depend on
the linked FFmpeg build.

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
| `cl_menu_cursor` | `ch1` | In-game cursor image; Windows uses it in exclusive fullscreen. Accepts image names, including `cross`/`ch1`, `dot`/`ch2`, `angle`/`ch3`, `ch4`, and `ch5`. `none` hides it except during display confirmation. |
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

The default `ui_menu_model_position bottom` preset is applied when the UI starts
and sets the active X/Y position to `0.5` / `0.8`. The X/Y table entries are their
registered defaults; use `custom` positioning to keep independent coordinates.

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

#### In-Game Menu layout

The built-in and bundled external menus use this in-game layout:

```text
In-Game Menu
  Join Game

  Quick Settings
  Quick Binds

  HUD & Jump Settings
  All Settings

  Find Jump Servers

  Disconnect
  Quit Game
```

**HUD & Jump Settings** opens **Jump Setup**, whose menu ID remains `jump`.
**All Settings** opens the existing options menu. **Join Game** retains its
team and voting actions. Address Book remains available through All Settings
and Multiplayer.

**Quick Binds** opens **Quick Bindings** (`bindings_quick`). Its existing
binding controls are arranged in three groups, separated by blank rows:

| Group | Commands in row order |
| --- | --- |
| Movement | `+moveup`, `+dj`, `+movedown`, `+forward`, `+back`, `+moveleft`, `+moveright` |
| Scores and server menu | `score`, `inven`, `invuse`, `invprev`, `invnext` |
| Teams and jump actions | `team easy`, `team hard`, `kill`, `store` |

#### Jump Settings layout

The built-in menu and the bundled external `q2pro.menu` use this layout.
The root has four choices, with related settings grouped into submenus.

```text
Jump Setup
  HUD & Visuals
    Strafe Helper
      Color Presets
    Efficiency Display
    Speedometer
    Race Line
    Bind Reminders
  Controls & FPS
    Movement & Menu Bindings
    Jump Action Bindings
    FPS Controls
      FPS Preset Bindings
      Hold / Release FPS Bindings
  Network & Advanced
    Network Bar
    Network Alerts
    Warning Thresholds
    Advanced / Debug
    Reset All Network Settings

  Edit HUD Layout
```

Navigation pages use menu-width focus highlights. Strafe Helper, Efficiency
Display, Speedometer, and Network Alerts keep their settings visible while
disabled; controls that depend on a selected mode still follow that mode.
Efficiency Display is always accessible under HUD & Visuals.

The In-Game Menu's Quick Settings includes **mute player jump sounds**, which remains visible when
sound is disabled. Full sound settings remain under **All Settings > sound setup**,
where the same mute option is also available. Advanced / Debug includes the world-origin marker
toggle alongside its size and line controls; Quick Settings retains its
world-origin shortcut.

Existing menu IDs, cvars, bindings, and config commands retain their names.
Network & Advanced has one **Reset All Network Settings** action; it resets the
bar, alerts, and thresholds together through `ui_reset_netmeter_settings`.

#### Menu script behavior

The menu script supports `style --live` for controls that write cvars as the user
changes them. Individual controls can use `--defer` to stay pending in a live
menu and commit when it closes. The Graphics and Brightness/Color pages defer
selected controls. Advanced Rendering uses live controls in the built-in menu
and commits on leaving in the external script. The native Display page has its
own Apply/Keep/Revert flow; see [display settings](display-settings.md).

The `plaque` command accepts an optional position argument: `left`, `right`, `top`, `bottom`, `center`, `screen-topleft`, `screen-topright`, `screen-bottomleft`, or `screen-bottomright`.

Top-level `style` and `focus` commands apply defaults to subsequent `begin`
menus. `style` controls layout and update policy, including `--compact`,
`--center`, `--transparent`, and `--live`. `focus` controls the highlight,
including `--border`, `--inset`, and `--height`. Per-menu commands override the
corresponding defaults.

For actions, `--align` selects left alignment and alternate label color;
`--status` supplies hint text. Bind controls accept `--align` for compatibility
but do not change alignment; `--altstatus` sets the hint shown while waiting
for a key.

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

The editor is available only when `game` is `jump`. Its element list is limited
to the recognized Grish jumpmod HUD groups, server menus/scores, and client overlays.
Generic unrecognized server HUD content is excluded.
Open **Jump Setup → Edit HUD Layout**, or run `hud_edit`.
The editor works over the current view or from the main menu. Multiplayer
continues while editing; entering the editor does not pause a server.

The workbench has a fixed element list on the left and a command bar along the
bottom, plus a preview toolbar above the canvas. Fit preserves the scene's aspect
ratio inside the remaining space. Editing coordinates stay in the original HUD
coordinate system; zoom and pan do not change saved positions or scales. With no
map loaded, the preview uses the existing `q2jump_background` menu artwork (or a
neutral fill if unavailable). Live + Fit retains an active map's scene. Sample
scenarios and manually zoomed/panned views use the static backdrop, which follows
the same transform as the HUD; they do not magnify or freeze the live 3D scene.

- Preview scenarios: **Running**, **Spectating**, **Voting**, **Scoreboard**, and
  **Net warning** make the corresponding temporary Grish/client groups available
  without connecting to a server. Voting includes a sample map, tally and timer;
  Scoreboard includes sample player rows. These are representative arrangement
  samples, not an exact reproduction of each server screen. **Live** restores
  availability based on the current client context. The existing Grish/client
  scope, Show disabled, Focus, and per-element editor visibility rules still apply.
- **Fit** restores the complete canvas and recenters it. **100%** uses the normal
  in-game HUD pixel scale; **Selection** centers and zooms to the selected bounds
  with a margin, capped at 400%. **- / +** zoom around the view center. Hold Ctrl
  and use the wheel over the canvas to zoom at the pointer (25-400%). Plain wheel
  navigation remains unchanged. Toolbar controls and text fields keep their own
  input behavior.
- Hold the middle mouse button, or Space + left mouse button, and drag within the
  canvas to pan. Pan ends on release or focus loss; Fit returns to the full view.
  Scenarios, zoom, pan, and information expansion are session-only view controls:
  they neither write cvars nor create undo entries. Opening the editor starts in
  Live + Fit with the compact information strip.

- Click an overlay or its row to select it. The list groups elements by purpose;
  click a group heading to collapse it. All / Client / Server (keys 1-3) narrow
  the list. Client includes jump overlays and custom text; Server includes
  the 19 recognized Grish status-bar groups and server menus/scores.
  Rows show CL/SV, a dirty `*`, and two separate button columns: **Editor**
  (Show/Hide) and **HUD** (Enable/Disable, or Locked). Highlighted buttons indicate
  an element currently shown in the editor or enabled in the HUD, respectively;
  the button text names the action. Hover a row for its full name and states, or
  a button for its effect. The heading counts listed rows and how many of them
  are enabled in the HUD; enabled does not mean a live event is currently drawing.
  The compact information strip always shows Editor Shown/Hidden, HUD
  Enabled/Disabled, and the selected element's live state and reason. **More info** expands its contents, appearance
  conditions and supported edits; **Less info** recovers the canvas space.
- `/` focuses the name/key filter. Filtering also reveals matches in collapsed
  groups. Tab / Shift+Tab cycle matching elements; mouse wheel changes selection,
  and PgUp/PgDn move by a page. While filtering, Up/Down select matching elements,
  Enter leaves the field, and Ctrl+A followed by Backspace clears the query.
- Drag to move. The strafe helper remains horizontally centered, and efficiency
  keeps its position relative to the helper. Netgraph mode remains full width.
  Drag the lower-right handle to resize. Server/client groups and the lagometer
  scale proportionally; the classic full-width graph scales vertically and keeps
  its bottom edge fixed. Native UPS, helper, efficiency and other network modes
  retain their existing text/angle/dimension resize controls.
- X / Y focus the selected element's screen-coordinate fields. These are the
  measured top-left coordinates in HUD units, not the underlying cvar offsets.
  S edits visual scale for every editable item (`0.25` to `4`, default `1`).
  This multiplier includes the item's text, icons, borders and bars. Full-width
  network graphs scale vertically while retaining full width. Existing native
  text, angle and dimension settings remain separate. Locked fields show `--`.
  Enter commits the field
  into the draft; Esc abandons that field edit. Invalid numbers remain in the
  field until corrected or abandoned. Modifier keys preserve replacement of the
  opening value; cursor and editing keys switch to editing in place.
  Clicking the active field preserves its pending text and cursor position.
  Position and scale limits still apply.
  Cancel discards the draft even when a field contains invalid text.
- The arrow pad moves by the selected step (1, 5 or 10 HUD units); click its number
  to cycle the step. Hold a pad button to repeat. Keyboard arrows use the same
  step; Shift multiplies it by ten. Ctrl+arrows resize. UPS and text-only
  efficiency resize horizontally through text scale; the helper's horizontal
  resize changes angle scale. Bar/both efficiency and the network meter resize
  width and height. Histogram horizontal resizing selects custom-width mode.
- Snap / N aligns dragging to screen edges/center and other visible elements'
  edges/centers within six screen pixels at any preview zoom. Cyan guides mark
  the current snap; Alt temporarily bypasses it.
  Align places the selection at the left, center, right, top, middle or bottom
  of the screen. Toggle Screen to Ref to align to the previously selected
  element instead. Without a usable reference, alignment uses the screen.
- **Show in Editor / Hide in Editor** (E), also available in each row's Editor
  column, affects only that element's preview. A hidden element remains listed
  and keeps its measured bounds for coordinate editing/reference alignment, but
  its drawing, outline, label, resize handle and canvas hit target are hidden.
  Hidden elements are skipped as snap targets. Show can reveal disabled or
  inactive samples without enabling them in the HUD. Explicit Show/Hide choices
  survive selection, scenarios, gameplay Undo/Redo and reset during the session;
  opening the editor again restores automatic preview visibility. Focus still
  restricts drawing to the selected element. Editor visibility does not write
  cvars, create undo entries or change gameplay, and can be changed while locked.
- **Enable in HUD / Disable in HUD** (V), also available in each row's HUD
  column, changes the element's gameplay visibility in the draft. Apply saves
  that setting; Cancel discards it. HUD enable/disable supports Undo/Redo and
  respects element locks. It does not override an explicit editor Show/Hide
  choice or the element's normal gameplay event/feature conditions.
- **Show disabled** (G) controls automatic previews of disabled elements and
  starts enabled. Unless explicitly hidden, the selected element has a preview.
  Other transient panels/messages appear automatically when their live group is
  drawing or their sample scenario is selected. Explicit Show can reveal them
  outside that context; explicit Hide takes precedence over Show disabled and
  automatic selection/scenario visibility. Disabled NetMeter previews use
  histogram mode. These controls never put sample data into the live HUD.
- R / Reset restores the selected group's default layout and visibility in the
  draft. **Reset to Default**, at the bottom of the element list, or Shift+R
  restores defaults for all unlocked editable elements, including custom text
  offsets, regardless of the current filter or category. Locked and excluded
  elements are untouched. Reset is undoable; Apply saves it and Cancel discards
  it with the rest of the draft.
- Details / P opens the pending-changes list. Click an entry to select it or
  `revert` to restore that element's opening values and lock state. A revert
  never overrides another element's lock: the strafe helper keeps its values
  while Strafe efficiency is locked. Scroll the list with the wheel. Esc closes
  it without discarding the draft. Undo and Redo reselect the affected element,
  clearing the filter or expanding its group when needed.
- Undo / Ctrl+Z and Redo / Ctrl+Y (also Ctrl+Shift+Z) retain up to 64 editing
  actions during the session. Each drag, resize, held pad gesture, field commit,
  per-element revert, HUD enable/disable, alignment, reset or lock change
  is undoable. A new edit after Undo replaces the redo branch. Navigation,
  filtering, view controls and opening overlays do not consume history.
- Lock / L protects movement, resizing, alignment, HUD enable/disable and resets. Locked
  elements remain selectable. Where a locked element overlaps an unlocked one,
  clicking the overlap selects and drags the unlocked element. Overlapping unlocked
  elements follow their preview drawing order; an already selected element keeps
  priority. Locking efficiency also protects the strafe helper that positions it.
  Apply accepts lock changes as archived `hud_lock_<key>` cvars
  (default `0`); Cancel discards them. Undo may restore an earlier lock state.
  Locking **Ping graph / connection icon** also protects its shared lagometer
  position. Locking **Classic network / debug graph** protects its shared
  netgraph height. The affected Network monitor mode reports the dependency
  and blocks editing until the related element is unlocked. Histogram controls
  remain independent. Reset and per-element revert preserve protected
  shared settings even when another network mode is selected.
- F focuses only the selected preview. Other elements retain measured bounds
  for reference alignment. H hides or restores the workbench, returning the scene
  to full size while hidden. The workbench stays visible during dragging.
- `?` opens the keyboard map. Esc closes the map or pending-changes overlay,
  leaves a focused field, or otherwise cancels the editor. Enter commits a
  focused field, or otherwise applies and closes the editor. Apply changes live
  cvars; archived settings use the client's normal configuration-saving path.
  Opening the console, changing maps, disconnecting or restarting the UI also
  discards an unapplied draft.

The original jump overlays are named **Speed (UPS)**, **Strafe helper**,
**Strafe efficiency**, and **Network monitor** in the editor.
Their sample speed, efficiency and graph values are local visual previews.
They do not enter movement measurements or network history. The histogram's
optional ping text is included in its preview with a sample value of 42.
The legacy ping graph preview uses its 48-by-48 area and current lagometer
position, including draft changes, negative edge offsets and off-screen positions,
matching the live legacy graph. The native Network monitor retains viewport
clamping. Classic network/debug graph previews use the current draft height,
span the HUD width and grow upward from the bottom; detailed mode retains its
10-200 pixel range. Bounds and preview
positions use HUD coordinates independently of menu scale. Resizing the window
or losing focus ends an active drag.

UPS and efficiency text preview boxes use the height of the displayed font line,
without extra vertical padding, and include their configured text scale; shadows
do not add to the selection height. Layout group boxes keep the full extent of
the live group (or its representative size when nothing was captured), even when
the sample text is a single line, so clamping, snapping and alignment keep the
whole group on screen. These preview bounds do not change gameplay text size.

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
and registered `draw` text objects, including dynamic-color objects (up to 87
custom objects). Dynamic colors use `draw <macro> <x> <y> dynamic`; there is no
separate `draw_dynamic` command.
Custom text registered while editing becomes available after reopening the editor.
No server or protocol changes are required.

### Server HUD groups

The recognized Grish jumpmod `ctf_statusbar` contains 19 independently movable groups:

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
number alone. Unrecognized status bars still render as a single group and retain
existing `hud_other_status_*` settings, but **Other server HUD** is excluded from
the editor list, selection, previews, and reset. **Scoreboard / server
menus** remains a separate whole-panel control for Grish menus and scores; its
individual rows, selection logic and contents are preserved.
Server stats, input image indices, timer precision, conditions, and replay/chase
values are never rewritten. **Reported player FPS** retains the server-reported value
rather than becoming a measured local rendering rate.

### Client HUD groups

Independent position, scale and visibility controls cover **Chat history**, **Center messages**,
**Console messages**, **Chat input**, **Network alerts**, **Ping graph / connection icon**,
**Frame / prediction warnings**, **Inventory**, and **Classic network / debug graph**.
Crosshair, hit marker, debug movement, debug frame stats, nerd stats, loading,
demo progress, and pause are excluded from the editor list, selection, and previews.
Their normal gameplay behavior and existing saved settings are retained.
The full console, ordinary client menus, world-space race lines and other 3D effects
are not HUD layout elements.

**Render FPS** (`render_fps`, measured `r_mfps`) and **Movement rate (MPS)**
(`move_fps`, measured `cl_mmps`) are optional new widgets, off by default. Enable them
with V / Enable in HUD. Existing custom FPS/UPS/ping text remains independently editable
under its original macro/cvar name; no `draw` commands are replaced.

### Saved offsets and previews

Each additional group has archived `hud_<key>_x`, `hud_<key>_y` (default `0`),
`hud_<key>_scale` (default `1`, range `0.25` to `4`) and `hud_<key>_visible`
(default `1`, except render FPS and movement rate). Offsets are in HUD units and
are applied after visual scaling. Scale `1` preserves existing layouts.
Bind reminders use `scr_bindreminders_x`, `scr_bindreminders_y`,
`scr_bindreminders_scale` and `scr_bindreminders_visible` instead.

The four native elements use `hud_ups_scale`, `hud_strafe_scale`,
`hud_efficiency_scale` and `hud_network_scale`, with the same default and range.
The helper stays centered and efficiency remains attached to its scaled height,
with its own independent scale. Network scale is shared across monitor modes;
full-width netgraph and full-width histogram modes retain their horizontal extent.
These are presentation settings only; they do not change movement, server values
or the number of network samples represented by a native graph.

Group scaling uses stable screen anchors; custom `draw` objects use their
configured text anchor. Editor scale changes also adjust group offsets to retain
its selected top-left position (bottom edge for the classic full-width graph).
Console scale changes retain the native anchor instead. Hidden/locked state,
Apply/Cancel, undo/redo and reset include scale alongside the other saved settings.
Editor viewport zoom and global `scr_scale` remain independent of item scale.
For example, `hud_timer_x -80` moves the complete run timer 80 HUD units left.
Custom draw objects use stable `hud_draw_<name-hash>_*` keys so config command order
can change without assigning a saved position to a different object.

The information strip distinguishes **Disabled** (layout or an underlying
feature is off), **Waiting for event** (no map or no current live output),
**Visible** (the live layout group produced drawing bounds this frame), and
**Preview only** (native speed/helper/efficiency/network sample values or a group
included in the selected sample scenario). Disabled feature settings retain their
Disabled state even when a sample is shown. A selected or ghost sample can remain on the canvas in any of
these states. State text describes the source of the content, not whether a
sample is selected, clipped, or hidden by Focus. Live notification/chat-input
panels and network alerts are suppressed while editing; their state explains
that a selected sample is available. Dynamic Grish run-info descriptions retain
the server-defined meaning rather than assigning a fixed checkpoint/lap label.

Visibility is an additional local filter. Allowing a group does not override its
normal feature setting or server condition: chat still needs `scr_chathud`, pause
still requires a paused game, and zeroed server input stats remain hidden. Reset
clears the selected group's added offsets/filter; it does not reset its original
feature settings. Demo progress is excluded from editing; any existing saved
demo offsets and its normal viewport reservation are retained.

Additional groups use labeled preview boxes, with the last captured drawing bounds
when available and representative bounds for inactive/unseen content. Original
client position cvars are honored for crosshair, hit marker, chat and network-alert
fallbacks. Custom text positions are read when opening the editor. All editable groups retain preview bounds for reference alignment, including
inactive or hidden groups; per-element editor visibility, Show disabled and Focus
control their emission. Sample text,
including scenario panels, does not enter live stats, chat history, input state,
timers, or the live bounds cache. H hides the
workbench when working near screen edges. Per-element color editing and
user-saved named layouts are not implemented.
