# Display settings

The Windows WGL and EGL backends provide a native Display page at the existing
Video menu entry points. Other backends retain their existing display controls;
the Graphics, Brightness and Color, and Advanced Rendering groups are shared.

## Menu

The Display page contains Display mode, Monitor, Window size/Resolution, Refresh
rate, VSync, Graphics settings, Apply changes, and Back.

| Mode | Window | Resolution/size | Refresh rate |
| --- | --- | --- | --- |
| Windowed | Title bar, borders, resizing and normal maximize | Window client size in pixels; oversized windows fit the monitor work area | Desktop information |
| Borderless fullscreen | Fills the selected monitor | Current desktop resolution, read-only | Current desktop rate, read-only |
| Exclusive fullscreen | Uses a display mode on the selected monitor | Modes reported for that monitor | Rates reported for the chosen resolution |

The monitor selector shows the monitor name, a number, and a Primary label.
With one connected monitor it becomes read-only. Monitor choices refresh while
the page is open; window-size choices keep their order, including the original
window size. Desktop information describes the current desktop configuration,
not necessarily the panel's native resolution.

Graphics contains anti-aliasing, texture detail/filtering, anisotropic filtering
and FOV scaling. Brightness and Color contains the existing texture and lightmap
controls. Advanced Rendering contains the renderer, hardware gamma and the
existing advanced controls. Their cvars, ranges and per-control defer flags are
retained. Advanced Rendering follows its menu source's existing commit policy:
live controls in the built-in menu, changes on leaving in the external script.
Quick Settings and Jump physics FPS bindings remain at their previous entry points.

## Applying changes

Display controls edit a draft. Back, Escape or closing the page discards an
unapplied draft. Apply changes is disabled while the draft matches the current
configuration.

A change applied through the Display menu starts a 15-second Keep changes/Revert
confirmation. Keep commits the archived settings; the client's normal
configuration-saving path writes them to disk. Revert, Escape, the fullscreen
shortcut or timeout restores the previous configuration. VSync-only changes
commit without this confirmation. The timer uses the engine's real-time clock
and continues when the menu is closed or recreated by a renderer restart.

Trial settings do not change archived cvars. A configuration saved during the
trial therefore contains the confirmed configuration. The controller verifies
the reported mode after applying it and again before Keep. Exclusive dimensions
and refresh rate come from the current Windows display mode. Unreadable or
mismatched display state fails verification and is rolled back.
If the previous monitor is gone, recovery attempts a decorated
640 by 480 window on an available monitor.

Alt-Enter switches immediately without a confirmation or countdown. It remembers
the last accepted fullscreen type and resolution; a failed switch still restores
the previous configuration. If a Display-menu confirmation is already pending,
Alt-Enter cancels that trial. Normal window maximize remains a desktop action.

## Monitor and input behavior

Exclusive mode enumeration, switching and restoration use the selected Windows
display device. Leaving exclusive mode restores the desktop mode captured for
that device; it does not reset another monitor. The existing
`vid_flip_on_switch` setting continues to control desktop restoration on
focus loss.

Window placement supports monitors with negative coordinates and clamps the
frame to the selected work area. Window geometry is remembered independently
of fullscreen size. Borderless follows Windows monitor-move shortcuts and
adopts the destination desktop dimensions. A disconnected active monitor
causes recovery to a visible window.

On Windows versions exposing the DPI APIs, window frame sizing uses the window
DPI and corrects its size after a move between displays. Desktop DPI-change
messages apply their suggested rectangle for ordinary windows. Older Windows
versions retain the existing frame-sizing fallback.

Windows normally supplies the menu cursor outside exclusive fullscreen.
Exclusive fullscreen draws the configured game cursor without inherited menu
tint. Missing cursor images use a drawn cross. An explicit `cl_menu_cursor none`
still hides the game cursor during ordinary menus; a display confirmation
always permits a visible fallback cursor.
Backends without a cursor callback retain the existing fullscreen-only game
cursor policy, leaving ordinary windowed menus to the system cursor.

## Configuration compatibility

- `vid_fullscreen` remains a one-based index into `vid_modelist`; zero means windowed.
- `vid_noborder` remains available to existing configs and scripts. The new menu
  maps its three modes onto these existing controls.
- `vid_monitor` stores the Windows display device identifier. An empty value
  permits automatic monitor selection. Device identifiers come from Windows;
  they are not physical monitor serial numbers.
- `_vid_fullscreen_borderless` records the last confirmed fullscreen type for
  Alt-Enter. An empty value retains the initial exclusive-fullscreen preference.
- Confirming an exclusive selection preserves the selected mode in
  `vid_modelist` and archives that list together with its index.
- Explicit legacy refresh-rate and bit-depth mode syntax remains supported.
  Each whitespace-delimited entry keeps its one-based index, including malformed
  entries. Only the selected entry is parsed: a malformed selection uses the
  existing backend fallback, but does not block later valid modes. Saving a mode
  reuses the first exact matching entry or appends it; existing spelling, order,
  whitespace and duplicate entries remain unchanged.
- Selecting Windowed through the new menu clears `vid_noborder`,
  `win_notitle` and `win_noresize` to provide a draggable, resizable frame.

## Implementation and validation

`src/client/display.c` owns the trial and confirmed-state transition;
`src/client/ui/video.c` owns menu drafts and confirmation presentation.
`inc/client/display.h` describes the shared values. Optional callbacks in
`inc/client/video.h` connect supported backends. The Windows implementation
lives in `src/windows/client.c`.

From the repository root, run the offline suites in a build configured with `client-ui=true`:

```sh
meson test -C builddir --suite video --suite jump-hud --print-errorlogs
```

`display-settings` runs the production parser, controller and menu with engine
boundaries stubbed. Both video fixtures require `client-ui=true`;
`windows-display` is additionally registered only on Windows. It runs the
Windows backend against simulated display and window APIs. It does not create a
game window or change host display
settings. Cases cover selection, negative coordinates, desktop restoration,
readback failures, rollback, disconnection, DPI transitions, focus changes,
legacy bit depth, config preservation, fullscreen shortcuts, renderer restarts,
window-size cycling across monitor polls, saved-mode reapplication after malformed
legacy entries, unchanged configuration on Revert/timeout, and external resolution
changes at an unchanged refresh rate.

These fixtures do not verify GPU-driver presentation or visual appearance on
physical monitors. Manual game validation, when authorized, should cover all
three modes on one and two monitors, mixed DPI, Alt-Tab, Alt-Enter,
Win-Shift-Arrow, monitor disconnect/reconnect, the confirmation countdown,
and menu pointer visibility with present/missing cursor images.
