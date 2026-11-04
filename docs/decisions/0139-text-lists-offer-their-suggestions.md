# 0139. Text lists offer their suggestions in a checklist

- Status: Accepted
- Date: 2026-11-04

## Context

The GitHub and GitLab items over several repositories or projects take them as a text list, one per line. The profile's settings already keep a list of repositories or project paths, and the single-repository field offers it as `suggestions` in a drop-down list. The text list ignores `suggestions`, so the user types each name again.

The settings dialog already has a checklist for **Add from Server…** ([decision 0138](0138-add-list-values-from-the-server.md)), but it lives in the app, which modules and `FieldForm` cannot use.

## Decision

A `TextList` field with `suggestions` gets an **Add from List…** button under it, in every form `FieldForm` builds. It opens a checklist of the suggestions, with a filter, in which the entries already in the field are checked and fixed. **Add** appends the checked ones.

The checklist moves into the module widgets as `mirante::widgets::ChoiceListDialog`. The settings dialog's `ServerChoiceDialog` becomes a subclass that adds "Loading…", the server's error and the client it keeps.

## Alternatives

- **A drop-down menu of the suggestions.** It adds one entry per click, and a long list has no filter.
- **A field editor of each module's own.** GitHub and GitLab would each build the same button and dialog, since modules share no code.

## Consequences

- A module offers a list for a text list by filling `suggestions`, as it does for a text field. GitHub and GitLab fill it from the profile's repositories and project paths.
- A field without suggestions looks as before.
- A form whose field has a custom editor ([decision 0122](0122-custom-configuration-screens.md)) gets no button.
