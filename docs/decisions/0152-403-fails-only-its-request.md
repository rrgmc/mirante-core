# 0152. A 403 fails only its own request

- Status: Accepted
- Date: 2026-11-09

## Context

Since M5, a `401` on any request, or a `403` on a `GET`, marked the profile's server as failing. While a server is failing, its requests return "Token refused" without being sent, until its token or settings change. See the [M5 plan](../plans/m5-servers-and-status.md).

GitLab answers `403` when a valid token cannot see one resource, such as the pipelines of one project or the approvals of one merge request. One such answer paused the whole server. Lists that had already loaded kept their rows, but showed "Token refused", stopped polling, and lost their pipeline and approval columns. The token itself was fine.

## Decision

- Only a `401` marks the server as failing.
- A `403` on any request, `GET` included, fails only that request with `ApiReply::Error::Forbidden` and "Not allowed (403)". The server keeps working.
- A rate-limited `403`, as GitHub sends it, still waits and sends again, as in [decision 0102](0102-github-module.md).

## Alternatives

- **Pause only on a `403` that names the token's scope.** GitLab sends `WWW-Authenticate: Bearer error="insufficient_scope"` when the token lacks a scope. Other servers say it differently or not at all, and a missing scope still affects only the requests that need it.
- **Keep the rule and log the URL that was refused.** This explains the pause, but it does not stop one hidden resource from pausing everything.

## Consequences

- A token that cannot see one project no longer stops the other items of the profile.
- A token that is valid but has too few scopes for every read no longer shows "Token refused" or a line in the warning bar. Each item shows "Not allowed (403)" as an error instead.
