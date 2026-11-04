# 0138. Settings lists can be filled from the module's server

- Status: Accepted
- Date: 2026-11-04

## Context

The GitHub and GitLab settings screens keep a list of repositories or project paths, typed one per line. The token in the same screen can already list them, so typing them is slow and invites typos.

The screen's form is built by the core from the module's fields. A module's own editor cannot reach the server either: an editor may write a token but never read it ([decision 0122](0122-custom-configuration-screens.md)), and `EditorContext` has no `ApiClient`.

## Decision

The core offers it for every module. A `SettingsScreen` names the `TextList` fields its server can fill with `serverChoiceFields()`, and answers for one with `listServerChoices()`, which gets an `ApiClient` and returns `ServerChoice` values or a message.

In the form the core builds, each such field gets an **Add from Server…** button. It opens a dialog that lists the values, with a filter and a check box for each. The values already in the field are checked and fixed, and **Add** appends the checked ones.

The `ApiClient` comes from `ServerClients::clientFor()`. It uses the values and the token in the open dialog, saved or not, so the list works when the token was just typed. It is a client of its own: it keeps no ETags with the profile's client, and a refused token does not mark the profile's server as failing.

## Alternatives

- **A screen editor of each module's own.** GitHub and GitLab would each build the button and the dialog, and modules share no code, so it would be written twice. The core would still have to give the editor a way to reach the server, so it saves no core change.
- **Only the saved values and token.** Simpler, since `ServerClients::client()` already exists. But the first setup is the moment the list helps most, and the token is not saved until OK.
- **The button for `Text` fields too, picking one value.** No module needs it yet.

## Consequences

- A module adds a server-filled list with two short methods and no UI code.
- The button shows only in forms the core builds. A screen with its own editor gets none.
- A token typed into the dialog is sent to the server before OK, but only to the origin of the base URL in the dialog, as for every request ([decision 0123](0123-tokens-stay-on-their-origin.md)).
