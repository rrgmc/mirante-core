# 0137. The Windows setup does not start Mirante at sign-in by default

- Status: Accepted
- Date: 2026-11-03

## Context

[Decision 0106](0106-start-at-sign-in.md) ticks "Start Mirante when I sign in" on the Windows setup's last page on a first install. The user asked that the setup leaves it unticked. The user also saw the setup's pages in Portuguese and its last two checkboxes in English, because the two texts were fixed English strings while NSIS translates its own pages into Windows' language.

## Decision

- **The sign-in checkbox starts unticked on a first install.** An upgrade keeps the installed version's choice, as before, so a Mirante that started at sign-in still does after an upgrade. A silent first install does not start at sign-in either.
- **The two checkbox texts follow the setup's language,** as its own pages do: Portuguese when Windows is in Portuguese, of Brazil or of Portugal, and English otherwise. They are ASCII, because CPack makes an ANSI setup.
- **Tools > Settings in the app** still turns starting at sign-in on and off, as decision 0106 says.

This supersedes the part of decision 0106 that ticks the checkbox on a first install.

## Alternatives

- **Keep the checkbox ticked.** Starting at sign-in without being asked surprises people, and is easy to turn on later in the app's settings.
- **A Unicode setup,** so the texts can use the app's exact Portuguese wording. CPack's template would need a line before its first command, and the script's encoding could not be tested without NSIS on this machine.

## Consequences

- A first install starts Mirante at sign-in only when the person ticks the box.
- The setup's last page is in the same language as its other pages, for English and Portuguese.
