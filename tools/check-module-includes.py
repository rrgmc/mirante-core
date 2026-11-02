#!/usr/bin/env python3
"""Checks that modules include only their own files and the module API.

    python tools/check-module-includes.py <folder>

The folder is a repository of modules. The script scans every .h and .cpp file
in it, except in build/ and in folders whose name starts with a dot. An #include
fails the check if its path starts with "../" or contains "/../", or if it names
a core header, that is, a path that starts with one of the core source folders.
<mirante/...> includes are the module API and are allowed.

Module tests, in the folder tests/ of a module, follow the same rules, except
that they may include their module's files with one "../".

The build already keeps the core folders off a module's include path. This
check catches relative paths, which the build cannot block.

Prints each failure as file:line and exits with status 1 if there is any.
"""

import pathlib
import re
import sys

CORE_FOLDERS = ("core/", "items/", "layout/", "ui/", "platform/", "web/", "api/")
SOURCE_SUFFIXES = {".h", ".cpp"}
INCLUDE = re.compile(r'^\s*#\s*include\s*[<"]([^>"]+)[>"]')


def problem(path, in_tests):
    """Returns why an include path is not allowed, or None if it is."""
    if in_tests and path.startswith("../"):
        path = path[len("../"):]
    if path.startswith("../") or "/../" in path:
        return "path with \"..\""
    if path.startswith(CORE_FOLDERS):
        return "core header"
    return None


def is_source(file, modules):
    """Returns whether a file is a module's source, and not a build's or a tool's."""
    if not file.is_file() or file.suffix not in SOURCE_SUFFIXES:
        return False
    folders = file.relative_to(modules).parts[:-1]
    return not any(folder == "build" or folder.startswith(".") for folder in folders)


def main():
    if len(sys.argv) != 2:
        print(__doc__.strip(), file=sys.stderr)
        return 2
    modules = pathlib.Path(sys.argv[1]).resolve()
    failures = []
    files = sorted(p for p in modules.rglob("*") if is_source(p, modules))
    for file in files:
        # A repository of one module has tests/ at its top.
        in_tests = "tests" in file.relative_to(modules).parts[:2]
        text = file.read_text(encoding="utf-8", errors="replace")
        for number, line in enumerate(text.splitlines(), start=1):
            match = INCLUDE.match(line)
            if not match:
                continue
            path = match.group(1).replace("\\", "/")
            reason = problem(path, in_tests)
            if reason:
                name = file.relative_to(modules).as_posix()
                failures.append(f"{name}:{number}: {reason}: {line.strip()}")

    for failure in failures:
        print(failure)
    if failures:
        print(f"{len(failures)} include(s) not allowed in {modules}. "
              "Modules may include their own files and <mirante/...> only.", file=sys.stderr)
        return 1
    print(f"Checked {len(files)} file(s) in {modules}: no forbidden includes.")
    return 0


if __name__ == "__main__":
    sys.exit(main())
