### Added

- `build-app.yml` checks the changelog fragments of an app or module repository that has `changelog.d/`. `tools/release-changelog.py --root <repository>` checks and releases another repository's fragments. It writes `VERSION` only where the repository has one.
