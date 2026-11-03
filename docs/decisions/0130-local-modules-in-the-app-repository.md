# 0130. Local modules in the app repository

- Status: Accepted
- Date: 2026-11-03

## Context

Every repository in a manifest is fetched from its `repo` at its `ref` ([decision 0125](0125-how-modules-are-fetched.md)). The user asked that an app repository can hold modules of its own, beside its manifest, so that a module used by one app needs no repository of its own.

## Decision

- **`local` in place of `repo` and `ref`.** A repository entry with `"local": "modules"` names a folder of the app repository. Its `folders` are added from there, with no fetch. `local` with `repo` or `ref` stops configuration.
- **The path is relative to the app repository's top,** the top source folder, not to the manifest's folder. A manifest can be in a subfolder, such as `ci/manifest.json`, and `"."` should still mean the app repository. A path that is absolute, uses `\`, leaves the app repository or is not a folder stops configuration.
- **The About dialog** shows the app repository's `origin` and its commit for a local module. Without Git, it shows "local" for both.
- **The include check** runs on every local module folder in `build-app.yml`, on Linux, whether or not the app asks for `check-modules`. An app's own modules follow the same rules as everyone's.

```json
{ "name": "app_modules", "local": "modules", "folders": ["acmetickets"] }
```

## Alternatives

- **`"repo": "."`.** It would reuse the field for something that is not a repository address, and `ref` would mean nothing.
- **A path relative to the manifest.** `ci/manifest.json` in a modules repository would then need `".."`, which the rule against leaving the repository refuses.
- **Add the folders from the app's own `CMakeLists.txt`.** A module must be added from the core's scope ([decision 0125](0125-how-modules-are-fetched.md)), so the app cannot add it itself.

## Consequences

- A local module moves with the app's commits. The update job does not touch it.
- A local module's binary folder is `modules/<name>/<folder>`, as for a fetched one.
- A module can start local and later move to a repository of its own, by changing its entry and nothing in its code.
