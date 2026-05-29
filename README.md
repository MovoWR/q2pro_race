# q2pro_race

q2pro_race is an enhanced fork of [q2pro](https://github.com/skullernet/q2pro), tailored specifically for playing [q2jump](http://q2jump.net).
It incorporates features from [q2pro-speed](https://github.com/kugelrund/q2pro-speed) and [q2pro-jump](https://github.com/TotallyMehis/q2pro-jump)
and remains synchronized with upstream Q2PRO changes.

This fork introduces visual assist overlays, dynamic menu rendering, modular network diagnostics, and configuration improvements for advanced race/jump gameplay.

---

## Key Features

### 1. Modular NetMeter Subsystem (`sh_netmeter`)
Replaces the legacy, static network bar with a highly customizable diagnostics overlay offering three modes:
* **Lagometer (Mode 1)**: Traditional, compact 48x48 pixel packet quality monitor.
* **Netgraph (Mode 2)**: Full-width scrolling packet ribbon with hybrid log-linear scaling to visualize ping fluctuations.
* **Histogram (Mode 3)**: A localized scrolling latency distribution graph tracking network quality history over time.
* **Adaptive Scaling**: Automatically adjusts the maximum scale limits to match your recent rolling ping baseline.

### 2. On-screen Network Alerts (`sh_netalert`)
Triggers real-time warnings overlaying the HUD to alert you of incidents that might impact a race run:
* `PACKET LOSS` (Incoming Server-to-Client dropped packets)
* `NETWORK JITTER` (High ping standard deviation)
* `PING SPIKE` (Sudden latency jumps compared to baseline)

### 3. Dynamic FPS Key Bindings & Shortcuts
Enables quick changes to the maximum frame rate to optimize physics at different run segments:
* **Press-and-release bindings (`+fps` / `-fps`)**: Press a key to drop FPS and release it to restore (e.g. `bind space "+fps 60 120"`).
* **Console Shortcuts (`f20` to `f120`)**: Instantly changes `cl_maxfps` via shortcut commands (e.g. typing `f60` sets `cl_maxfps 60`).

### 4. Advanced UI Improvements
* **Scrollable Menus**: Large configuration pages now scroll seamlessly using the mouse wheel (`K_MWHEELUP` / `K_MWHEELDOWN`) with scroll boundaries and indicators (`...`).
* **Conditional Visibility (`--show-if`)**: Menu controls dynamically hide or show based on related settings (e.g. hiding download category settings if downloads are globally disabled).
* **Color Swatches & Centering**: Slider controls for color configuration show a live preview color swatch. The color picker dynamically shifts to center-screen to avoid overlapping visual HUD items.

---

## Settings & Macros

### Position & Speed Macros
* **Position Tracking**:
  * `cl_playerpos_x`: Player's X-coordinate.
  * `cl_playerpos_y`: Player's Y-coordinate.
  * `cl_playerpos_z`: Player's Z-coordinate.
* **Speed Metrics**:
  * `cl_ups`: Horizontal velocity (Units Per Second).
  * `cl_rups` / `sh_ups_3d`: Toggles 3D speed tracking including vertical velocity.
  * `cl_fps` / `r_fps`: Reports engine frame rate.

### Cvar Mapping updates
For HUD customization and menus, several settings have been renamed or refactored:
* `sh_draw` (0-1) - Toggles the strafe helper HUD.
* `sh_height` - Adjusts the height of the helper bar.
* `sh_scale` - Modifies the visual scaling factor.
* `sh_y` - Sets the vertical position of the helper.
* `sh_optimal_outline` (0-1) - Renders the optimal acceleration zone as an outline.
* `sh_ups_color_mode` - Supports color presets: `dynamic`, `static`, `threshold`, `rainbow`, `gradient`, and `strafing`.


---

## Installation

1. Extract all binary files into your main Quake 2 directory.
2. Ensure the original Quake 2 base assets are present.
3. Access the menu inside the client to customize your helper, network overlays, and key bindings.

## Building

This project utilizes the **Meson** build system. To compile:

1. Configure the build directory:
   ```bash
   meson setup buildDir
   ```
2. Build the project targets:
   ```bash
   ninja -C buildDir
   ```

It can be compiled for Windows, Linux, and macOS. Refer to [INSTALL.md](file:///C:/Users/stasi/Desktop/q2pro_race_github/INSTALL.md) for prerequisite dependencies.

---

## License

* **Q2PRO** is licensed under the GPL-2.0 license.
* **q2pro-jump** is licensed under the GPL-2.0 license.
* **strafe_helper** is licensed under the MPL-2.0 license.
* Any modifications made to the files covered by those licenses follow their original licenses.
