# 0132. Extension points: a module lets an app replace part of its code

- Status: Accepted
- Date: 2026-11-03

## Context

The user asked that a module can let an app replace part of its code at build time, set in the manifest. For example, a native module's view could take its layout from code that the app gives, through an interface the two agree on.

## Decision

With the user's choice, an extension is a folder that the manifest names, compiled into the module.

- **A module declares its extension points** in `mirante_add_module()`, with the sources that implement each one when the app gives none: `EXTENSIONS runlayout=RunLayoutDefault.cpp`. Several default files are separated by commas. A point's name has lowercase letters and digits. This has the form of `OPTIONS`, because CMake cannot repeat a keyword with its own arguments.
- **The interface is the module's own.** A header of the module declares what an extension implements, such as a function `std::unique_ptr<RunLayout> makeRunLayout()`. Its default sources define it, and so does an extension. The module's README documents each point. The core does not know the interface.
- **The manifest names an extension** in the module's folder entry: `"extensions": { "runlayout": { "local": "extensions/run-layout" } }` for a folder of the app repository, or `{ "name", "repo", "ref", "folder" }` for a folder of another repository, fetched as a repository of modules is. The `name` lets a checkout on disk stand in with `FETCHCONTENT_SOURCE_DIR_<NAME>`.
- **What is compiled.** Every `.cpp` and `.h` file of the extension's folder, in place of the point's defaults, into the module's own target. The module's folder is on the include path, so an extension includes the module's headers by name. It sees the module interface and Qt, and nothing else. Resources that its `resources.txt` lists, one path per line, are served as `:/modules/<module id>/<path>`.
- **Checks.** An extension point the module does not declare, a folder with no sources, and a mistaken entry stop configuration. `build-app.yml` runs the include check on every extension folder. `-Werror=suggest-override` applies, as to the module.
- **The About dialog** lists each extension under its module, as `<module>/<point>`, with its repository and commit.

## Alternatives

- **Custom code as a module of its own,** found at run time. The interface would live in one module's headers and be reached through `DEPENDS`, and a missing implementation would show only when the app runs.
- **An extension's own `CMakeLists.txt`.** It could do anything a module can, which makes the extension a second module in all but name. A folder of sources and a list of resources keep it to what it replaces.
- **The plan's form `EXTENSION <point> DEFAULT <files>`.** It cannot be repeated for a second point with `cmake_parse_arguments()`.

## Consequences

- An extension must match the module's interface for its point. A change to that interface breaks the extensions, so a module treats it like its public interface and says so in its changelog.
- An extension is code from the app, compiled into the module, and has the module's reach. It is trusted, as modules are ([decision 0121](0121-modules-are-trusted-code.md)).
- A sample in `mirante-modules-samples` shows an extension point and an extension.
