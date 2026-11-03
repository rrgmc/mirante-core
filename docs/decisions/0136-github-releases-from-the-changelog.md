# 0136. GitHub releases from the changelog

- Status: Accepted
- Date: 2026-11-03

## Context

[Decision 0134](0134-release-tags-from-a-version-file.md) tags a release of each library repository from its `VERSION` file, with a `CHANGELOG.md` section for each version. A tag alone shows on GitHub only in the list of tags, without its notes. Apps published their releases with notes that GitHub generates from the merged pull requests. The user asked that every main repository publishes GitHub releases with its changelog, and that the libraries attach nothing beyond their sources.

## Decision

- **The tag workflow publishes the release.** `tag-release.yml` publishes a GitHub release for each tag it makes, titled `<repository> vX.Y.Z`. GitHub attaches the source archives by itself, so nothing else is uploaded.
- **The notes are the version's section of the changelog,** without the link lines at the end of the file. A link to a file of the repository points to the tag's tree, since a release page does not resolve relative links.
- **A tag without a release gets one** on the next push to the default branch, while `VERSION` still names it. The tags made before this decision get their releases that way, or by hand with the same notes.
- **An app's release takes its notes from its own `CHANGELOG.md`.** `release-app.yml` reads only that file of the app, and uses the version's section, or the section of `X.Y.Z` for a prerelease `vX.Y.Z-rc.N`. Without a changelog or a section, it falls back to GitHub's generated notes, as before.

## Alternatives

- **GitHub's generated notes everywhere.** They list pull requests, not what changed for the people who use a release, and repeat what the changelog already says.
- **A separate release workflow on the tag.** A tag made with the workflow's own token starts no other workflow, so it would never run.

## Consequences

- A changelog section is now also the public text of a release, so it is written for the people who read it.
- An edit to a section after its release does not change the published notes. `gh release edit` does that.
