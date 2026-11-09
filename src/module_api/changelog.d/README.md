# Module interface changelog fragments

Each pull request that changes `src/module_api` adds one file here, instead of editing [CHANGELOG.md](../CHANGELOG.md). CI fails without it. Pull requests then never change the same lines, so they do not conflict. A release of the core moves the files to the top of the Unreleased list in `CHANGELOG.md` with `tools/release-changelog.py release X.Y.Z`, and deletes them. See [decision 0154](../../../docs/decisions/0154-changelog-fragments.md).

Name the file after the branch, such as `filter-fixed-values.md`. It holds entries only, without headings. An entry is a line that starts with `- `, with any further lines indented under it. Say what changed and what a module must do about it, or that modules need no change:

```markdown
- `ItemListView::setFilterValues(column, values)` gives values that a filter always lists first. Modules need no change.
```

`python3 tools/release-changelog.py check` checks every fragment. CI runs it on each pull request.
