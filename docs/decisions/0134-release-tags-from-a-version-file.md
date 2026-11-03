# 0134. Release tags from a version file, and apps pin tags

- Status: Accepted
- Date: 2026-11-03

## Context

Every manifest pinned the core and the modules by commit. The core and the repositories of modules had no tags, so the update job could only move a pin to the newest commit of a default branch, and an app took every change. The user asked that the official way to pin is a release tag, and that the main app pins tags.

## Decision

- **Each library repository holds its version in `VERSION`,** as `X.Y.Z`: `mirante-core`, `mirante-modules` and `mirante-modules-work`. With the user's choice, they all start at `0.8.0`, and each moves on by itself.
- **A merged version bump makes the tag.** The core publishes `tag-release.yml`. On a push to the default branch, it makes the annotated tag `vX.Y.Z` of `VERSION` on that commit, unless the tag exists. On a pull request it checks the file only. Each library repository calls it from a short workflow. Nobody pushes these tags by hand.
- **Apps and the template pin tags.** With the user's choice, `mirante-app`, and so `mirante` after part 9, and `mirante-app-template` pin the core and each repository of modules to `vX.Y.Z`. The update job moves a tag pin to the newest release tag ([decision 0127](0127-automatic-pin-updates.md)), so an app takes only releases.
- **Development pins stay commits.** The core's `ci/modules.json` and each repository of modules' own `ci/manifest.json` keep commit pins. An interface change is built across the repositories before anything is tagged, by the order in the custom builds plan.
- **A release is a reviewed pull request** that raises `VERSION`, with the changes since the last tag. A change to the module interface raises the minor version while the interface is 0.x.

## Alternatives

- **Tags pushed by hand.** They are easy to forget, and nothing checks that the version in the code matches the tag.
- **Tags in every manifest, also for development.** An interface change would need a tag of the core before the modules could build against it, and so a release for every step of the change.
- **The app's version for every repository.** The repositories release at different times, so the numbers would drift apart at the first release of one alone.

## Consequences

- A tag is made by the workflow's own token, so it starts no other workflow. Nothing in these repositories runs on a tag.
- A tag can still be moved or deleted by someone with write access. Rules that protect tags matching `v*` in each repository prevent it, and are a setting of each repository.
- An app that pins a commit keeps working, and the update job still moves it to the newest commit.
