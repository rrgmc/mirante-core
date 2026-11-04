# Mirante documentation

Mirante is a personal desktop dashboard for all of one person's work, built with Qt 6. Start with the [overview](overview.md).

## Documents

| Document | Contents |
|---|---|
| [Overview](overview.md) | Vision, platforms, scope and non-goals |
| [Data model](data-model.md) | Entities, fields, priorities, profiles, servers, layout trees |
| [User interface](ui.md) | Navigation, dashboards, layout editing, Focus, keyboard, tray |
| [Item types](item-types.md) | Every type of panel: configuration, display, actions |
| [Modules](modules.md) | How item types are grouped, built, enabled and contributed |
| [Writing a module](writing-a-module.md) | A guide for authors outside this project, from an empty repository to an app that lists the module |
| [Integrations](integrations.md) | GitLab, GitHub, Jira, Claude, polling, terminal launch |
| [Web views](web-views.md) | Login profiles, live copies, memory, built-in panels |
| [Sync](sync.md) | Data folder, conflict copies, device settings, secrets |
| [Architecture](architecture.md) | Modules, classes, data flow, build, tests |
| [Roadmap](roadmap.md) | Milestones with acceptance criteria |
| [Releasing](releasing.md) | Version tags, the release workflow and its signing secrets |
| [Plans](plans/) | Detailed code plans, one per milestone, written just before it starts |
| [Decisions](decisions/README.md) | Records of design decisions and their reasons |
| [Research](research/) | Studies of possible features, with what was tested. Nothing in them is decided |

## Translations

The app speaks English and Brazilian Portuguese. Every text the user sees is written in English in the source and translated in `translations/mirante_pt_BR.ts` in the same change; `task check` fails on a text without a translation. The documents, the log and ids stay in English. See [translations/README.md](../translations/README.md) for how to add a text, the Portuguese word for each glossary term, and [decision 0086](decisions/0086-english-and-brazilian-portuguese.md).

## Glossary

**Group.** The top level, such as "Personal" or "Work". A group holds projects and chooses a default [profile](data-model.md#profile). It has a colour, which tints the title bars of its task dashboards.

**Project.** A unit of work inside a group, usually one repository. A project has a main folder, optional extra folders and a priority.

**Folder.** A directory that belongs to a project, usually a Git repository. Synced data stores its name and Git remote URL. Each device maps it to a local path.

**Task.** A piece of work inside a project. A task has items, dashboards, a state (active, done or archived) and a priority.

**Item.** One named panel in a task, such as one merge request or one Jira list. A task can have several items of the same type.

**Item type.** What kind of panel an item is, such as "GitLab MR (summary)" or "Jira List". See [item types](item-types.md).

**Module.** A folder of item types and the code they share, such as settings screens and API clients. Each module can be included or left out when building. See [modules](modules.md).

**Folder entry.** One item of a repository's `folders` list in the manifest: a folder name, or an object with the folder and its settings. See [decision 0129](decisions/0129-folder-entries-options-and-optional-modules.md).

**Module option.** A named build-time value that a module declares and the manifest sets, such as `MAX_ITEMS`.

**Optional module.** A module whose folder entry says `"optional": true`. It is listed in the manifest but built only when a build option turns it on.

**Local module.** A module in a folder of the app repository itself, listed with `"local"` in place of a repository and a pin. See [decision 0130](decisions/0130-local-modules-in-the-app-repository.md).

**Edition.** The name of one app repository's build, such as `work`, set in its manifest. It appears in package names and in the About dialog. Every edition is still the same app. See [decision 0131](decisions/0131-editions.md).

**Extension point.** A named place where a module lets an app replace part of its code, such as the layout of a view, through a C++ interface of the module. See [decision 0132](decisions/0132-extension-points.md).

**Extension.** The code that an app gives for an extension point: a folder that the manifest names, compiled into the module in place of the point's defaults.

**Settings screen.** A configuration screen of a whole module, such as GitLab's "General Configuration", opened from a profile's Modules submenu in the Profiles menu. Its values are the module's settings. Each profile has its own, kept in the workspace and synced. See [settings screens](modules.md#settings-screens).


**Placement.** One place where an item is shown: a cell, or the preview slot. Each visible placement has its own view, unless the item type allows only one.

**Dashboard.** A named arrangement of items or tasks, stored as a layout tree.
- A **task dashboard** belongs to a task and shows its items.
- A **root dashboard** is a tab in the main window and shows tasks and single items from anywhere.

**Layout tree.** The structure of a dashboard, made of splits, tabs and cells nested in any combination. See [data model](data-model.md#layout-tree).

**Split.** A layout node that places its children side by side or stacked, with sizes the user can drag.

**Tabs.** A layout node that shows one of its children at a time, chosen by a tab bar.

**Cell.** A layout node that shows one item or one task, or nothing (an empty cell).

**Preview slot.** A fixed place right of the tabs that shows the last task dashboard or item opened, for example from the sidebar. The next one replaces it. It never becomes a tab. Older documents call it the preview tab. See decisions [0048](decisions/0048-focus-and-preview-slots.md) and [0052](decisions/0052-no-pinning.md).

**Focus slot.** A fixed place right of the tabs, before the preview slot, that shows the Focus view. It is always there and cannot be closed.

**Focus view.** A list of active tasks from all projects, sorted by priority.

**Priority.** Urgent, High, Normal or Low, set on projects and tasks.

**Attention.** A live reason to act now, such as a failed pipeline. Shown as a badge. It is not a priority.

**Watched item.** An item whose attention counts on the tray icon and may send desktop notifications. Items are not watched until the user chooses.

**Account.** One set of a module's server settings in a profile, with its own token, such as a personal GitHub account and a bot. Each profile has a default account per module, and may have others; an item chooses one. See [accounts](data-model.md#accounts) and [decision 0144](decisions/0144-several-accounts-per-module.md).

**Server.** The GitLab or GitHub server, or the Jira site, that a profile talks to: a base URL in the module's settings, and a token kept in the keychain. Each [account](data-model.md#accounts) of a profile has one per module. Earlier versions called these connections. See [servers and tokens](data-model.md#servers-and-tokens).

**Profile.** A named set of what belongs to one account or context, such as "Work" or "Personal": web logins (cookies and site data) and module settings, such as the server of each module, with its token. The workspace holds a list of them, always at least one, and one of them is the default profile. Each group chooses a profile, and an item can choose its own; those that choose none use the default. Items in one profile share all of it. Logins and tokens stay on each device; the rest is synced. See decisions [0050](decisions/0050-profiles-hold-logins-connections-and-settings.md), [0054](decisions/0054-module-settings-replace-connections.md) and [0057](decisions/0057-any-profile-can-be-the-default.md).

**Status cache.** The part of the app that polls GitLab, GitHub and Jira, keeps the results and computes attention.

**Data folder.** The folder holding the synced JSON files.

**Web data folder.** The folder on this device that holds the profiles' web logins: the app's local data folder plus `/profiles`, or the folder given with `--web-data`. It is never synced, and only one Mirante process uses it at a time. See [decision 0058](decisions/0058-one-instance-per-web-data-folder.md).

**Workspace.** A data folder with a `workspace.mirante` file at its top. File > Open Workspace opens one, File > New Workspace makes one, and the app remembers the recent ones on each device. A workspace has a name, kept in its `workspace.mirante` and shown above the sidebar tree.
