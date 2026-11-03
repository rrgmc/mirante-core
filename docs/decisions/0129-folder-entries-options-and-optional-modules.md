# 0129. Folder entries with build options, and optional modules

- Status: Accepted
- Date: 2026-11-03

## Context

A manifest lists each repository's folders by name ([decision 0125](0125-how-modules-are-fetched.md)), and every listed folder is built. The user asked for settings per folder, and for modules that are listed but built only when asked for, so that one app repository can make builds with and without them.

## Decision

**A folder entry is a name or an object.** The object has `folder` and any of `optional` and `options`. The core stops configuration on any other field, as it does elsewhere in the manifest.

```json
"folders": [
  "web",
  { "folder": "document", "optional": true },
  { "folder": "github", "options": { "MAX_ITEMS": "50" } }
]
```

**Options are build options.** With the user's choice, a folder's settings are values that the module reads when it is configured, not settings at run time.

- A module declares its options with defaults in `mirante_add_module()`: `OPTIONS MAX_ITEMS=25 COMPACT=OFF`. A name has capital letters, digits and underscores.
- The manifest's values are texts, and replace the defaults. An option the module does not declare stops configuration with a message that lists what the module takes.
- Each option is the variable `MIRANTE_OPTION_<NAME>` in the module's folder, and the compile definition `MIRANTE_OPTION_<NAME>="<value>"` on the module and its tests.

**An optional module is off unless the build turns it on.** With the user's choice, `"optional": true` makes the module's option `MIRANTE_MODULE_<ID>` default to off, and `-DMIRANTE_MODULE_<ID>=ON` builds it. `build-app.yml` takes the ids in its input `enable-modules`, and `tools/build.ps1` in `-EnableModules`. A listed folder without `optional` is built, as before.

## Alternatives

- **Default settings at run time** in place of build options. A module would need a new way to read them, through the interface, and they would mix with the values the user sets in the workspace.
- **`"enabled": false` in the manifest** for an optional module. Building with and without it would need two manifests, or an edit of the manifest in CI.
- **Option values of any JSON type.** CMake reads JSON numbers and booleans as text anyway, so texts keep one rule.

## Consequences

- Every manifest of before stays valid.
- A module's options are part of what its README documents, beside its item types and settings.
- A change of an option's value needs a new configuration. In an app repository, that is a commit to the manifest, which the update job does not touch.
