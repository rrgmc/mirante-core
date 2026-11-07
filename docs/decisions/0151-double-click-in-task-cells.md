# 0151. A double click in a task cell acts on what was clicked

- Status: Accepted
- Date: 2026-11-07

## Context

[Decision 0020](0020-edit-task-dashboards-alone.md) made a double click anywhere in a task cell open its task dashboard on its own, for editing. The task cell shows item cells, each with a header that holds the item's title. A double click on an item's title therefore opened the task dashboard, while the user expected it to open the item.

## Decision

- A double click on the task cell's header, which holds the task's title and the dashboard's name, opens the task dashboard on its own, as before.
- A double click on the header of an item cell inside the task cell opens the item dialog for that item, as "Edit item…" does.
- A double click anywhere else in the task cell, such as on an item's body, does nothing.
- The "Edit task dashboard" button and menu entry are unchanged.
- This replaces the double click point of decision 0020. The rest of 0020 stands.

## Alternatives

- **Open the item alone in the preview tab.** The user asked for the item to open for editing, and the item dialog is where an item is edited.
- **Keep the double click on the body opening the task dashboard.** A double click on an item's body is too close to the item to read as a request for the whole task dashboard.

## Consequences

- An item shown inside a task cell can be edited without opening its task dashboard first.
- Opening a task dashboard with a double click needs a click on its header, not anywhere in the cell.
