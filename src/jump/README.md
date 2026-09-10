# Strafe Helper

Code for drawing a [gazhud](https://www.q3df.org/wiki?p=133)-like HUD overlay
for id Tech 3 engine games, that shows optimal strafing angles. This was
initially written for the three **id Tech 3** games by Raven Software,
*Star Trek: Voyager - Elite Force*, *Star Wars Jedi Knight II: Jedi Outcast* and
*Star Wars Jedi Knight: Jedi Academy*, but it can work fine for other
**id Tech 3** games or even other **id Tech** engines too.

## Integration in q2pro_race

This directory is integrated directly into the engine. The upstream usage
instructions below describe the original reusable library; they are not the
setup procedure for this checkout. Do not add a submodule or create the old
`strafe_helper_includes.h` shim to build this repository.

| Responsibility | Current source |
| --- | --- |
| Cvar registration and defaults | `sh_init.c`, called by `CL_InitLocal` in `../client/main.c` |
| Local prediction ownership | `../client/predict.c` brackets the final local command with `StrafeHelper_BeginPrediction` / `StrafeHelper_EndPrediction` |
| Movement observations | `../common/pmove/template.c` publishes helper/efficiency inputs only during owned prediction |
| Helper state and calculations | `strafe_helper.c`, `strafe_helper.h` |
| Efficiency math and display | `strafe_efficiency.c`, `sh_efficiency_draw.c` |
| Drawing adaptation and screen composition | `strafe_helper_customization.c`, `../client/screen.c` |
| Network diagnostics | `sh_netmeter.c`, sampled from the client screen/network path |
| Jump command/menu handling | `sh_menus.c`, `sh_hud_menu.c`; native menu inputs are in `../client/ui/` |

The helper observes movement; preview values must not enter movement or
network history. Drawing reads HUD editor drafts only during its preview
pass. See the [developer guide](../../doc/development.md#architecture) for
the engine flow and the
[cvar reference](../../doc/custom-cvars.md) for current settings.

These sources are currently collected in `ui_src` in
[meson.build](../../meson.build). Keep `client-ui=true` for full client builds;
the efficiency calculation module's standalone test does not prove the
complete overlay is independent of UI/editor code.

## Tests

From the repository root, after [toolchain setup](../../INSTALL.md):

```sh
meson test -C builddir --suite jump-hud --print-errorlogs
```

The [coverage table](../../doc/development.md#standalone-jumphud-suite)
distinguishes math, production-code fixtures, renderer stubs, and editor
state. Fixture success does not replace an explicitly authorized in-game
rendering or input check.

## Historical upstream usage

Include this repository as
[submodule](https://git-scm.com/book/en/v2/Git-Tools-Submodules) in your project
and add `strafe_helper.c` to your build system. Then a header file called
`strafe_helper_includes.h` has to be created directly outside the submodule.
It needs to declare the functions `acosf`, `atan2f`, `snprintf`, `sqrtf` and
`truncf` from the C Standard Library. If the standard library can be used,
including `math.h` and `stdio.h` is enough. If not (for example for Quake
Virtual Machine code), equivalent implementations have to be given manually.

Additionally, the functions `shc_drawFilledRectangle` and `shc_drawString` have
to be implemented as desired, but according to their declarations in
`strafe_helper_customization.h`.

Then, call the functions `StrafeHelper_SetAccelerationValues` and
`StrafeHelper_Draw` where appropriate.

If drawing the current speed is not wanted, the declaration of `snprintf` and
the implementation of `shc_drawString` can be omitted. Instead, use the
preprocessor definition

```c
#define STRAFE_HELPER_CUSTOMIZATION_DISABLE_DRAW_SPEED
```

to disable that part of the strafe helper.
