# 0128. Module pages have their own origin, on a `module://` scheme

- Status: Accepted
- Date: 2026-11-03

## Context

A module can show web pages through `ItemContext::createWebView()`. Until now, a page that it shipped itself could only use a `qrc:` address. All `qrc:` pages share one origin, so the pages of two modules from different authors would share their storage and could read each other's data. The roadmap asks that each module's web pages have their own origin before third-party modules arrive. [The research on web content in modules](../research/module-web-content.md) compared `qrc:`, a local server and a scheme handler, and recommended the scheme handler.

## Decision

- **One scheme, with a host per module:** `module://<module id>/<path>`. It is registered beside `dash://` before `QApplication`, so no module changes when it registers. Its flags are `SecureScheme`, `LocalScheme` and `FetchApiAllowed`.
- **Static files only, for now.** `module://<id>/<path>` serves the resource `:/modules/<id>/web/<path>`, which a module already ships with `RESOURCES`. The module interface does not change.
- **Each profile of the list serves it,** so a module's page has the item's logins. The app profile does not, so module pages never sit beside built-in panels.
- **Other origins are refused.** The handler answers a top-level load, and a request from a page of the same module. Every other request fails.
- **The headers of `dash://`,** with `'self'` in place of `dash:`.
- **`qrc:` pages are refused** in item web views, so no module page falls back to the shared origin.

## Alternatives

- **A handler that modules answer from code,** with a new member of `ModuleRegistrar`. It would let a page get data from its module. No module needs it yet, and it is an interface change. It can be added later without changing addresses.
- **A local server per module.** Any local program could reach the port, and the module would manage the server's life. See the research.
- **A scheme per module.** Registering schemes after `QApplication` worked in the research, but the Qt documentation does not promise it.

## Consequences

- A module's page cannot use cookies, and gets no HTTP status codes. It keeps data in `localStorage`, per module.
- The external browser cannot open these pages, so the web toolbar hides its button for them.
- The research found that a refused `fetch` from another origin may wait instead of failing. A refused request returns no data either way, but the page that sent it may wait.
- Tests in `tests/web/tst_module_scheme.cpp` cover the addresses, the origins and the profiles, in release builds.
