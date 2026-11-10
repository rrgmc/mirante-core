### Changed

- Without the Developer ID, the macOS build job of `build-app.yml` makes the disk image itself, signed ad hoc, so a run needs one macOS runner instead of two. The separate signing job runs only when `macos-signing` is `developer-id`. See [decision 0155](docs/decisions/0155-ad-hoc-disk-image-in-the-build-job.md).
