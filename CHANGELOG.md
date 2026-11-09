# Changelog

All notable changes to the core of Mirante are documented in this file. The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/), and the core follows [Semantic Versioning](https://semver.org/spec/v2.0.0.html). While the version is 0.x, a minor version may break apps and modules.

The changes to the module interface, and what a module must do about them, are in [src/module_api/CHANGELOG.md](src/module_api/CHANGELOG.md). A release is tagged when a merged pull request raises `VERSION`. See [decision 0134](docs/decisions/0134-release-tags-from-a-version-file.md).

## [Unreleased]

### Added

- A list filter can always list a fixed set of values, such as every state of a Status column, so a chosen state stays chosen while no row has it. See `ItemListView::setFilterValues()` in [the module interface changelog](src/module_api/CHANGELOG.md).

## [0.14.0] - 2026-11-09

### Changed

- A double click on an item cell's header opens the item dialog, as "Edit Item…" does, in every dashboard. In a task cell, only a double click on the task cell's own header opens the task dashboard; before, a double click anywhere in the cell did. See [decision 0151](docs/decisions/0151-double-click-in-task-cells.md).

### Fixed

- On macOS, Mirante hidden to the menu bar shows again on the current desktop (Space). Before, macOS switched back to the desktop the window was hidden from.

## [0.13.0] - 2026-11-05

### Changed

- **Collapse All Tasks** works in two steps: the first press closes every task, and a second press, while every task is closed, closes the projects too. **Expand All Tasks** is the reverse: it opens the projects first, then every task with its lists. The sidebar's header buttons do the same.
- On macOS, a left click on the menu bar icon shows or hides Mirante, as a click on the tray icon does on Windows and Linux. A right click or a Control-click opens the menu.

### Fixed

- The colour dot of the current root dashboard tab keeps its colour, with a ring around it so it shows on the highlight. Before, it turned white.
- In native lists, such as Jira issues or GitLab merge requests, a long title or summary no longer hides the other columns. The title's text is elided instead. Status pills no longer cut a word short on macOS, as in "Pass…" for "Passed".

## [0.12.0] - 2026-11-04

### Added

- **Duplicate item…** in the context menu of an item in the sidebar, and **Duplicate Item…** in the menu of an item cell, open the new item dialog filled with a copy of the item.
- Item cells have an **Open in browser** button in their header when the item has a page on the web. Item types give the address with `ItemContext::setWebLink()`. See [decision 0148](docs/decisions/0148-web-link-in-the-cell-header.md).
- A profile can hold several accounts of a module, such as a personal GitHub account and a bot, or github.com and a GitHub Enterprise server. Each has its own settings and token. **Add Account…** in a module's submenu of Profiles > <profile> > Modules adds one, and an item chooses its account in the item dialog. A pasted link is read in the account of its host. See [decision 0146](docs/decisions/0146-several-accounts-per-module.md).

### Changed

- The **Open in browser** button of a web page moved from its toolbar to the cell header, beside the address. While "Open page" shows a page, it opens that page.
- **Profiles > Manage Profiles…** is now the one place for profile settings: a tree of profiles, their modules and the modules' accounts, with the selected one's page beside it, each with Apply and Revert. Each profile in the Profiles menu opens it at that profile, and **Set Token…** opens it at the account. The profiles' Modules, Set as Default and Settings… submenus, and the Profile Settings window, are removed. See [decision 0147](docs/decisions/0147-profile-settings-in-one-window.md).
- Every build has **Help > Show Log** and keeps the log messages from the start. The `--log` option is removed. See [decision 0140](docs/decisions/0140-log-window-in-all-builds.md).
- **Add Items from Link…** and **New Task from Link…** take any web address. When no module recognises it, they offer one item of the type that shows it best, such as a Web page. See [decision 0141](docs/decisions/0141-one-menu-entry-for-links.md).

### Removed

- Release builds no longer have **Help > Open Sample Data**. The sample data still opens at start when there is no workspace.
- **Paste URL…** in the context menu of a task and of its Items row. Use **Add Items from Link…**, or the link field of **New item…** to choose another type.

### Fixed

- A module's section in the project dialog has a **Remove** button. Before, a section added with **Add settings** could not be taken away.
- Cell and page headers show their whole titles when there is room. Before, on macOS, a title could end in an ellipsis even though it fit.

## [0.8.6] - 2026-11-04

### Added

- A list in a module's settings, such as GitHub's repositories, can have an **Add from Server…** button. It lists what the server offers with the token in the dialog, saved or not, to check and add. See [modules](docs/ui.md#modules).
- A list tells its module when the user changes a filter, so the module can keep the choice for the next start.

## [0.8.5] - 2026-11-03

### Added

- A list's filter can start with chosen values, which a module takes from the item's settings. The user's own choice replaces them.

### Changed

- A new workspace starts with one group, "Default", so the first project has somewhere to go. See [menus](docs/ui.md#menus).
- A list's filters, such as Status in the GitHub, GitLab and Jira lists, can choose several values. Clicking a value checks or unchecks it and leaves the list open; clicking "All" clears them.

### Fixed

- The Windows setup's last page shows the texts of its two checkboxes again, "Run Mirante" and "Start Mirante when I sign in", in English or Portuguese. The sign-in checkbox now starts unticked on a first install, as [decision 0137](docs/decisions/0137-the-setup-does-not-start-mirante-at-sign-in-by-default.md) says.

## [0.8.4] - 2026-11-03

### Changed

- The reusable workflows run their actions on Node.js 24, so apps that build with them no longer get GitHub's warnings about Node.js 20. setup-uv no longer caches, which also ends its warning that no dependency files matched.

## [0.8.3] - 2026-11-03

### Fixed

- On Windows, the window no longer seems to close and open again when the first web view in it appears, such as on opening a tab with a web page. Windows draw through the graphics API from the start. See [web views](docs/web-views.md#the-window-and-its-first-web-view).

## [0.8.2] - 2026-11-03

### Changed

- Help > About lists each module by its name and the release it was built from, such as "Claude: 0.8.0", in place of its repository and commit. A module pinned to a commit shows the short commit, and a checkout on disk "local".
- The Windows setup leaves "Start Mirante when I sign in" unticked on a first install. An upgrade keeps the installed version's choice. See [decision 0137](docs/decisions/0137-the-setup-does-not-start-mirante-at-sign-in-by-default.md).

### Fixed

- The Windows setup's last page is in Portuguese when Windows is, as its other pages are.

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

[Unreleased]: https://github.com/rrgmc/mirante-core/compare/v0.14.0...HEAD
[0.14.0]: https://github.com/rrgmc/mirante-core/compare/v0.13.0...v0.14.0
[0.13.0]: https://github.com/rrgmc/mirante-core/compare/v0.12.0...v0.13.0
[0.12.0]: https://github.com/rrgmc/mirante-core/compare/v0.8.6...v0.12.0
[0.8.6]: https://github.com/rrgmc/mirante-core/compare/v0.8.5...v0.8.6
[0.8.5]: https://github.com/rrgmc/mirante-core/compare/v0.8.4...v0.8.5
[0.8.4]: https://github.com/rrgmc/mirante-core/compare/v0.8.3...v0.8.4
[0.8.3]: https://github.com/rrgmc/mirante-core/compare/v0.8.2...v0.8.3
[0.8.2]: https://github.com/rrgmc/mirante-core/compare/v0.8.1...v0.8.2
[0.8.1]: https://github.com/rrgmc/mirante-core/compare/v0.8.0...v0.8.1
[0.8.0]: https://github.com/rrgmc/mirante-core/tree/v0.8.0
