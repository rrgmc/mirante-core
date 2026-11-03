# Code plan: the manifest before the switch

The user asked for six changes before part 9 of [the custom builds plan](custom-builds.md) replaces `mirante`. Five of them extend the manifest and the module build. The sixth moves four modules to a repository of their own. This plan comes before part 9 and does not change it.

| Part | Change | Repositories |
|---|---|---|
| 1 | [Folder entries: options and optional modules](#part-1-folder-entries) | `mirante-core` |
| 2 | [Local modules in the app repository](#part-2-local-modules) | `mirante-core` |
| 3 | [Editions](#part-3-editions) | `mirante-core` |
| 4 | [Extension points](#part-4-extension-points) | `mirante-core` |
| 5 | [The work modules move](#part-5-the-work-modules-move) | new `mirante-modules-work`, `mirante-modules`, `mirante-core` |
| 6 | [The apps follow](#part-6-the-apps-follow) | `mirante-app`, `mirante-app-template`, `mirante-modules-samples` |

Parts 1 to 4 each go in a pull request of their own, in this order, because each one changes the manifest's code. Part 5 can run beside them. Part 6 comes last.

## Terms

These join the glossary in [docs/README.md](../README.md):

- **Folder entry.** One item of a repository's `folders` list in the manifest: a folder name, or an object with the folder and its settings.
- **Optional module.** A module whose folder entry says `"optional": true`. It is listed but not built unless a build option turns it on.
- **Module option.** A named build-time value that a module declares and the manifest sets, such as `MAX_ITEMS`.
- **Extension point.** A named place where a module lets an app replace part of its code, such as the layout of a view, through a C++ interface of the module.
- **Extension.** The code that an app gives for an extension point: a folder, compiled into the module.
- **Edition.** The name of one app repository's build, such as `work`. It appears in package names and in the About dialog. Every edition is still the same app.

## The manifest

All new fields are optional, so every manifest of today stays valid.

```json
{
  "edition": "work",
  "qt": ["Pdf", "PdfWidgets"],
  "core": { "repo": "https://github.com/rrgmc/mirante-core", "ref": "<commit>" },
  "modules": [
    {
      "name": "mirante_modules",
      "repo": "https://github.com/rrgmc/mirante-modules",
      "ref": "<commit>",
      "folders": [
        "web",
        "notes",
        { "folder": "document", "optional": true }
      ]
    },
    {
      "name": "mirante_modules_work",
      "repo": "https://github.com/rrgmc/mirante-modules-work",
      "ref": "<commit>",
      "folders": [
        {
          "folder": "github",
          "options": { "MAX_ITEMS": "50" },
          "extensions": {
            "runlayout": { "local": "extensions/github-run-layout" }
          }
        }
      ]
    },
    {
      "name": "app_modules",
      "local": "modules",
      "folders": ["acmetickets"]
    }
  ]
}
```

The core still stops configuration on an unknown field, at every level.

## Part 1: Folder entries

**Options and optional modules,** with the user's choices: options are build options, and an optional module is turned on by a build option.

- A folder entry is a string, as today, or an object with `folder` and any of `optional`, `options` and `extensions`. Part 4 adds `extensions`.
- **`optional: true`.** The core adds the folder, so the module's option `MIRANTE_MODULE_<ID>` exists, but with the default off. `-DMIRANTE_MODULE_<ID>=ON` builds it. A listed folder without `optional` stays on, as today.
- **`options`.** An object of names and string values. A module declares the options it takes in `mirante_add_module()`, with a default for each: `OPTIONS MAX_ITEMS=25 COMPACT=OFF`. The manifest's value replaces the default. An option the module does not declare stops configuration, with a message that names the module and lists what it takes. A name has capital letters, digits and underscores.
- **What the module gets.** Each option becomes the CMake variable `MIRANTE_OPTION_<NAME>` in the module's folder, before `mirante_add_module()` returns, and the compile definition `MIRANTE_OPTION_<NAME>="<value>"` on the module's target and its tests. A module's own CMake code can read the variable after the call.
- **The workflow.** `build-app.yml` gets an input `enable-modules`, a list of module ids separated by spaces, which becomes `-DMIRANTE_MODULE_<ID>=ON` for each. An app can then build with and without an optional module from the same manifest.
- **The About dialog** lists only built modules, as today.
- **Tests:** `tests/cmake/manifest/` gets cases for an optional module off and on, an option's default and its value from the manifest, an undeclared option, and a bad option name.

Decision 0129 records this.

## Part 2: Local modules

An app repository can hold modules of its own.

- **`local`** in place of `repo` and `ref`: a folder relative to the app repository's top, such as `"local": "modules"`, not to the manifest's folder, since a manifest can be in `ci/`. `"."` is the app repository's top. `local` with `repo` or `ref` stops configuration.
- The core adds the listed folders from there, with no FetchContent. A path that leaves the app repository, with `..` or an absolute path, stops configuration.
- **The About dialog** shows the app repository and its commit for a local module, read with `git rev-parse` as for fetched ones. Without Git, it shows "local".
- **Checks.** `build-app.yml` runs the include check on the app repository's local folders, as `check-modules` does for a modules repository, so a local module cannot include core headers either.
- **The template** gains an empty `modules/` folder with a README that shows how to add one, in part 6.
- **Tests:** a local folder, a local folder with `repo`, and a path that leaves the repository.

Decision 0130 records this.

## Part 3: Editions

With the user's choice: an edition is a name, and every edition is the same app, as [decision 0118](../decisions/0118-all-builds-are-one-app.md) says.

- **`edition`** at the top of the manifest. Lowercase letters, digits and `-`, from 1 to 32 characters. Without it, the build has no edition.
- **Package names.** With an edition, `mirante-<edition>-<version>-win64.exe`, `mirante-<edition>-<version>-macos.dmg` and `mirante-<edition>_<version>_amd64.deb`. Without one, the names stay as today. `MiranteInstall.cmake` sets `CPACK_PACKAGE_FILE_NAME` and `CPACK_DEBIAN_FILE_NAME` from it. The macOS signing job reads the disk image's name from a file the build writes, `build/ci/package-name.txt`, in place of building it from the version.
- **What does not change.** The app's name, the bundle identifier, the Debian package name `mirante`, the install folders, the settings and the keychain entries. Installing one edition replaces another.
- **The About dialog** shows the edition after the version, such as "0.1.0 (work)". `MIRANTE_EDITION` is a compile definition of the app.
- **The release** is titled "Mirante <edition> vX.Y.Z" when there is an edition.
- **Tests:** a bad edition name stops configuration. A test of the About dialog checks the text. The package name is checked by `tools/check-package.py`, which takes the expected name.

Decision 0131 records this.

## Part 4: Extension points

With the user's choice: an extension is a folder that the manifest names, compiled into the module.

- **A module declares an extension point** in `mirante_add_module()`: `EXTENSIONS runlayout=RunLayoutDefault.cpp`, with several default files separated by commas. This replaced the form `EXTENSION runlayout DEFAULT ...` when part 4 was built, because CMake cannot repeat a keyword with its own arguments. The name has lowercase letters and digits. The default is the sources that implement it when the app gives none.
- **The interface** is the module's own: a header in the module's folder that declares what an extension implements, such as a class `RunLayout` and a function `std::unique_ptr<RunLayout> makeRunLayout()`. The default sources define that function, and so does an extension. The module's README documents each extension point. The core does not know the interface.
- **The manifest** gives an extension in the folder entry: `"extensions": { "runlayout": { "local": "<folder>" } }`, or with `name`, `repo`, `ref` and `folder` to fetch it from another repository. The `name` lets a checkout on disk stand in, as for repositories of modules. An extension point the module does not declare stops configuration.
- **The build.** The core compiles the extension folder's `.cpp` and `.h` files into the module's target in place of the default sources. They see the module's folder on the include path, the module interface and Qt, and nothing else. An extension that needs resources lists them in a `resources.txt` in its folder, one path per line.
- **Checks.** The include check runs on extension folders. `-Werror=suggest-override` applies, as to modules.
- **The About dialog** shows each extension under its module, with its source and commit.
- **Tests:** a module with an extension point built with its default and with an extension, an undeclared extension point, and an extension that includes a core header.

The first user is a sample in `mirante-modules-samples`, `rrgmcextension`, whose view's layout comes from an extension point, with a second layout in the samples repository.

Decision 0132 records this.

## Part 5: The work modules move

`github`, `gitlab`, `jira` and `claude` move to a new public repository, `mirante-modules-work`. None of the nine modules lists another in `DEPENDS`, so no code changes.

1. Make `mirante-modules-work` with `git filter-repo`, keeping the four folders' history, from a clone of `mirante-modules`. Creating it needs the user's go-ahead.
2. Give it the same top `CMakeLists.txt`, presets, `ci/manifest.json`, workflow, `CLAUDE.md`, licence and README as `mirante-modules`, with its own name, `mirante_modules_work`, and branch protection.
3. A pull request in `mirante-modules` removes the four folders, and lists the five that stay in its manifest and README.
4. A pull request in `mirante-core` lists both repositories in `ci/modules.json`, and in the `module-includes` job. The core's CI then still builds all nine.

Decision 0133 records the split, which [decision 0116](../decisions/0116-custom-builds-in-separate-repositories.md) did not foresee.

## Part 6: The apps follow

- **`mirante-app`** lists both repositories, and gets the edition-free names of today, since it stands for `mirante`.
- **`mirante-app-template`** lists both repositories, with the work modules optional, and gains `modules/` for local modules and an example of an edition in its README.
- **`mirante-modules-samples`** gets `rrgmcextension` from part 4.
- The trial app `mirante-app-trial` is left as it is.

## What is not checked

- macOS: the package name and the signing job's change are checked by `mirante-app`'s CI, which runs on macOS.
- An extension from another repository is tested with local folders through `FETCHCONTENT_SOURCE_DIR_<NAME>`, as fetched modules are today, not over the network.
