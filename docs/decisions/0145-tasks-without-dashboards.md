# 0145. A task may have no dashboards

- Status: Accepted
- Date: 2026-11-04

## Context

A new task got one dashboard, "Main", with an empty cell. Many tasks are only a list of items in the sidebar, so the user deleted that dashboard or left it empty. The last dashboard of a task could not be deleted.

## Decision

- A new task has no dashboards. "New dashboard…" in the task's menu adds one, and the first becomes the default.
- Any dashboard can be deleted, the last one too. With none left, the task's `defaultDashboardId` is empty.
- Quick add still gives a new task one dashboard, "Main", that holds its new items, since [decision 0078](0078-quick-add-dialog.md) makes them show at once. A new task from quick add without items gets none.
- A task with no dashboards opens nothing when it is clicked. Its items still show in the sidebar and open alone. A root dashboard cell that shows such a task shows an empty layout.

## Alternatives

- **A setting for the default.** One more setting for a choice the user made once.
- **Keep the empty dashboard, but hide it.** The data would still hold dashboards the user never wanted.

## Consequences

- Code that reads a task's dashboards must allow none. The main window, task cells and the layout editor already did.
- Old tasks keep their dashboards.
