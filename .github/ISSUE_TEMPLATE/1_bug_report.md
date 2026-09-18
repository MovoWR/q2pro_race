---
name: 'Report a bug'
about: 'Create a bug report'
title: ''
labels: ''
assignees: ''

---

### Before reporting bugs

Report the exact `q2pro_race` build that reproduces the bug. For local builds,
include the source revision and whether the checkout has local changes. For
prebuilt clients or Starter installations, identify the downloaded artifact.
If you also tested another version, state its result separately.

### Important information

Provide the following information:

- `q2pro_race` version and artifact/source identity
- OS version
- GPU driver and version

For Linux:

- Linux distribution and version
- Window manager version

### Reproduction steps

Steps to reproduce the behavior.

### Expected behavior

A clear and concise description of what you expected to happen.

### Actual behavior

A clear and concise description of what actually happened.

### Screenshots

If reporting graphics glitches, provide screenshot or video.

### Log file

Provide the relevant log from `q2pro_race +set developer 1 +set logfile 1`
(`q2pro_race.exe` on Windows), launched from the normal game-data directory.

### Crash reports

If `q2pro_race` crashes, provide a crash report (Windows, when crash-dump support
is enabled) or a backtrace (Linux). On Linux, start
`gdb --args ./q2pro_race [client arguments]`, type `run`, then `bt` after the crash.

### Compilation issues

If reporting a building / compilation issue, provide `meson setup` command
line and full console output.
