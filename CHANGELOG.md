# Changelog

All notable changes to the core of Mirante are documented in this file. The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/), and the core follows [Semantic Versioning](https://semver.org/spec/v2.0.0.html). While the version is 0.x, a minor version may break apps and modules.

The changes to the module interface, and what a module must do about them, are in [src/module_api/CHANGELOG.md](src/module_api/CHANGELOG.md). A release is tagged when a merged pull request raises `VERSION`. See [decision 0134](docs/decisions/0134-release-tags-from-a-version-file.md).

## [Unreleased]

## [0.8.1] - 2026-11-03

### Added

- `tag-release.yml` publishes a GitHub release for each tag it makes, with the version's section of the changelog as its notes, and gives a release to a tag that has none. See [decision 0136](docs/decisions/0136-github-releases-from-the-changelog.md).
- `release-app.yml` takes a release's notes from the version's section of the app's `CHANGELOG.md`, when it has one.

## [0.8.0] - 2026-11-03

The first release of the core on its own, after it was split from `mirante` for [custom builds](docs/plans/custom-builds.md).

### Added

- A manifest, `mirante.json`, from which the core fetches and builds the modules of an app repository. See [decision 0125](docs/decisions/0125-how-modules-are-fetched.md).
- Folder entries in the manifest, with build options for a module and optional modules. See [decision 0129](docs/decisions/0129-folder-entries-options-and-optional-modules.md).
- Local modules, in a folder of the app repository itself. See [decision 0130](docs/decisions/0130-local-modules-in-the-app-repository.md).
- Editions, which name an app repository's packages and show in the About dialog. See [decision 0131](docs/decisions/0131-editions.md).
- Extension points, where an app replaces part of a module's code at build time. See [decision 0132](docs/decisions/0132-extension-points.md).
- A version of the module interface, 0.1, which each module states, and a changelog of the interface. See [decision 0119](docs/decisions/0119-module-interface-versions-and-checks.md).
- Translations carried by each module, which the core loads for the enabled modules.
- The About dialog lists each module and extension with its repository and commit.
- A `module://` scheme, which gives each module's own web pages an origin of their own. See [decision 0128](docs/decisions/0128-module-pages-have-their-own-origin.md).
- Reusable workflows for app repositories: `build-app.yml` builds, tests and packages on Windows, macOS and Linux, `release-app.yml` publishes, and `update-app.yml` opens a pull request with newer pins. See decisions [0126](docs/decisions/0126-macos-signing-is-a-separate-job.md) and [0127](docs/decisions/0127-automatic-pin-updates.md).
- macOS signing and notarisation in a job of its own, which runs nothing from the build.
- `tools/scan-module-apis.py`, which reports the modules that use the keychain, the network, processes and web storage.
- `tag-release.yml`, which tags a release of a library repository from its `VERSION` file.
- [Writing a module](docs/writing-a-module.md), a guide for authors outside the project.

### Changed

- The first-party modules moved to [`mirante-modules`](https://github.com/rrgmc/mirante-modules) and [`mirante-modules-work`](https://github.com/rrgmc/mirante-modules-work). See decisions [0116](docs/decisions/0116-custom-builds-in-separate-repositories.md) and [0133](docs/decisions/0133-work-modules-in-their-own-repository.md).
- The core owns `main.cpp`, the resources and the packaging of every app. See [decision 0124](docs/decisions/0124-the-core-owns-the-app.md).
- Modules are built with `-Werror=suggest-override` on GCC and Clang.

### Removed

- `qrc:` addresses in item web views, which shared one origin among all pages.

[Unreleased]: https://github.com/rrgmc/mirante-core/compare/v0.8.1...HEAD
[0.8.1]: https://github.com/rrgmc/mirante-core/compare/v0.8.0...v0.8.1
[0.8.0]: https://github.com/rrgmc/mirante-core/tree/v0.8.0
