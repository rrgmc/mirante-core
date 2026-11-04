# 0143. The cell header opens an item's web link

- Status: Accepted
- Date: 2026-11-04

## Context

Many items have a page on the web: a pull request, a merge request, a Jira issue, a list of them, or the page a Web page item shows. Two parts of the app opened that page in the external browser, each in its own way.

- A web view's toolbar had an **Open in browser** button, at the end of the address.
- A native view put a "Web" link in its `ActionRow` or its `ItemListView`, inside the cell's body. It sat in a different place in each view, and a view without one of these widgets had no way to offer it.

The user asked for one feature that any item type can use: the type gives an address, and the cell shows the button.

## Decision

- `ItemContext::setWebLink(url)` gives the item's address on the web. A view calls it again whenever the address changes, such as when its data arrives. An empty address removes it. The default does nothing, so contexts without a placement need no change.
- The cell header shows an **Open in browser** button for that address, after the view's header widget and before the status. It opens the address in the external browser, and its tooltip shows the address. Only an `http` or `https` address shows the button. `module://`, `dash://`, `data:` and other addresses do not.
- A web view of `createWebView()` sets its page's address as the web link, and follows the page as it moves. Its toolbar no longer has a button of its own.
- While a page of `showPage()` is shown, the button opens that page. The view's own address comes back with the view, including a change made while the page was shown.
- A new view starts without a web link, so an address cannot outlive the view that gave it.
- `ActionRow::setWebLink()` and `ItemListView::setWebLink()` stay in the module interface, so modules built for an older core still build. First-party modules move to `setWebLink()` on the context.

## Alternatives

- **A function of the item type that returns the address.** The type has no data. A pull request's address comes from its data or from the profile's base URL, and it can change when the view refreshes. Only the view knows it.
- **Keep the "Web" link in the view's body.** Each view would place it differently, and views without an `ActionRow` could not offer it.
- **A cell menu entry only.** The cell menu shows its button only while the dashboard is unlocked, and a right click is harder to find than a button.

## Consequences

- Every item type can offer its page in the browser with one call, in the same place in every cell.
- Web page items keep the button in the same place, beside the address.
- The core needs a new release before modules can use `setWebLink()`. Until a module moves, its "Web" link stays in its body.
- A later version of the module interface can remove `ActionRow::setWebLink()` and `ItemListView::setWebLink()`, once no module uses them.
