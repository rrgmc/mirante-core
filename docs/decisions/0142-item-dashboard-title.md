# 0142. Items have an optional title for dashboards

- Status: Accepted
- Date: 2026-11-04

## Context

An item's name is short when it is read in its task, such as "TODO" in the sidebar under a task. A root dashboard can hold items of many tasks, so several cells may all say "TODO". The place pill names the project and task, but it is grey, small and cut short first. The user wants a name that makes sense in the task, and another title that makes sense in a dashboard.

## Decision

- An item has an optional `dashboardTitle`. It is stored in the item's JSON only when it is not empty, and synced like the name.
- The item dialog has a **Dashboard title** row under the name. Its placeholder says "Same as the name". It is shown for every type, and for items of missing modules too, since it belongs to the item, not to its type.
- Dashboards show the dashboard title instead of the name when it is set: the cell header, the title of a new layout tab made around the item, and the task summary panel. `Item::dashboardName()` returns it, or else the name.
- Every other place keeps the name: the sidebar, the tabs and headers of item pages, the quick switcher, notifications and menus.

## Alternatives

- **Show the task title in the cell header.** The place pill already does this, and the user wants their own text.
- **A setting per cell, in the layout.** The same item in two dashboards would need the title twice, and the layout tree would hold text that is not about layout.
- **Use the dashboard title everywhere.** The sidebar already shows the task above the item, so a longer title would only repeat it.

## Consequences

- Items can read well in both the sidebar and a crowded root dashboard.
- A layout tab keeps the title it got when it was made. Changing the dashboard title later does not rename it, as with the name.
