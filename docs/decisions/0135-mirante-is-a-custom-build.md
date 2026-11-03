# 0135. Mirante is built from a manifest, like every app

- Status: Accepted
- Date: 2026-11-03

## Context

[Decision 0013](0013-compiled-in-modules.md) put each module in a folder `src/modules/<id>/` of the app's repository, chosen with the CMake option `MIRANTE_MODULE_<ID>`. [Decision 0116](0116-custom-builds-in-separate-repositories.md) split the core and the modules into repositories of their own, and tried the result in a sample app, `mirante-app`. The user set the conditions for replacing `mirante` in the roadmap, and approved the switch on 2026-11-03.

## Decision

- **`mirante` is an app repository,** with the files of `mirante-app`: a manifest, `mirante.json`, a short `CMakeLists.txt`, presets and workflows that call the core's. Its code, tests, resources, translations, tools and design documents left it, since they are in `mirante-core`, `mirante-modules` and `mirante-modules-work`. Its history keeps them.
- **It pins release tags** of the core and both repositories of modules, as [decision 0134](0134-release-tags-from-a-version-file.md) says, and lists all nine first-party modules.
- **It releases from a pushed tag `vX.Y.Z`,** as before. The build jobs hold no secrets, and a separate job signs and notarises the macOS bundle with the Developer ID, from the `release` environment, which admits only tags. See [decision 0126](0126-macos-signing-is-a-separate-job.md).
- **It keeps its own numbering.** The first release from the manifest is 0.8.0, after 0.5.0.
- **`mirante-app` is deleted,** at the user's choice, since `mirante` takes its place.
- **This supersedes the part of decision 0013** that places modules in `src/modules/<id>/` and chooses them only with CMake options. A module is a folder of a repository of modules, listed in a manifest ([decision 0125](0125-how-modules-are-fetched.md)). The rest of decision 0013 stands: modules are compiled in, use only the module interface, and register through generated code.

## Alternatives

- **Release on merge, as the template does.** The `release` environment would then have to admit `master`, so any code merged there could reach the signing job's secrets.
- **Keep `mirante-app` as a sample.** The template and the sample modules already show what a custom build needs.

## Consequences

- A change to the app is made in `mirante-core` or a repository of modules, released there, and reaches `mirante` when its pin moves.
- `master` in `mirante` requires the checks of the core's workflow, `build / build (windows-2022)` and `build / build (ubuntu-24.04)`.
- The weekly update job needs the secret `MIRANTE_UPDATE_TOKEN` in `mirante`. After merging an update, a pushed tag releases it.
