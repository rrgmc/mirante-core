# 0155. Without the Developer ID, the macOS build job makes the disk image

- Status: Accepted
- Date: 2026-11-10

## Context

By [decision 0126](0126-macos-signing-is-a-separate-job.md), a separate job signs the macOS bundle and makes the disk image, also when `macos-signing` is `none` and it signs ad hoc. That job needs a second macOS runner in every run. macOS runners are the scarcest and slowest to start, so every pull request waits for one only to sign ad hoc and run `hdiutil`.

The install already signs the bundle ad hoc, with `tools/sign-macos.sh`. The signing job's ad hoc signature repeats it. Its separation protects only the signing secrets, and with `none` there are none.

## Decision

- **With `macos-signing: none`, the build job makes the disk image.** It runs `tools/package-macos.sh` on the installed bundle, which the install signed ad hoc, and uploads the disk image as the artifact `mirante-macOS`, as the signing job did.
- **With `developer-id`, nothing changes.** The build job packs the bundle as a tar file, and the signing job signs, makes the disk image and notarises it, in the environment `release`.
- **The signing job runs only for `developer-id`.** Its steps no longer check the input.

This supersedes the part of decision 0126 in which the signing job runs, and signs ad hoc, with `none`.

## Alternatives

- **Keep the signing job for every run.** It tests the separate job's path on every build, but costs a macOS runner each time, for a path that only a release uses.
- **Use `cpack` in the build job.** The build job may run the build's install scripts, but `tools/package-macos.sh` gives the same layout as the signing job's disk image, so both paths share one script.

## Consequences

- A pull request's run needs one macOS runner instead of two, and finishes sooner.
- The signing job's path is no longer tested on every build. It runs first on a release tag of an app that sets `developer-id`, such as `mirante`, so a fault there shows only at a release. A prerelease tag `vX.Y.Z-rc.N` tests it without a final release.
