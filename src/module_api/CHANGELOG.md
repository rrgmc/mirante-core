# Module interface changelog

What changed in the module interface, newest first, and what a module must do about it. CI fails if a pull request changes `src/module_api` without changing this file. See [decision 0119](../../docs/decisions/0119-module-interface-versions-and-checks.md).

While the version is 0.x, any minor version may break modules, so a module must state the exact version. From 1.0, a minor version only adds things, and only a major version may break them.

## Unreleased

Changes that do not affect modules are listed here too.

- `mirante_add_module()` takes `OPTIONS NAME=default...`, build options that the manifest's folder entry can set. Each is the variable and the compile definition `MIRANTE_OPTION_<NAME>`. A folder entry can also make a module optional. See decision 0129.
- `mirante_add_module()` takes `EXTENSIONS point=files...`, extension points with their default sources. An app's extension, named in the folder entry's `extensions`, is compiled in place of a point's defaults. See decision 0132.
- A module's own web pages, in its `RESOURCES` under `web/`, are served as `module://<module id>/<path>`, with an origin per module. `createWebView()` refuses `qrc:` addresses, which share one origin. See decision 0128.
- A module with a folder `translations/` carries its own texts, as `<id>_<language>.ts`. The core loads them for the enabled modules. A module without them keeps working, in English.

## 0.1

The first version with a number.

- `mirante_add_module()` needs `API 0.1`. Without it, configuration stops.
- `<mirante/ModuleApiVersion.h>` gives the version as C++ macros.
- `mirante_add_module()` builds modules with `-Werror=suggest-override` on GCC and Clang. A method that overrides without saying `override` stops the build.
- `QT_LIBS` may name only Qt modules that the core uses or that the manifest's `qt` list names.
