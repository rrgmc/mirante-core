# 0133. The work modules move to a repository of their own

- Status: Accepted
- Date: 2026-11-03

## Context

[Decision 0116](0116-custom-builds-in-separate-repositories.md) put all nine first-party modules in `mirante-modules`. The user asked to move the modules for work, GitHub, GitLab, Jira and Claude, to a repository of their own, so that an app for personal use can leave them out by not listing the repository.

## Decision

- **`mirante-modules-work`** holds `claude`, `github`, `gitlab` and `jira`, split from `mirante-modules` with their history by `git filter-repo`.
- **`mirante-modules`** keeps `document`, `folder`, `image`, `notes` and `web`.
- **Both build alone,** each as an app of its own folders, with the same top `CMakeLists.txt`, presets, `ci/manifest.json` and workflow.
- **The core's CI builds both.** `ci/modules.json` pins the two repositories, and the `module-includes` job checks both.
- **No code changes,** since no module lists another in `DEPENDS`.

## Alternatives

- **Keep one repository, and make the work modules optional** in each app's manifest. Every app would still fetch them, and an update of a work module would move the pin of every module.
- **A repository per module.** Nine pins to move in every app, for modules that change together.

## Consequences

- An app that wants the work modules lists both repositories, each with its own pin.
- An interface change now touches two module repositories before the apps.
- The modules' READMEs still link to documents of `mirante`, as before the move.
