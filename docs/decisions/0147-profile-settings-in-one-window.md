# 0147. A profile's settings are edited in one window

- Status: Accepted
- Date: 2026-11-04

## Context

A profile's settings were spread over several places. Profiles > Manage Profiles… created, renamed and deleted profiles. Each profile had a submenu in the Profiles menu, with Modules > <module> > <screen> for each settings screen, Set as Default, and Settings…, which opened a separate window with the profile's storage. [Decision 0146](0146-several-accounts-per-module.md) added an entry for each account and Add Account… to the module submenus. The user found the menus too deep, and asked for everything to be in the profiles window.

## Decision

- **One window.** Profiles > Manage Profiles… opens the Profiles window. It holds everything about profiles: the list, each profile's general settings and storage, and every module settings screen of every account.
- **A tree on the left, a page on the right.** The tree lists the profiles, the default first. Under each profile: General, then each module with settings screens. A module with a server lists its accounts under it, "Default" first, then the others by name, then its other screens. A module without a server and with one screen has no children; its node is that screen.
- **Pages.** A profile's General page edits its name, makes it the default, says who uses it, shows its storage on this device, and has Clear Data… and Delete Profile…. A module's page describes it and, for a module with a server, has Add Account. An account's or a screen's page is the settings form of [decision 0044](0044-module-settings-in-the-workspace.md), with Add from Server and tokens as before, and, for another account, its name and Remove Account.
- **Apply and Revert per page.** A page that edits values has Apply and Revert, enabled while something changed. Leaving a changed page, or closing the window, asks to apply, discard, or stay. Adding, deleting and removing profiles and accounts, making a profile the default, and clearing its data happen at once, as before.
- **Every way in opens the window at the right place.** The warning bar's Set Token…, an item's fix button for a missing or refused token, and each profile's entry in the Profiles menu open the window at that account or profile.
- **The Profiles menu** keeps Manage Profiles… and lists the profiles, each opening the window at its General page. The Modules, Set as Default and Settings… entries, and the separate Profile Settings window, are removed.
- **Changes from elsewhere.** The tree follows the model while the window is open. A page without changes shows the new values; a page with changes keeps them.

## Alternatives

- **Keep the submenus and add a link to the window.** Settings would still be in two places, and the account entries would still make the menus long.
- **One OK and Cancel for the whole window.** Tokens are written to the keychain, and another device can change the workspace while the window is open, so one transaction over everything cannot be undone reliably.
- **Tabs for the modules on a profile's page.** Tabs do not scale with modules and accounts, and cannot show accounts as children.

## Consequences

- A profile's settings are found in one place, two clicks from the menu bar.
- The window is larger, 900 by 600 pixels by default.
- `EntityActions::editModuleSettings()` still opens one screen alone, for code and tests that need only that; the app opens the Profiles window instead.
