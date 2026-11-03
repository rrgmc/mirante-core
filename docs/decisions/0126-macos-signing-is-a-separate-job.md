# 0126. macOS signing is a separate job

- Status: Accepted
- Date: 2026-11-02

## Context

[Decision 0101](0101-macos-release-signing.md) has one job build, test, sign and notarise. Every build step can then reach the certificate and the notarisation key. For Mirante's own modules that is acceptable. For a module from someone else's repository it is not, because its CMake code and build steps would run with the certificate available. [Decision 0121](0121-modules-are-trusted-code.md) trusts a module's code inside the app, but not with the signing secrets of whoever builds it.

## Decision

`.github/workflows/build-app.yml` splits the work:

- **The build jobs hold no secrets.** They build, test, install and check. On macOS, the bundle is universal and signed ad hoc, and it travels as a tar file, because a zip would lose the symbolic links inside Qt's frameworks.
- **The signing job runs nothing from the build.** It checks out the core at its pin, downloads the bundle, and runs only `tools/sign-macos.sh`, `tools/package-macos.sh`, `tools/notarise-macos.sh`, `notarytool` and `stapler`. It reads no cache, and pins its actions to commits. It treats the bundle and its version as data.
- **`tools/package-macos.sh` makes the disk image with `hdiutil`,** because `cpack` would run the build's install scripts. It keeps CPack's layout, and fails if the layout differs. It tries `hdiutil` again after the chance failure `Resource busy`.
- **The input `macos-signing` is `none` or `developer-id`.** With `none`, the signing job still runs, with no environment or secrets, and signs ad hoc, so every build tests this path. With `developer-id`, it runs in the environment `release`, which admits only tags.
- **A last job publishes a release for a tag,** `vX.Y.Z` or the prerelease `vX.Y.Z-rc.N`. Only this job can write to the repository. It builds nothing.

`tools/release-macos.sh`, which builds and signs on the maintainer's Mac, stays until `mirante` moves to this workflow.

## Alternatives

- **Keep one job, and trust only first-party modules with the secrets.** A custom build with an outside module would have to choose between an unsigned build and exposing its certificate.
- **Run `cpack` in the signing job.** It runs the build's install scripts, which is the code this job must not run.
- **Sign on the builder's own Mac.** It keeps the secrets off GitHub, but the point of an app repository is that CI makes its executables.

## Consequences

- The bundle that CI tests is the ad hoc one. Signing changes its signatures, not its code, so the tests still apply, but the hardened runtime is first exercised when the signed app runs.
- The Developer ID path cannot be tested in `mirante-app`, which holds no secrets, by [decision 0116](0116-custom-builds-in-separate-repositories.md). It is first tried in `mirante`, on a prerelease tag.
- Signing a build still vouches for what its modules do, so whoever holds the certificate should review the modules they sign.
