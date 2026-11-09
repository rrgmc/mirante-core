# Changelog fragments

Each pull request that changes what users of the core see adds one file here, instead of editing [CHANGELOG.md](../CHANGELOG.md). Pull requests then never change the same lines, so they do not conflict. A release moves the files into `CHANGELOG.md` with `tools/release-changelog.py release X.Y.Z` and deletes them. See [decision 0154](../docs/decisions/0154-changelog-fragments.md).

Name the file after the branch, such as `filter-fixed-values.md`. It holds one or more headings of [Keep a Changelog](https://keepachangelog.com/en/1.1.0/), each with its entries:

```markdown
### Fixed

- A list's Status filter keeps a chosen status while no row has it.
```

The headings are `### Added`, `### Changed`, `### Deprecated`, `### Removed`, `### Fixed` and `### Security`. An entry is a line that starts with `- `, with any further lines indented under it. Write it as in `CHANGELOG.md`: for the people who use a release. Links are relative to the repository's root, as they are in `CHANGELOG.md`.

A change to the module interface also needs a fragment in [src/module_api/changelog.d](../src/module_api/changelog.d/README.md).

`python3 tools/release-changelog.py check` checks every fragment. CI runs it on each pull request.
