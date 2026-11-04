# 0143. A list of local Claude agents, from `claude agents --json`

- Status: Accepted
- Date: 2026-11-04

## Context

[Decision 0008](0008-claude-sessions.md) left Claude items without status, because no documented API lists sessions or their state. The user wants a list of Claude agents that shows only each one's title, status and repository, without any terminal or transcript text, and for several Claude accounts.

Claude Code now has a documented command for this: `claude agents --json` (see the [agent view](https://code.claude.com/docs/en/agent-view.md)). It prints the sessions running on this computer as a JSON array: those in a terminal, in the Claude desktop app and in the background. Each entry has a `name`, a `cwd`, a `kind`, a `startedAt` time, a `sessionId`, and either a `status` (`busy`, `waiting`, `idle`) or, for a background session, a `state` (`working`, `blocked`, `done`, `failed`, `stopped`). `--all` adds finished background sessions, and `--cwd <path>` keeps only sessions under a folder. The output holds no transcript text.

Claude Code keeps each account in a config folder, which `CLAUDE_CONFIG_DIR` chooses. The command lists only the sessions of the config folder it runs with.

Cloud sessions on claude.ai and Remote Control sessions on another computer still have no documented API.

## Decision

- **The `claude` module gets a list type, Claude agents (`claude.agents`).** Its status runs `claude agents --json` and holds the rows. Like other lists, it reports a summary, such as "3 agents · 1 needs input", and no attention. See [decision 0067](0067-lists-report-status.md).
- **The program is found, not configured.** The module looks for `claude` on the PATH, then in the folders where Claude Code's installers put it, such as `~/.local/bin`. The status job has no device settings, and an app started from a desktop does not always get the user's PATH.
- **Accounts are config folders.** The item lists config folders, one per account. The module runs the command once for each, with `CLAUDE_CONFIG_DIR` set, and labels each row with its folder's name. An empty list runs the command once, with the user's default.
- **The repository comes from the session's folder.** The module walks up from `cwd` to the folder that holds `.git`, follows a worktree's `.git` file to its main repository, and shows that repository's folder name, with the branch from `StatusJob::gitHead()`. It starts no `git` process.
- **The app still reads none of Claude Code's files** and calls no undocumented endpoint. Decisions 0008 and 0084 stand for everything else.

## Alternatives

- **Hooks that report to the app.** Claude Code's hooks can post each session's events, but the user would have to install them for each account, and the app would need a listener.
- **The status line.** It carries the repository and the session name, but not whether the session waits for input.
- **The undocumented endpoints of claude.ai.** Decision 0008 rejected them, and nothing has changed.
- **A device setting for the program's path.** The status job cannot read device settings, so it would need a change to the module interface.

## Consequences

- The list shows only sessions on this computer. A cloud session appears only when it was started from here and runs in the background here.
- An account whose config folder is on another computer, or missing, shows an error for that account. The other accounts still show their rows.
- If Claude Code changes the command's output, the list breaks until the module follows. The parser ignores fields it does not know.
- Running the command can create files in an empty config folder, as Claude Code does on first use.
