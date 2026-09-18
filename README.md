# q2pro_race

`q2pro_race` is an enhanced fork of [Q2PRO](https://github.com/q2pro/q2pro) by skuller, tuned for [q2jump](http://q2jump.net) race and jump play.
It incorporates work from [q2pro-speed](https://github.com/kugelrund/q2pro-speed) and [q2pro-jump](https://github.com/TotallyMehis/q2pro-jump), and includes upstream Q2PRO code.

This fork focuses on practical race tooling: visual assist overlays, centered speed feedback, modular network diagnostics, an expanded in-game setup menu, and configuration improvements for advanced jump gameplay.

## Highlights

### Strafe Helper HUD

A configurable helper bar for reading strafe angles and acceleration behavior while playing.

* Show or hide the helper HUD with `sh_draw 1` or `sh_draw 0`.
* Adjust position, scale, height, alpha, and marker widths.
* Enable an optional center marker.
* Show an airborne acceleration-efficiency bar with `sh_efficiency 1`.
* Choose between solid, gradient, outline, and minimal bar styles.
* Smooth helper movement to keep the HUD readable during fast runs.
* Customize accelerating-zone, optimal-marker, and center-marker colors.
* List color presets with `sh hud preset`; apply one with `sh hud preset <name>`.

### Center UPS Display

A lightweight center speed readout for tracking UPS during jump and race play.

* Control it with `sh ups enable`, `sh ups disable`, or `sh ups toggle`; inspect it with `sh ups status`.
* Adjust scale and vertical position for different HUD layouts.
* Choose dynamic, static, threshold, rainbow, gradient, or strafing color modes.
* Keep speed feedback readable without covering the route.

### Modular NetMeter (`sh_netmeter`)

The NetMeter replaces the legacy static network bar with customizable diagnostics overlays:

* **Lagometer, Mode 1**: compact 48x48 packet-quality monitor.
* **Netgraph, Mode 2**: full-width scrolling packet history with linear ping-height scaling.
* **Histogram, Mode 3**: a configurable time-based ping and network-event history.
* **Adaptive scaling**: automatically adjusts display limits against your recent rolling ping baseline.

### Network Alerts (`sh_netalert`)

On-screen warnings call out network incidents that can affect a race run:

* `PACKET LOSS`: incoming server-to-client dropped packets.
* `CLIENT DROP`: server-reported dropped or reconstructed client movement commands.
* `NETWORK JITTER`: elevated smoothed differences between consecutive ping samples.
* `PING SPIKE`: sudden latency jumps compared with baseline.

### Dynamic FPS Binds

Quickly change cl_maxfps to match different run segments.

* **Press-and-release binds (`+fps` / `-fps`)**: hold a key to drop FPS and release it to restore, for example `bind space "+fps 60 120"`.
* **Console shortcuts (`f20` to `f120`)**: set `cl_maxfps` instantly, for example `f60`.
* **FPS toggles (`toggle`)**: cycle through values with `bind mouse4 "toggle cl_maxfps 30 120"`. If the current FPS is outside the list, the next press selects its first value.

### Bind Reminders

Bind reminders are on by default at the right edge, just below screen center. The compact panel automatically finds loaded FPS bindings, shortcuts such as `f20`, hold/release actions and aliases; a thin outline marks known values matching the current FPS. Configure it in **Jump Settings > HUD & Visuals > Bind Reminders**. Configure Menu, Respawn, Store and Observer in **Extended reminders**, or add up to eight custom reminders. Keys, FPS-slot values and captions update with your config; opacity and position are configurable. See [settings and examples](doc/custom-cvars.md#bind-reminders).

### Step Smoothing (`cl_step_smoothing_mode`)

Controls how the camera interpolates when the player climbs stairs and steps. This changes camera presentation without changing movement physics.

* `q2pro` (default): standard Q2PRO stair smoothing.
* `r1q2-1` / `r1q2-2` / `r1q2-3`: alternative R1Q2-style smoothing modes.

### HUD Layout Editor

Open **Jump Settings → Edit HUD Layout** or run `hud_edit` to arrange jump overlays, all 19 recognized server HUD groups, client overlays, and custom draw text with the mouse. Move and resize supported elements, snap to screen edges/center, preview hidden elements, and apply or cancel the draft. See [editor controls](doc/custom-cvars.md#drag-and-drop-hud-editor).

### Expanded Menu System

The menu has been rebuilt around the race and jump workflow while keeping classic Q2PRO options available.

* **Jump Settings pages**: strafe helper, efficiency display, UPS, server-supplied race-line styling, bind reminders, network diagnostics, FPS controls, movement/JumpMod bindings, and debug tools.
* **Animated player model preview**: a 3D player model is rendered behind top-level menus, with configurable position presets, orbit rotation, and distance (`ui_menu_model*` cvars).
* **Dropdown and binding widgets**: `select`, `select2`, and `bindselect2` add selection controls alongside the existing `values`, `pairs`, and `bind` widgets.
* **Improved main flow**: multiplayer, jump server browsing, address book access, quick video tuning, and HUD/helper setup are easier to reach.
* **Built-in menu fallback**: compiled menu data is available when `game` is `jump`; other games can continue to load external `q2pro.menu` data.
* **Conditional visibility (`--show-if`)**: controls appear only when their related settings are active.
* **Live controls and status hints**: many sliders, toggles, and fields apply immediately and show short bottom-bar hints.
* **Color picker and swatches**: color fields show live previews and can open an RGBA picker with current/original previews and preset swatches.
* **Menu styling**: `Menu Setup` exposes menu scale, cursor selection, menu source, and color style; `Custom Menu Colors` exposes archived `ui_menu_color_*` cvars plus `ui_reset_menu_colors`.

## Position and Speed Macros

### Position Tracking

* `cl_playerpos_x`: player's X coordinate.
* `cl_playerpos_y`: player's Y coordinate.
* `cl_playerpos_z`: player's Z coordinate.

### Speed Metrics

* `cl_ups`: horizontal velocity in units per second.
* `cl_rups`: 3D velocity in units per second, including vertical velocity.
* `sh_ups_3d`: a cvar selecting horizontal or 3D speed for the center UPS widget.
* `cl_fps` / `r_fps`: rates derived from the movement/render frame intervals.
* `cl_mmps` / `r_mfps` / `cl_mfps`: measured movement, render, and main-loop rates.

## Custom Cvars

See [doc/custom-cvars.md](doc/custom-cvars.md) for the current list of race, HUD, network, sound, menu and video cvars added by this fork.

## Install

1. Extract the client package into your Quake II data directory, retaining its `jump/` assets.
2. Make sure the original Quake II base assets are present.
3. Launch the client.
4. Open the in-game menus and tune the helper, network overlays, colors, and binds until the setup feels right.

## Build

Q2PRO uses the Meson build system. A basic local build looks like this:

```sh
meson setup buildDir
meson compile -C buildDir
```

For platform-specific dependencies, portable builds, and Windows/macOS build notes, see [INSTALL.md](INSTALL.md).

Run the standalone suites with:

```sh
meson test -C buildDir --suite jump-hud --suite video --suite console --print-errorlogs
```

With `client-ui=true`, Meson registers 13 jump/HUD tests, four console tests,
and one video test, plus a second video test on Windows: 18 on Linux and 19
on Windows. Meson builds the fixture targets automatically; no game assets or
live server are needed. The Linux fixture configuration works without OpenGL
headers or a video backend, but does not prove that a full client was built.
Keep UI enabled for full client builds and the five UI-dependent jump/HUD tests.
CI runs all three suites on Linux and Windows with `-Db_ndebug=true`; assertions
remain enabled. Windows jobs initialize the x86/x64 MSVC toolchain through
Visual Studio's Developer PowerShell.
Configuration, compilation, and tests run in separate Windows CI steps so a failure stops artifact packaging.

CI also builds macOS Apple Silicon and Intel clients with Clang, Homebrew and
Meson (Intel uses Rosetta). It packages separate ZIPs with the client, base game
library and selected `jump/` assets for the existing release workflow. See the
[macOS recipe and runtime requirements](INSTALL.md#macos); native macOS build
and runtime results must be checked separately from this configuration.

## Developer documentation

* [Setup and dependencies](INSTALL.md)
* [Jump/strafe-helper integration](src/jump/README.md)
* [Display settings and validation](doc/display-settings.md)

The feature list describes source present in this checkout. It does not
establish what a published binary contains.

## License

* **Q2PRO** is licensed under GPL-2.0.
* **q2pro-jump** is licensed under GPL-2.0.
* The imported **strafe_helper** component uses [MPL-2.0](src/jump/LICENSE); the jump directory also includes GPL-derived additions.
* Modified files retain their notices. See [NOTE](NOTE) for the unresolved per-file attribution boundary.
