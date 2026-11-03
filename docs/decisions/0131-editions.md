# 0131. Editions name an app repository's build, which stays the same app

- Status: Accepted
- Date: 2026-11-03

## Context

Every app repository makes packages with the same names, such as `mirante-0.1.0-win64.exe`, so two custom builds cannot be told apart by their files. The user asked for a name per app repository that appears in the file names, among other places. [Decision 0118](0118-all-builds-are-one-app.md) makes every build the same app.

## Decision

- **The name is the edition.** With the user's choice, it is called the edition, and every edition is still the same app, as decision 0118 says.
- **`edition`** at the top of the manifest. It has lowercase letters, digits and `-`, up to 32 characters, and starts and ends with a letter or a digit. Without it, a build has no edition.
- **It names the files:** `mirante-<edition>-<version>-win64.exe`, `mirante-<edition>-<version>-macos.dmg` and `mirante-<edition>_<version>_amd64.deb`. Without an edition, the names stay as before.
- **The About dialog** shows it after the version, such as "0.1.0 (work)", from `MIRANTE_EDITION` in the generated `Version.h`.
- **The release** is titled "Mirante <edition> vX.Y.Z". `build-app.yml` gives the edition as an output, and `release-app.yml` takes it as an input.
- **Nothing else changes:** the app's name, the bundle identifier, the Debian package's name `mirante`, the Windows installer's display name, the install folders, the settings and the keychain entries. Installing one edition replaces another.

## Alternatives

- **An edition that is its own app,** with its own settings and keychain entries, so two editions can be installed side by side. The user chose the same app, which keeps decision 0118. A later decision can supersede it if two editions on one computer are needed.
- **Calling it a distribution or a fork.** The user chose edition.
- **The edition in the Debian package's name.** Two editions would then be two packages that install the same files, which `dpkg` refuses.

## Consequences

- An app repository's packages can be told apart by name, and the About dialog tells which edition runs.
- The same app installed from two editions shares everything, as before.
- The macOS signing job takes the edition from the `prepare` job, so the disk image's name needs nothing from the build.
