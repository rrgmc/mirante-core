# 0140. The log window in every build, the sample data menu in debug builds only

- Status: Accepted
- Date: 2026-11-04

## Context

Help > Show Log lists the app's Qt log messages. Debug builds always kept them; release builds only when started with `--log`. A user who meets a problem in a release build has to restart the app with an option they do not know, and the messages from before the restart are lost.

Help > Open Sample Data opens the read-only sample data. It is useful while developing, but in a release build it leaves the user's workspace for data they cannot keep, with no clear reason to do so.

## Decision

Every build keeps the log messages from the start and has Help > Show Log (`Ctrl+Shift+L`). The `--log` option, and `-Log` in `tools/run.ps1`, are removed.

Only debug builds have Help > Open Sample Data. Every build still opens the sample data at start when there is no workspace to open.

## Alternatives

- **Keep `--log` and turn it on by default.** An option that cannot be turned off has no use.
- **Hide Open Sample Data behind a setting.** It adds a setting for a command that only development needs.

## Consequences

- Release builds keep the latest log messages in memory, within the buffer's limit.
- A release build has no command to go back to the sample data once a workspace is open.
