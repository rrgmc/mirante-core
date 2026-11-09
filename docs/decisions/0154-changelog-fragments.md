# 0154. Changelog fragments, and a merge queue

- Status: Accepted
- Date: 2026-11-09

## Context

[Decision 0134](0134-release-tags-from-a-version-file.md) keeps the changes of each library repository under Unreleased in `CHANGELOG.md`. [Decision 0119](0119-module-interface-versions-and-checks.md) makes CI require an entry in `src/module_api/CHANGELOG.md` for each interface change. Several sessions work in parallel, and each pull request adds its entry at the top of the same list. When one pull request merges, the others conflict on those lines, and each must merge `master` and run CI again.

Branch protection also requires a branch to be up to date with `master` before it merges. So every merge sends every other open pull request back through "update branch" and a new CI run, even when nothing conflicts.

## Decision

- **A pull request adds its changelog entries as a file of its own,** a fragment, and does not edit the changelogs. Fragments for `CHANGELOG.md` go in `changelog.d/`, and those for `src/module_api/CHANGELOG.md` in `src/module_api/changelog.d/`. Each is named after its branch. A fragment of `changelog.d/` holds headings of Keep a Changelog, such as `### Fixed`, with their entries. A fragment of the interface holds entries only. Each folder's `README.md` says how to write one.
- **A release moves the fragments into the changelogs.** `tools/release-changelog.py release X.Y.Z` writes `VERSION`. It makes the section `## [X.Y.Z]` of `CHANGELOG.md` from the entries under Unreleased and the fragments, grouped by heading, and adds its link. It puts the interface's fragments at the top of the Unreleased list of `src/module_api/CHANGELOG.md`. Then it deletes the fragments. The pull request that raises `VERSION` runs it, so `tag-release.yml` still finds the section it requires.
- **CI checks the fragments.** `tools/release-changelog.py check` runs on each pull request and fails on a fragment that is not well formed. A pull request that changes `src/module_api` must add a fragment to `src/module_api/changelog.d/`. The changelog and its fragments do not count as a change of the interface, so a release needs no fragment.
- **CI runs for the merge queue.** `ci.yml` and `release-tag.yml` also run on `merge_group`, so that the user can turn on GitHub's merge queue for `master`. The repository belongs to an organization, which allows it. The queue tests each pull request on top of the latest `master` and then merges it, so branches need not be brought up to date by hand. Turning the queue on, and turning off "Require branches to be up to date before merging", are settings of the repository's rules.

## Alternatives

- **A union merge driver** (`CHANGELOG.md merge=union` in `.gitattributes`). It is one line, and Git then keeps both sides' lines. GitHub ignores merge drivers, so its pages still show a conflict and its "Update branch" button still fails. Two pull requests that each add a heading, such as `### Fixed`, give that heading twice.
- **Changelogs written at release time** from the titles or descriptions of pull requests. The entries would be written long after the change, by someone else. CI could no longer check that an interface change says what a module must do.
- **An existing tool,** such as towncrier or changesets. Each brings a dependency and its own format, for a task that a short script does in the format these changelogs already use.

## Consequences

- Pull requests no longer conflict on the changelogs. They can still conflict on code.
- An entry is not in `CHANGELOG.md` until the release. What is unreleased is the entries under Unreleased and the files in `changelog.d/`.
- The repositories of modules keep editing their changelogs until they adopt fragments too, each by a pull request of its own.
- Until the merge queue is on, branches must still be brought up to date before they merge.
