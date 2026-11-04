# 0141. One menu entry adds items from a link

- Status: Accepted
- Date: 2026-11-04

## Context

The context menu of a task and of its Items row had two entries for a link. "Paste URL…" ([decision 0049](0049-items-from-a-pasted-link.md)) opened the item dialog with the clipboard's link, and made one item of any type that shows it. "Add Items from Link…" ([decision 0083](0083-quick-add-into-a-task.md)) opened quick add, which only the modules' quick add handlers read, such as Jira's and GitLab's. The user found the two entries confusing, because both seem to do the same thing.

## Decision

- **"Paste URL…" is removed** from both menus. "Add Items from Link…" is the one entry for a link.
- **Quick add takes any web address.** When no quick add handler takes the text, and the text is an `http` or `https` address, `QuickAddRun` proposes one item. Its type is the one the item dialog would select first for the link: a module's type before a generic one, such as Web page. Its configuration is the type's defaults with what the type made of the link. Its name is the type's name for the link, else the type's name. The task title is that name, else the link.
- **The proposal counts as the module's** of the item's type, so "Read as" and project matching treat it like a handler's. It has no project match, so a new task asks for its project.
- This applies to "New Task from Link…" too: a plain link now makes a task with one item, where before the dialog said "Mirante does not recognise this text."
- "New item…" keeps its link field, for choosing another type that takes the link.

This supersedes point 4 of decision 0049.

## Alternatives

- **Keep both entries and rename them** to say the difference. The user would still choose between two link entries in every use.
- **Remove "Paste URL…" only.** A plain link would then need "New item…" and its link field, and "Add Items from Link…" would still refuse it.
- **Offer one item per type that takes the link.** A link that a module's type and Web page both take would offer two items of the same thing.

## Consequences

- One entry for a link, which takes every web address.
- Choosing a type other than the best match needs "New item…" and its link field.
- `EntityActions::pasteUrl()` and `clipboardUrl()` are gone.
