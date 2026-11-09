### Changed

- Pull requests add their changelog entries as fragments in `changelog.d/` and `src/module_api/changelog.d/`, and a release moves them into the changelogs with `tools/release-changelog.py`. CI runs for GitHub's merge queue too. See [decision 0154](docs/decisions/0154-changelog-fragments.md).
