# 0144. A profile can hold several accounts of a module

- Status: Accepted
- Date: 2026-11-04

## Context

Since [decision 0054](0054-module-settings-replace-connections.md), a profile has one server per module: the module's settings screen holds one base URL, and the keychain holds one token. The user works with more than one account of the same service in one context, such as a personal GitHub account and a bot account, or github.com and a GitHub Enterprise server, and wants them in one profile. A second profile does not fit: it also splits the web logins and every other setting.

## Decision

- **An account** is one set of values of a module's server screen, the screen that its `ServerType` names, with its own token. Each profile has a **default account** for each module with a `ServerType`: the values stored as before, in `moduleSettings`. It may have any number of **other accounts**, each with an id, a name and its own values, such as its own base URL, repositories or Jira email.
- Other accounts are kept in the profile's entry in `workspace.mirante`, under `accounts`: an object of module ids, each an array of `{ "id", "name", "values" }`. They are synced like the module settings. The default account has no entry and no stored name; lists call it "Default".
- **An item chooses its account** in its dialog, with the **Account** row, shown for a type whose module has a `ServerType` when its profile has other accounts. The item keeps the id under `account`, written only when not empty. Without one, or when its profile has no account with that id, the item uses the default account. There is no account per group or project.
- **The token** of another account is kept under `<workspace id>/<profile id>/<module id>/<account id>/<field key>`. The default account keeps the key it had. Removing an account removes its tokens on this device; deleting its profile removes them too.
- **Only the server screen is per account.** The module's other screens, and project settings, stay one per profile.
- **The core does the choosing.** `ItemContext::server()`, `StatusJob::server()` and the quick-add requests give the client of the item's account, and their `moduleSettings()` give that account's values for the server screen. Each account has its own client, its own request limit, its own ETags, and fails on its own; the warning bar names the account.
- `ModuleSettings::accountValues()`, `ItemType::fieldsForAccount()` and `ItemType::fromUrlForAccount()` let a module read an account's values where it is given ids instead of a context: for a form's suggestions and for a pasted link. Their defaults fall back to the default account, so a module without them keeps working.
- **A pasted link** is read in the profile's accounts, the default first: the first account whose server is on the link's host, else the default. The new item gets that account.
- **The Modules submenu** of a profile lists the server screen once per account, such as "General Configuration…" and "General Configuration: Bot…", and has **Add Account…**. The dialog of another account has a name row and a **Remove Account** button.

## Alternatives

- **Accounts in place of the server screen,** a list where every account is equal. The settings of every workspace would change form, and so would every module that reads its settings. Keeping the default account where it was needs no migration.
- **An account per group, as profiles are chosen.** It adds another place to look when an item talks to the wrong server, for a choice that is rare. It can be added later.
- **Choose the account from the item's URL alone.** Two accounts on one server, such as a person and a bot on github.com, could not be told apart.
- **Accounts that hold only a base URL and a token.** Lists such as GitHub's repositories and Jira's email belong to one account and its token, so they would be wrong for the others.

## Consequences

- One profile can talk to several accounts of each service, each with its own token.
- The default account cannot be renamed or replaced by another. Making another account the default means moving its values by hand.
- An item moved to a profile that lacks its account uses that profile's default account until one with the same id exists.
