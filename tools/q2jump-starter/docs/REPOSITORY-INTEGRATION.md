# Starter repository integration

## Release workflow

`.github/workflows/build.yml` builds Starter on Windows after downloading and
hash-checking its pinned build inputs. The release job waits for Starter and
the existing client, macOS, Linux, and standalone test jobs. On pushes to
`dev` or `q2pro_race`, it uploads `q2pro_race_starter.exe` alongside the
existing Windows and macOS ZIP assets in the `latest` release. Pull requests
build and test Starter but do not publish a release. This describes the
configured workflow, not evidence that a remote CI run or public upload passed.

Starter downloads the published Windows x64 client ZIP at installation time.
Windows and macOS client packaging copies every file in the checked-in
`jump/pics/` directory, including the PCX images and the PNG backgrounds and
font. Starter installs this complete picture set from the Windows x64 ZIP.
When `latest` contains several revisioned x64 ZIPs, Starter chooses the matching
asset with the newest GitHub `created_at` timestamp, using the higher asset ID
to break a same-second tie. Revision counts can differ across publishing
branches, so they do not determine upload order. Older release assets are kept.
The chosen asset must pass the upload-state, URL, size, and digest checks; a
failure is reported without silently selecting an older package. Selection is
also deterministic when reconstructing retained installation/recovery receipts.
The wizard has separate new-game and existing-game choices. The latter reviews
client replacements before using the managed transaction and its backups;
neither choice changes player configuration or custom PAKs.
The client supplies first-launch Jump bindings when no saved player config or
custom defaults exist; Starter does not install or overwrite configuration files.
See `doc/client.asciidoc` in the engine source for the layout and startup order.
Existing-game folders may contain unrelated uninstallers. Legacy combined
Starter installations are identified by the `Q2JUMP.installation` marker.

The helper coordinates operations through an exclusively created
`.q2jump-operation.lock` file in the selected game folder. This requires write
access to that folder, including for readiness checks. Nested checks on the
same thread share the lock; other operations fail immediately. Windows removes
the temporary file when its final handle closes, including on process exit.
An existing file, directory or linked path at that name is rejected without
being changed. Do not remove an occupied lock path while setup is running.
Older helpers that used a global mutex do not share this lock, so do not run
different Starter versions concurrently against the same installation: their
operations can overlap and interfere with recovery metadata. Close older
Starter instances before using the updated helper.

Keep the `q2pro_race-windows-x64-r<revision>.zip` asset name and its supported
file layout when changing the engine release job. Starter checks the GitHub
asset digest, rejects unexpected files, and requires the client executable and
Jump resources. The ZIP and the original Quake II archives are not embedded
in `q2pro_race_starter.exe`.

## Installer build inputs

`packaging/starter-inputs.json` pins Inno Setup 6.7.3 and its compiler hash.
`docs/baseline-evidence/baseline-archives.json` pins the demo and 3.20 update
archive URLs, sizes, and SHA-256 hashes. The Python builder checks all selected
original-data files against `baseline-lock.json` before compiling setup.
Keep these downloads under `downloads/baseline/` and the verified compiler
under `tools/vendor/innosetup-6.7.3/`; those directories are excluded from Git.
CI waits for the compiler installer to finish and checks its process exit code
before verifying the pinned `ISCC.exe` checksum. A failed installation stops
the provisioning step before the Starter tests or build run.
The build also requires the Windows x64 .NET Framework C# compiler and
PowerShell. See `packaging/SOURCE.txt` for source and rebuild details.

From the repository root, use a fresh output directory for each build:

```powershell
python -m unittest discover -s tools/q2jump-starter/tests -p test_build.py -v
& ./tools/q2jump-starter/client-helper/build.ps1 -Output tools/q2jump-starter/build/client-helper-tests -Test
python tools/q2jump-starter/scripts/build.py --output tools/q2jump-starter/build/starter-next
```

The builder produces `Q2JUMP-Starter-<version>-setup.exe` and a corresponding
source ZIP under the selected output directory. CI publishes a byte-identical
copy of the setup file named `q2pro_race_starter.exe`. The source files used by
that build are kept in this repository; CI does not publish the generated
source ZIP as an additional release asset.

Offline tests and compilation do not establish native wizard behavior on a
clean Windows installation. Do not run the installer or game as part of the
repository's automated checks. Original-data permission review and clean
Windows acceptance remain separate release considerations.
