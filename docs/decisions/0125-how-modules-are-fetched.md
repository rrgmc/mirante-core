# 0125. How the core fetches modules, and the fields of the manifest

- Status: Accepted
- Date: 2026-11-02

## Context

[Decision 0117](0117-app-repository-and-json-manifest.md) has the app fetch the core, and a function from the core fetch the modules. It leaves the exact fields of the manifest to the code plan. CMake's scopes decide where each step must run.

## Decision

**The manifest.** It is `mirante.json`, and the core stops configuration on an unknown field, so a misspelt field cannot build the wrong thing without saying so.

```json
{
  "qt": ["Pdf", "PdfWidgets"],
  "core": { "repo": "https://github.com/rrgmc/mirante-core", "ref": "v0.7.0" },
  "modules": [
    {
      "name": "mirante_modules",
      "repo": "https://github.com/rrgmc/mirante-modules",
      "ref": "v0.7.0",
      "folders": ["gitlab", "web"]
    }
  ]
}
```

- `name` is the name given to FetchContent, and so the name in `FETCHCONTENT_SOURCE_DIR_<NAME>`. It is written out because two owners can have repositories with the same name. It has lowercase letters, digits and underscores only.
- `folders` are folders of that repository, and `"."` is its top. A listed folder must add at least one module.

**What runs where:**

- **The app calls `enable_testing()`,** because `ctest` in the top build folder finds only tests below a top folder that called it.
- **The app's `project()` has `LANGUAGES NONE`,** so the core's `project()` is the first with a language, and the policies the core sets before it still apply.
- **The core adds the modules from inside its own directory scope,** with `mirante_add_manifest_modules()`. The targets from `find_package(Qt6)` are visible only from the core's folder down, and so are the settings from `qt_standard_project_setup()`.
- **A module repository's top `CMakeLists.txt` does not run in an app.** It is for that repository's own CI. Each repository is declared with a `SOURCE_SUBDIR` that does not exist, so FetchContent adds nothing, and the core adds each listed folder.
- **A listed folder is built, whatever its `DEFAULT` says.** Being listed is the choice. `MIRANTE_MODULES` still narrows the list.
- **The core finds its files through `MIRANTE_CORE_DIR`,** because its functions also run in a module's scope, where `PROJECT_SOURCE_DIR` can be the module's. The bin folder and CPack use `CMAKE_BINARY_DIR`, the top build folder.

## Alternatives

- **Add the modules from the app's scope.** A module would not find `Qt6::Core`. CMake 3.24's `CMAKE_FIND_PACKAGE_TARGETS_GLOBAL` would solve that for targets but not for variables such as `CMAKE_AUTOMOC`, and it would raise the minimum version.
- **`FetchContent_Populate()` instead of a `SOURCE_SUBDIR` that does not exist.** It is the direct way to fetch without adding, but its single-argument form is deprecated from CMake 3.30.
- **A list of modules, each with its repository, as the roadmap first said.** The same repository and pin would be repeated on each line.

## Consequences

- An app's `CMakeLists.txt` does not change between core versions unless these rules change. A small project in `tests/cmake/manifest/` tests them.
- A module cannot be added from anywhere but the core's scope, so an app cannot add a module by a line in its own `CMakeLists.txt`.
- A module repository needs a top `CMakeLists.txt` only for its own CI, and only a repository of one module, listed as `"."`, has its module there.
