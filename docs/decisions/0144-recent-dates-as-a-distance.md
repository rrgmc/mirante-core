# 0144. Recent dates in lists read as a distance

- Status: Accepted
- Date: 2026-11-04

## Context

[Decision 0071](0071-lists-link-out-without-a-preview.md) made list dates compact, such as `10-30 14:32`. For a recent date, the user has to compare it with today to see how old it is. The user wants recent dates to say how long ago they were.

## Decision

- `compactDate()` gives a date up to 6 days away as a distance in one unit, rounded down: "just now" under a minute, then "10 minutes ago", "1 hour ago" or "3 days ago". A date to come reads "in 2 hours".
- A date 7 days away or more reads as before: `10-30 14:32` in this year, `2025-10-30` otherwise.
- Every list date goes through `compactDate()`, so the core and every module change together. The dates still sort by the real date.
- `relativeDate(when, now)` gives the distance alone, for tests and for modules.

## Alternatives

- **Two units, such as "1 hour 20 minutes ago".** More exact, but longer, and the user asked for one unit.
- **Qt's plural texts with `%n`.** The module widgets' tests run without translations, where they would read "1 minute(s) ago". Both languages have only a singular and a plural, so each unit has two texts instead.

## Consequences

- A distance is correct when the list is drawn. It is not redrawn on its own, so it can fall behind until the list's next fetch, which comes at the poll interval while the list is shown.
- A module test that compared a recent date with a fixed text must compare it with `compactDate()`.
