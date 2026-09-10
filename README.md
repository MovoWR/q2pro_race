# q2pro_race

`q2pro_race` is an enhanced fork of [Q2PRO](https://github.com/q2pro/q2pro) by skuller, tuned for [q2jump](http://q2jump.net) race and jump play.
It incorporates work from [q2pro-speed](https://github.com/kugelrund/q2pro-speed) and [q2pro-jump](https://github.com/TotallyMehis/q2pro-jump), while continuing to track upstream Q2PRO changes.

This fork focuses on practical race tooling: visual assist overlays, centered speed feedback, modular network diagnostics, an expanded in-game setup menu, and configuration improvements for advanced jump gameplay.

## Highlights

### Strafe Helper HUD

A configurable helper bar for reading strafe angles and acceleration behavior while playing.

* Toggle the helper HUD with `sh_draw`.
* Adjust position, scale, height, alpha, and marker widths.
* Enable an optional center marker.
* Show an airborne acceleration-efficiency bar with `sh_efficiency`.
* Choose between solid, gradient, outline, and minimal bar styles.
* Smooth helper movement to keep the HUD readable during fast runs.
* Customize accelerating-zone, optimal-marker, and center-marker colors.
* Load HUD presets with `sh hud preset`.

### Center UPS Display

A lightweight center speed readout for tracking UPS during jump and race play.

* Toggle with `sh ups enable`, `sh ups disable`, `sh ups toggle`, or `sh ups status`.
* Adjust scale and vertical position for different HUD layouts.
* Choose dynamic, static, threshold, rainbow, gradient, or strafing color modes.
* Keep speed feedback readable without covering the route.

### Modular NetMeter (`sh_netmeter`)

The NetMeter replaces the legacy static network bar with customizable diagnostics overlays:

* **Lagometer, Mode 1**: compact 48x48 packet-quality monitor.
* **Netgraph, Mode 2**: full-width scrolling packet ribbon with hybrid log-linear scaling for ping fluctuations.
* **Histogram, Mode 3**: localized scrolling latency distribution graph with network-quality history.
* **Adaptive scaling**: automatically adjusts display limits against your recent rolling ping baseline.

### Network Alerts (`sh_netalert`)

On-screen warnings call out network incidents that can affect a race run:

* `PACKET LOSS`: incoming server-to-client dropped packets.
* `CLIENT DROP`: prediction error / client-side disconnect.
* `NETWORK JITTER`: high ping standard deviation.
* `PING SPIKE`: sudden latency jumps compared with baseline.

### Dynamic FPS Binds

Quickly change cl_maxfps to match different run segments.

* **Press-and-release binds (`+fps` / `-fps`)**: hold a key to drop FPS and release it to restore, for example `bind space "+fps 60 120"`.
* **Console shortcuts (`f20` to `f120`)**: set `cl_maxfps` instantly, for example `f60`.

### Step Smoothing (`cl_step_smoothing_mode`)

Controls how the camera interpolates when the player climbs stairs and steps. This is purely visual and does not affect movement physics or give any competitive advantage.

* `q2pro` (default): standard Q2PRO stair smoothing.
* `r1q2-1` / `r1q2-2` / `r1q2-3`: alternative smoothing styles from R1Q2, ranging from subtle to aggressive.

### HUD Layout Editor

Open **Jump/Race Setup → Edit HUD layout** or run `hud_edit` to arrange jump overlays, all 19 recognized server HUD groups, client overlays, and custom draw text with the mouse. Move and resize supported elements, snap to screen edges/center, preview hidden elements, and apply or cancel the draft. See [editor controls](doc/custom-cvars.md#drag-and-drop-hud-editor).

### Expanded Menu System

The menu has been rebuilt around the race and jump workflow while keeping classic Q2PRO options available.

* **Jump/Race setup pages**: strafe helper, UPS display, race line, NetMeter, network alerts, replay recording, FPS binds, mod binds, jump binds, and debug tools.
* **Animated player model preview**: a 3D player model is rendered behind top-level menus, with configurable position presets, orbit rotation, and distance (`ui_menu_model*` cvars).
* **Dropdown and bindfield widgets**: `select`, `select2`, `bindfields`, and `binds elect2` widget types replace old `values` menus with dropdown-based selection.
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
* `cl_rups` / `sh_ups_3d`: toggles 3D speed tracking, including vertical velocity.
* `cl_fps` / `r_fps`: reports engine frame rate.

## Custom Cvars

See [doc/custom-cvars.md](doc/custom-cvars.md) for the current list of race, HUD, network, menu and video cvars added by this fork.

## Install

1. Drop the binary files into your main Quake II directory.
2. Make sure the original Quake II base assets are present.
3. Launch the client.
4. Open the in-game menus and tune the helper, network overlays, colors, and binds until the setup feels right.

## Build

Q2PRO uses the Meson build system. A basic local build looks like this:

```sh
meson setup buildDir
meson compile -C buildDir
```

For platform-specific dependencies, portable builds, and Windows build notes, see [INSTALL.md](INSTALL.md).

Run the 11 jump/HUD regression tests with `meson test -C buildDir --suite jump-hud --print-errorlogs`.
Meson builds these test targets automatically. The suite also works without a video backend;
keep `client-ui` enabled to include the five UI fixtures. CI is configured to run it on Linux and Windows
with `-Db_ndebug=true`; test assertions remain enabled.
CI actions use Node 24. Windows jobs initialize the x86/x64 MSVC toolchain through
Visual Studio's Developer PowerShell.

## License

* **Q2PRO** is licensed under GPL-2.0.
* **q2pro-jump** is licensed under GPL-2.0.
* **strafe_helper** is licensed under MPL-2.0.
* Modified files remain covered by their original licenses.
