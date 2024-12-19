# q2pro_race

q2pro_race is an enhanced fork of [q2pro](https://github.com/skullernet/q2pro), tailored specifically for playing [q2jump](http://q2jump.net).
It incorporates features from [q2pro-speed](https://github.com/kugelrund/q2pro-speed) and [q2pro-jump](https://github.com/TotallyMehis/q2pro-jump)
Remains synchronized with the latest upstream Q2PRO changes through integration with [q2jump-pro](https://github.com/q2jump-pro/q2jump-pro).
This ensures compatibility with ongoing updates and adds new functionality for advanced gameplay.

---

### Macros

- **Position Tracking**:
  - `cl_playerpos_x`: Displays the player’s X-coordinate.
  - `cl_playerpos_y`: Displays the player’s Y-coordinate.
  - `cl_playerpos_z`: Displays the player’s Z-coordinate.
- **Speed Metrics**:
  - `cl_ups`: Displays horizontal speed.
  - `cl_rups`: Displays total speed, including vertical movement.
  - `cl_fps`, `r_fps`: Reports accurate frame rates.
  - `cl_mfps`, `r_mfps`: Monitors measured frame rates.

### Race Line Enhancements

Visual tools for race line customization:

- **Race Line Settings**:
  - `race_width`: Adjust the thickness of the race line.
  - `race_color`: Choose from a range of colors for the racing line.
  - `race_alpha`: Set transparency level for optimal visibility.
  - `race_life`: Define how long the race line remains visible.

### Command Auto-Completion

Auto-completion accelerates gameplay by simplifying access to Q2Jump mod commands, reducing typing errors and saving time.

---

## Settings (Cvars)

### Renamed Cvars


To enhance clarity, these cvars have been renamed:

- ~~`cl_drawStrafeHelper`~~  -> `sh_draw`(0-1)                 _Toggles the display of the strafe helper HUD._
- ~~`cl_strafeHelperHeight`~~ -> `sh_height`              _Adjusts the height of the strafe helper HUD on the screen._
- ~~`cl_strafeHelperCenter`~~ -> `sh_center` (0-1)               _Enables or disables the center alignment of the strafe helper._
- ~~`cl_strafeHelperCenterMarker`~~ -> `sh_centermarker` (0-1)   _Displays a visual marker at the center of the strafe helper._
- ~~`cl_strafeHelperScale`~~ -> `sh_scale`                _Modifies the scaling factor of the strafe helper for better visibility._
- ~~`cl_strafeHelperY`~~ -> `sh_y`                        _Sets the vertical position of the strafe helper on the HUD._

### New Configuration Options

Expanded settings for deeper customization:

- **HUD Colors (RGBA)**:
  
  - `sh_color_accelerating`: Set the color for acceleration zones.
  - `sh_color_optimal`: Define the color for optimal strafe zones.
  - `sh_color_centermarker`: Customize the center marker color.
  - `sh_color_indicator`: Adjust the indicator color.
- **Indicator Settings**:
  
  - `sh_tolerance`: Adjust indicator tolerance (degrees) from optimal angle Default value is 0.20
  - `sh_indicator`: (0-1) -Enable or disable visual indicator. Default value is 0.
  - `sh_indicator_pos "x y"` Set indicator position (X, Y).
  - `sh_indicator_size "x y"` Adjust the size of indicator (X, Y).

---

## Installation

1. Extract all files into your Quake 2 directory.
2. Ensure the original Quake 2 game files are present for compatibility.
3. Use the `sh_*` settings to personalize the HUD and features to your preferences.

---

## Building

Follow the instructions in the q2pro INSTALL.md

It should be possible to compile a version for at least Windows, Linux, and macOS.

---

## License

Q2pro is licensed under the GPL-2.0 license.
Q2pro-jump is licensed under the GPL-2.0 license.
strafe_helper is licensed under the MPL-2.0 license.
Any modifications made to the files covered by those licenses follow their original licenses.

No warranty is provided and no additional rights are reserved for the changes introduced in this project.

---

