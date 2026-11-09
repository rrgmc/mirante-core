# 0153. A double click on an item's title opens the item alone

- Status: Accepted
- Date: 2026-11-09

## Context

[Decision 0151](0151-double-click-in-task-cells.md) made a double click on the header of an item cell open the item dialog. The user meant it to open the item alone, as the "Open Alone" button does. 0151 had rejected that option because it read the request as one for editing.

## Decision

- A double click on the header of an item cell opens the item alone in the preview slot, as "Open Alone" does. This holds in every dashboard, as in 0151.
- It does not open the item dialog. "Edit Item…" in the cell menu still does.
- This replaces the item point of decision 0151. The rest of 0151 stands: a double click on a task cell's header opens its task dashboard alone, and a double click elsewhere in a task cell does nothing.

## Alternatives

- **Keep the item dialog.** The user asked for the item to open alone.

## Consequences

- A double click on an item's title acts like a double click on the item in the sidebar, which also opens it alone.
- Editing an item from its cell needs the cell menu.
