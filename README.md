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

- `cl_drawStrafeHelper` -> `sh_draw`
- `cl_strafeHelperHeight` -> `sh_height`
- `cl_strafeHelperCenter` -> `sh_center`
- `cl_strafeHelperCenterMarker` -> `sh_centermarker`
- `cl_strafeHelperScale` -> `sh_scale`
- `cl_strafeHelperY` -> `sh_y`

### New Configuration Options

Expanded settings for deeper customization:

- **HUD Colors (RGBA)**:
  - `sh_color_accelerating`: Set the color for acceleration zones.
  - `sh_color_optimal`: Define the color for optimal strafe zones.
  - `sh_color_centermarker`: Customize the center marker color.
  - `sh_color_indicator`: Adjust the indicator color.
    
- **Indicator Settings**:
  - `sh_tolerance`: Adjust indicator tolerance (degrees) from optimal angle.
  - `sh_indicator`: Enable or disable visual indicator.
  - `sh_indicator_pos`: Set indicator position (X, Y).
  - `sh_indicator_size`: Adjust the size of indicator (X, Y).

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

