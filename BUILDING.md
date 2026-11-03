# Building Mirante

Mirante is in early development. You need CMake 3.21 or newer, Ninja, Python 3, and [uv](https://docs.astral.sh/uv/), which the setup scripts use to download Qt 6.8. Besides Qt itself, they download Qt WebEngine, `qttools` for `lupdate` and `lrelease`, Qt's translations, and `qtimageformats` for WebP images. A Qt downloaded before these were added is completed on the next build.

With [Task](https://taskfile.dev) installed, the common commands are the same on every platform:

```sh
task                         # lists the tasks
task build                   # builds the dev preset
task test PRESET=release     # builds and tests another preset
task check                   # the translation check, then tests with all modules and with none
task translations            # updates translations/ after a text changed; see translations/README.md
task run -- -Data <folder>   # starts the build, on Windows
```

Without it, on Windows, with Visual Studio 2022 and its C++ tools installed:

```powershell
tools/build.ps1                  # downloads Qt if needed, builds and tests the dev preset
tools/build.ps1 -Preset release
tools/run.ps1                    # starts the dev build; -Data <folder> opens that workspace, -Build builds first
tools/run.ps1 -Data <folder> -Settings <file.ini>   # a test run that leaves this device's settings alone
```

On macOS and Linux:

```sh
QT_DIR=$(tools/setup-qt.sh)
cmake --preset dev -DCMAKE_PREFIX_PATH="$QT_DIR"
cmake --build --preset dev
ctest --preset dev
```

Qt goes into `.qt/` in the repository. To share one Qt between projects, set the environment variable `MIRANTE_QT_ROOT` to a folder of your choice first.

On macOS, the build signs the app with a code signing certificate, so that macOS stops asking for keychain access after each build. It uses the maintainer's development certificate, named in `CMakeLists.txt`, when the keychain has it. Otherwise, set `MIRANTE_MACOS_SIGN_IDENTITY` to the name of one of your certificates before the first `cmake --preset`. `security find-identity -v -p codesigning` lists them. Without a certificate the app is signed ad hoc, and macOS asks again after each build. See decisions [0090](docs/decisions/0090-sign-the-macos-bundle.md) and [0091](docs/decisions/0091-default-macos-sign-identity.md).

Each build also makes a `bin` folder in its build folder, for example `build/release/bin`. It holds a copy of the app with the Qt libraries, plugins and Qt WebEngine files it needs, so it starts without Qt on `PATH`. On macOS the folder holds the app bundle. On Linux it holds only the executable, which finds Qt on the machine that built it.

To make the package of a build, install it and run CPack in its build folder. The install folder must be an absolute path. On Windows, `tools/build.ps1 -Preset release -Package` does all of it. NSIS must be installed.

```sh
cmake --install build/release --component Mirante --prefix "$PWD/build/release/install"
python3 tools/check-package.py build/release/install
cd build/release && cpack
```

[docs/releasing.md](docs/releasing.md) says how to publish a release.

## Building with modules

The modules are in other repositories. A manifest names the modules to build, and the core fetches them when it configures, so the first configure needs the network. See [decision 0125](docs/decisions/0125-how-modules-are-fetched.md). The `dev`, `release` and `ci` presets use `ci/modules.json`, which pins `mirante-modules` to a commit. The `no-modules` and `ci-no-modules` presets use no manifest. Another manifest is chosen like this:

```sh
cmake --preset dev -DMIRANTE_MANIFEST=<path to mirante.json>
```

To use a checkout on disk in place of a pinned repository, without changing the manifest, name it with its `name` from the manifest in capitals:

```sh
cmake --preset dev -DFETCHCONTENT_SOURCE_DIR_MIRANTE_MODULES=<folder>
```

An app repository does the same for the core, with `FETCHCONTENT_SOURCE_DIR_MIRANTE_CORE`. On Windows, `tools/build.ps1 -Source <folder>` builds the presets of an app repository.

`.github/workflows/build-app.yml` is the workflow that an app repository calls to build, test and package itself. Its only other requirement is a preset named `ci`. `release-app.yml` publishes what it built, and `update-app.yml` opens a pull request that moves the pins to the newest versions, with `tools/update-pins.py`. See [decision 0127](docs/decisions/0127-automatic-pin-updates.md).

An interface change that breaks the modules takes these steps: a branch in `mirante-modules` with the fix, the core's pull request with `ci/modules.json` pinned to that branch, a new pin of the core in `mirante-modules`, and the merged commit in `ci/modules.json`.

## Compiling faster

If [ccache](https://ccache.dev) is installed, the build uses it, so worktrees and presets share what they have compiled. Each still has its own build folder. To share between worktrees at different paths, tell it where they live once, with the parent folder of all of them:

```sh
ccache --set-config base_dir=<parent folder>
```
