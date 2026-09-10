Prerequisites
-------------

Q2PRO can be built on Linux, BSD and similar platforms using a recent version
of GCC or Clang.

The build requires a C11 compiler, Python 3, Meson >= 0.59.0, and Ninja.
`meson.build` declares the language/version requirements; `version.py` uses
Python during configuration. CI installs the Python build tools with:

    pip3 install --no-input meson ninja

Run the recipes below from the repository root. See [Testing](#testing) for
standalone regression checks.

Q2PRO client requires either SDL2 or OpenAL for sound output. For video output,
native X11 and Wayland backends are available, as well as generic SDL2 backend.

Note that SDL2 is optional if using native X11 and Wayland backends and OpenAL,
which is preferred configuration.

Both client and dedicated server require zlib support for full compatibility at
network protocol level. The rest of dependencies are optional.

For JPEG support libjpeg-turbo is required, plain libjpeg will not work. Most
Linux distributions already provide libjpeg-turbo in place of libjpeg.

For playing back cinematics in Ogg Theora format and music in Ogg Vorbis format
FFmpeg libraries are required.

OpenAL sound backend requires OpenAL Soft development headers for compilation.
At runtime, OpenAL library from any vendor can be used (but OpenAL Soft is
strongly recommended).

To install the *full* set of dependencies for building Q2PRO on Debian or
Ubuntu use the following command:

    apt-get install meson gcc libc6-dev libsdl2-dev libopenal-dev \
                    libpng-dev libjpeg-dev zlib1g-dev mesa-common-dev \
                    libcurl4-gnutls-dev libx11-dev libxi-dev \
                    libwayland-dev wayland-protocols libdecor-0-dev \
                    libavcodec-dev libavformat-dev libavutil-dev \
                    libswresample-dev libswscale-dev

This fork currently builds the client and the bundled base game library.
The dedicated `q2proded` executable block is commented out in `meson.build`;
there is no enabled dedicated-server-only recipe in this checkout.

Users of other distributions should look for equivalent development packages
and install them.


Building
--------

Q2PRO uses Meson build system for its build process.

Setup build directory (arbitrary name can be used instead of `builddir`):

    meson setup builddir

Review and configure options:

    meson configure builddir

Q2PRO specific options are listed in `Project options` section. They are
defined in `meson_options.txt` file. Keep `client-ui=true` for full client
builds: jump runtime sources currently share the menu source gate.
The default game directory is `jump`, while the base game is `baseq2`.

Optional libraries are selected with Meson feature options and the wrap files
in `subprojects/`. Native Windows video links `opengl32`, sound/input uses
`winmm`, and sockets use `ws2_32`. The tracked
[Khronos archive](subprojects/packagefiles/khr-headers.tar.xz) supplies
`subprojects/khr-headers.wrap` locally; Meson extracts it into
`subprojects/khr-headers/`. Keep `subprojects/packagefiles/khr-headers.tar.xz`
in the source tree, including offline/CI copies. If it is missing, obtain that
file from the same source snapshot; the wrap has no download URL.

On Unix, absent OpenGL headers or all video backends can disable the client
target. Inspect the setup output; building only the game library or fixtures
is not evidence of a successful client build.

E.g. to install to different prefix:

    meson configure -Dprefix=/usr builddir

Finally, invoke build command:

    meson compile -C builddir

To enable verbose output during the build, use `meson compile -C builddir -v`.
The outputs are in the build directory. For the Windows MSVC configurations:

| Architecture | Client | Base game library |
| --- | --- | --- |
| x64 | `builddir/q2pro_race.exe` | `builddir/gamex86_64.dll` |
| x86 | `builddir/q2pro_race.exe` | `builddir/gamex86.dll` |

Other platforms use `q2pro_race` and a `game` library with the configured CPU
suffix and platform library extension.


Testing
-------

Run the standalone jump/HUD fixtures in a configured build directory:

    meson test -C builddir --suite jump-hud --print-errorlogs

Meson builds the required fixture targets before running them. The full suite
has 11 tests with `client-ui=true`; no game assets or live server are needed.
To run one fixture:

    meson test -C builddir --suite jump-hud hud-editor-state --print-errorlogs

The dependency-minimal Linux recipe is in [.github/workflows/build.yml](.github/workflows/build.yml).
For display tests, coverage, and validation limits, see
[Display settings](doc/display-settings.md#implementation-and-validation).

Leave `-Dtests=false` for normal builds. The `tests` option enables dangerous
built-in engine diagnostics and is not needed for the standalone suite.


Installation
------------

You need to have either full version of Quake 2 unpacked somewhere, or a demo.
Both should be patched to 3.20 point release.

Run `sudo ninja -C builddir install` to install Q2PRO system-wide into
configured prefix (`/usr/local` by default).

Copy `baseq2/pak*.pak` files and `baseq2/players` directory from unpacked
Quake 2 data into `/usr/local/share/q2pro_race/baseq2` to complete the
installation.

Alternatively, configure with `-Dsystem-wide=false` to build a ‘portable’
version that expects to be launched from the root of Quake 2 data tree (this
is default when building for Windows).

On Windows, Q2PRO automatically sets current directory to the directory Q2PRO
executable is in. On other platforms current directory must be set before
launching Q2PRO executable if portable version is built.

The engine and bundled base game library do not supply a complete game-data
installation. Joining an external Q2Jump server does not mean the bundled
`src/game/` library implements that server's mod.
For a portable Windows installation, the client belongs in the Quake II data
root beside `baseq2/`. If installing the locally built base game library, place
the architecture-matching DLL in `baseq2/`; do not substitute it for a Q2Jump
server's own mod. Preserve the existing installation when choosing an output
directory or copying files.


Music support
-------------

Q2PRO supports playback of background music ripped off original CD in Ogg
Vorbis format. Music files should be placed in `music` subdirectory of the game
directory in format `music/trackNN.ogg`, where `NN` corresponds to CD track
number. `NN` should be typically in range 02-11 (track 01 is data track on
original CD and should never be used).

Note that so-called ‘GOG’ naming convention where music tracks are named
‘Track01’ to ‘Track21’ is not supported.


MinGW-w64
---------

MinGW-w64 cross-compiler is available in recent versions of all major Linux
distributions.

Wrapped dependencies with download URLs can be downloaded and built by Meson
when downloads are allowed. The Khronos headers instead use the tracked local
archive described in [Building](#building); retain it in the source tree.

To install MinGW-w64 on Debian or Ubuntu, use the following command:

    apt-get install mingw-w64

It is recommended to also install nasm, which is needed to build libjpeg-turbo
with SIMD support:

    apt-get install nasm

Meson needs a cross-file describing your compiler, target machine, and
`pkg-config` search paths. This checkout has no `.ci` directory or bundled
MinGW cross-file, so the historical cross-file command is not a ready-to-run
recipe here. Supply and verify a file for your toolchain before configuring.
Current Windows CI uses MSVC instead; MinGW was not validated in this
documentation update.


Visual Studio
-------------

It is possible to build Q2PRO on Windows using Visual Studio 2022 and Meson.

Install Visual Studio with the MSVC C/C++ toolset and a Windows SDK. Install
Python 3 and the Meson/Ninja tools described under [Prerequisites](#prerequisites).
Make Python, Meson, and Ninja available on PATH in the developer shell.

Optionally, download and install nasm executable. The easiest way to add it
into PATH is to put it into `Program Files/Meson`.

The build needs to be launched from appropriate Visual Studio command line
shell: use `x64 Native Tools Command Prompt` for x64 or
`x86 Native Tools Command Prompt` for x86. Use a separate build directory per
architecture.

Change to Q2PRO source directory, then setup build directory:

    meson setup -Dwrap_mode=forcefallback builddir

This permits Meson to download/build available dependency fallbacks. It does
not install Visual Studio, the Windows SDK, or dependencies without a wrap.
CI additionally disables FFmpeg, SDL2, X11, and Wayland, and uses release
builds with `-Db_ndebug=true`; see `.github/workflows/build.yml` for the exact
matrix and feature flags.

Build:

    meson compile -C builddir

Run the regression suite:

    meson test -C builddir --suite jump-hud --print-errorlogs

The CI configuration defines Windows x64 and x86 builds. Its existence is
not a current test result or a guarantee that an old local executable matches
your working tree.
