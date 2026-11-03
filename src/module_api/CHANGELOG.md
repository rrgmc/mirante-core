# Module interface changelog

What changed in the module interface, newest first, and what a module must do about it. CI fails if a pull request changes `src/module_api` without changing this file. See [decision 0119](../../docs/decisions/0119-module-interface-versions-and-checks.md).

While the version is 0.x, any minor version may break modules, so a module must state the exact version. From 1.0, a minor version only adds things, and only a major version may break them.

## Unreleased

Changes that do not affect modules are listed here too.

## 0.1

The first version with a number.

- `mirante_add_module()` needs `API 0.1`. Without it, configuration stops.
- `<mirante/ModuleApiVersion.h>` gives the version as C++ macros.
- `mirante_add_module()` builds modules with `-Werror=suggest-override` on GCC and Clang. A method that overrides without saying `override` stops the build.
- `QT_LIBS` may name only Qt modules that the core uses or that the manifest's `qt` list names.
