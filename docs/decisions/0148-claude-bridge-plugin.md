# 0148. A Claude Code plugin reports to the Claude agents list through a folder

- Status: Accepted
- Date: 2026-11-04

## Context

[Decision 0143](0143-claude-agents-list.md) gave the Claude agents list each session's title, state, folder and account, from `claude agents --json`. The user also wants each session's current tool, whether it waits for a permission, its model and effort, how full its context is, its cost, and the account's 5-hour and weekly rate-limit use. The command prints none of these.

Two documented features of Claude Code carry them:

- **Hooks** run a command on events such as a prompt, a tool call, a permission request, a notification and the end of a turn. Each gets the event as JSON, with the `session_id`. A Claude Code plugin can ship hooks, so one install covers every session of an account.
- **The status line** runs a command after each answer, with JSON that holds the model, the effort, the context use, the cost and, for claude.ai subscriptions, the rate limits. A plugin cannot set the status line. The user sets it once per account.

Decision 0143 rejected hooks because the app would need a listener.

## Decision

- **A Claude Code plugin, `mirante-bridge`, ships with the `claude` module** in its `plugin/` folder. Its hooks and a status line wrapper write files into an exchange folder. The app opens no port and runs no listener.
- **The exchange folder** is `MIRANTE_CLAUDE_DIR`, or `~/.mirante/claude/sessions` when that is not set. The plugin cannot read the app's settings, so the place is fixed. Each session has up to two files, named by its session id, each replaced whole on every write:
  - `<session>.hook.json`: the latest hook event, reduced to its name, the tool's name, the notification's type and the time. Prompt text and tool input are not written.
  - `<session>.status.json`: the latest status line input, as Claude Code gave it.
- **The module joins the files to the rows** of `claude agents --json` by session id. A row gets its tool, "Needs permission", its model, effort, context use and cost. Each account gets the rate limits of its newest status file. A file of a session that is not listed is ignored, and a file older than seven days is deleted.
- **The list refreshes when the folder changes,** at most once every five seconds, as well as on the usual poll.
- **Without the plugin the list works as before.** The plugin is optional, and the item has no setting for it.
- **The app reads only files that its own plugin writes.** It still reads none of Claude Code's stored data and calls no undocumented endpoint. Decisions 0008, 0084 and 0143 stand for everything else.

This supersedes the "hooks" and "status line" alternatives of decision 0143.

## Alternatives

- **Hooks that post to a listener in the core.** Changes would show sooner, but the core would need a local server and a port, and events sent while the app is closed would be lost.
- **One log file of every event.** It keeps the order of events, but it grows without end, and several sessions would write to it at once.
- **Reading Claude Code's transcripts for the same data.** Decisions 0008 and 0084 rejected reading Claude Code's files, and their format is not documented.

## Consequences

- The user installs the plugin once per account and points the status line at the wrapper once per account. The module's README says how.
- Live activity comes from the hooks, and usage from the status line. With only one set up, the list shows only its part.
- Rate limits show only for Pro, Max and Ultra subscriptions, and only after a session's first answer.
- The hooks run in a shell for every tool call. They run in the background, so they do not slow the session.
- If Claude Code changes the hook or status line input, the new fields stay empty until the module follows.
- Sessions in the Claude desktop app show the new fields only if it runs plugin hooks and the status line there.
