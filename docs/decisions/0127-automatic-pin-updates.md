# 0127. Automatic pin updates, and a release for each merged update

- Status: Accepted
- Date: 2026-11-03

## Context

An app repository pins the core and each repository of modules ([decision 0125](0125-how-modules-are-fetched.md)). The core's pin is also in the app's workflows, because GitHub needs a literal ref after `uses:`. The roadmap asks that a custom build keeps up with new versions without its owner doing the work by hand, and that a failed update does not take the app away.

## Decision

**A scheduled job opens a pull request.** The core publishes `update-app.yml`, which an app calls from a scheduled workflow. It runs `tools/update-pins.py` from the core at the current pin. The script finds what is newer than each pin:

- A pin on a release tag, `vX.Y.Z`, moves to the newest release tag.
- A pin on a commit moves to the newest commit of the repository's default branch.
- Any other pin, such as a branch or a prerelease tag, stays.

It writes the new pins into `mirante.json` and the core's into the workflows, raises the patch number of `MIRANTE_VERSION`, and writes a description with a link to each difference. The pull request reuses one branch, `mirante-update`, so a later run updates it instead of opening another.

**The app passes a token of its own.** A pull request opened with the workflow's `GITHUB_TOKEN` starts no CI, and that token cannot change workflow files. The app passes a fine-grained personal access token, or a GitHub App's token, as the secret `MIRANTE_UPDATE_TOKEN`. Without it, the job warns and does nothing.

**Merging publishes a release.** With `release-new-version`, `build-app.yml` builds a push to the default branch as the release `vX.Y.Z` of `MIRANTE_VERSION`, unless that tag exists. `release-app.yml` then makes the tag on the built commit and publishes. A merged update raised the version, so it is released. Any other push that leaves the version alone publishes nothing.

**A failed update changes nothing.** The pull request's CI fails, and the last release stays in place until someone fixes or closes it.

## Alternatives

- **Dependabot or Renovate.** Neither knows the manifest's format. Renovate's regex managers could read it, but each app would need its own configuration, and the core's pin in the workflows must change in the same commit as the manifest's.
- **Merge the update automatically when CI passes.** A new core can change what the app does without breaking its build. The owner reads the differences first.
- **Release from a tag only, as before.** The owner would have to push a tag after each merge, which is the work this decision removes. A pushed tag still publishes.

## Consequences

- Each app owner creates a token once, and renews it when it expires. The template's README says how.
- An app repository has two workflows: one that builds, tests and releases, and one that updates.
- `update-pins.py` comes from the core at the pin, so a fix to it reaches an app only with the next update.
- A pin on a commit follows the default branch, so an app that pins commits takes every change. Pinning release tags takes only releases.
