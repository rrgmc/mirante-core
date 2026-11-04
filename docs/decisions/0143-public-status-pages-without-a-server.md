# 0143. Status of public status pages, fetched without the profile's server

- Status: Accepted
- Date: 2026-11-04

## Context

The user wants an item that shows whether GitHub and GitLab themselves work: githubstatus.com and status.gitlab.com. Both pages have a public JSON answer that needs no token.

Every other status in the github and gitlab modules goes through the module's `ApiClient`, which `StatusJob::server()` gives for the profile's server. That client sends the profile's token, and only to the origin of the server's base URL ([decision 0123](0123-tokens-stay-on-their-origin.md)). The status pages are on other hosts, `www.githubstatus.com` and `api.status.io`, so the client cannot reach them. [Integrations](../integrations.md#polling-and-caching) also says that all requests use one `QNetworkAccessManager`.

## Decision

- The github module has a GitHub Status type, `github.status`, and the gitlab module a GitLab Status type, `gitlab.status`. Each reads the public status page of the public service: Statuspage's `summary.json` for GitHub, and status.io's public status API for GitLab.
- Both report status. The status cache decides when they are fetched, as for any other type, so attention, notifications and the poll interval work unchanged. An open incident needs attention, and notifies when its impact is major or critical.
- The fetch does not use `job.server()`. Each module makes the request with a `QNetworkAccessManager` of its own, with no token and no cookie.
- Both types turn their page's answer into the same data, so their views are the same: the overall state, open incidents, planned maintenance and components. The view code is repeated in each module, because modules do not depend on each other.
- Only the public services are covered. GitHub Enterprise Server and self-managed GitLab have no such page.

## Alternatives

- **One shared module for status pages.** It would hold the view once, but GitHub's page is Statuspage and GitLab's is status.io, so little else would be shared, and a build with only one of the two modules would carry both.
- **A view that fetches on its own timer.** The page would not count for attention or the tray, and the request would go around the status cache.
- **A second `ServerType` for the status page.** A module has one server type, and a server needs a settings screen. Neither fits a fixed public address without a token.
- **Show the status page in a web view.** It works without code, but gives no attention and costs a web view's memory.

## Consequences

- An outage of GitHub or GitLab shows on the dashboard and in the tray count, and a major one notifies.
- The rule that all requests use one `QNetworkAccessManager` now has an exception for requests that carry no token. Such a request still waits for the status cache, but is not limited to four at once per server.
- A change to either page's format breaks only its type. Both readers are tested on recorded answers.
