# 0124. The core owns the app

- Status: Accepted
- Date: 2026-11-02

## Context

[Decision 0117](0117-app-repository-and-json-manifest.md) makes an app repository the top-level CMake project, and leaves open who owns `main.cpp`, resources, packaging and the translation of the app's own texts.

## Decision

The core owns all of them. [Decision 0118](0118-all-builds-are-one-app.md) makes every build the same app with the same names, so an app repository has nothing of these to configure. It holds only what differs between builds: the module list, the pins and the version.

## Alternatives

- **An app repository with its own `main.cpp` and resources.** Each custom build would have a copy to keep up to date, and a change to startup would reach every repository by hand.
- **A template that copies them once.** The copies would drift in the same way, without any check.

## Consequences

- An app repository is a manifest, a short `CMakeLists.txt` and a workflow, so a new core changes it only through its pin.
- A custom build cannot change the app's icon or name. An optional setting can still be added if a real need appears, as decision 0118 says of an app id.
