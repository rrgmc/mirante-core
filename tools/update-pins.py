#!/usr/bin/env python3
"""Moves the pins of an app repository to the newest versions.

    python tools/update-pins.py [--body <file>]

Runs in the top folder of an app repository, which holds mirante.json. For the
core and for each repository of modules, it finds what is newer than the pin:

- A pin on a release tag, vX.Y.Z, moves to the newest release tag.
- A pin on a commit, 40 hexadecimal characters, moves to the newest commit of
  the repository's default branch.
- Any other pin, such as a branch or a prerelease tag, stays as it is.

It then writes the new pins into mirante.json, writes the core's new pin into
the workflows under .github/workflows, and raises the patch number of
MIRANTE_VERSION in CMakeLists.txt, so that merging the change publishes a new
release. With --body, it writes a pull request's description into <file>, with
a link to each difference.

On GitHub Actions, it sets the step output "changed" to true or false. See
docs/decisions/0127-automatic-pin-updates.md.
"""

import json
import os
import pathlib
import re
import subprocess
import sys

RELEASE_TAG = re.compile(r"^v(\d+)\.(\d+)\.(\d+)$")
COMMIT = re.compile(r"^[0-9a-f]{40}$")
VERSION = re.compile(r'(set\(MIRANTE_VERSION\s+")(\d+)\.(\d+)\.(\d+)(")')


def remote(options, repo, *patterns):
    """Returns the lines of git ls-remote for repo."""
    result = subprocess.run(["git", "ls-remote", *options, repo, *patterns],
                            capture_output=True, text=True, check=True)
    return result.stdout.splitlines()


def newest(repo, ref):
    """Returns the newest ref of the same kind as ref, or ref if it stays."""
    if RELEASE_TAG.match(ref):
        tags = [line.split("refs/tags/", 1)[1] for line in remote(["--tags", "--refs"], repo)]
        releases = [tag for tag in tags if RELEASE_TAG.match(tag)]
        return max(releases, key=lambda tag: tuple(int(n) for n in RELEASE_TAG.match(tag).groups()), default=ref)
    if COMMIT.match(ref):
        lines = remote([], repo, "HEAD")
        return lines[0].split()[0] if lines else ref
    return ref


def compare_link(repo, old, new):
    """Returns a link to the difference, or the repository if it is not on GitHub."""
    base = re.sub(r"\.git$", "", repo)
    if base.startswith("https://github.com/"):
        return f"{base}/compare/{old}...{new}"
    return base


def short(ref):
    return ref[:12] if COMMIT.match(ref) else ref


def replace_ref(text, repo, old, new):
    """Replaces the ref that follows repo in the manifest's text, keeping its layout."""
    pattern = re.compile(r'("repo"\s*:\s*"' + re.escape(repo) + r'"\s*,\s*"ref"\s*:\s*")' + re.escape(old) + '"')
    changed, count = pattern.subn(lambda m: m.group(1) + new + '"', text, count=1)
    return changed if count else None


def main():
    arguments = sys.argv[1:]
    body_file = None
    if arguments[:1] == ["--body"] and len(arguments) == 2:
        body_file = pathlib.Path(arguments[1])
    elif arguments:
        print(__doc__.strip(), file=sys.stderr)
        return 2

    manifest_file = pathlib.Path("mirante.json")
    text = manifest_file.read_text(encoding="utf-8")
    manifest = json.loads(text)

    entries = [("the core", manifest["core"])]
    entries += [(entry["name"], entry) for entry in manifest["modules"]]

    changes = []
    unchanged = []
    for label, entry in entries:
        old = entry["ref"]
        new = newest(entry["repo"], old)
        if new == old:
            unchanged.append(f"- {label}: {short(old)}")
            continue
        changes.append(f"- {label}: [{short(old)} to {short(new)}]({compare_link(entry['repo'], old, new)})")
        replaced = replace_ref(text, entry["repo"], old, new)
        if replaced is None:
            # The fields are in another order: write the whole file again.
            entry["ref"] = new
            replaced = json.dumps(manifest, indent=2) + "\n"
        text = replaced
        manifest = json.loads(text)
        if label == "the core":
            for workflow in pathlib.Path(".github/workflows").glob("*.yml"):
                content = workflow.read_text(encoding="utf-8")
                content = re.sub(r"(mirante-core/\.github/workflows/[a-z-]+\.yml@)" + re.escape(old),
                                 lambda m: m.group(1) + new, content)
                workflow.write_text(content, encoding="utf-8")

    output = os.environ.get("GITHUB_OUTPUT")
    if output:
        with open(output, "a", encoding="utf-8") as file:
            file.write(f"changed={'true' if changes else 'false'}\n")
    if not changes:
        print("All pins are the newest.")
        return 0

    manifest_file.write_text(text, encoding="utf-8")
    cmake = pathlib.Path("CMakeLists.txt")
    content = cmake.read_text(encoding="utf-8")
    match = VERSION.search(content)
    version = ""
    if match:
        major, minor, patch = (int(n) for n in match.group(2, 3, 4))
        version = f"{major}.{minor}.{patch + 1}"
        cmake.write_text(VERSION.sub(lambda m: m.group(1) + version + m.group(5), content, count=1), encoding="utf-8")

    body = ["Moves the pins to the newest versions:", "", *changes]
    if unchanged:
        body += ["", "Already the newest:", "", *unchanged]
    if version:
        body += ["", f"Merging publishes the release v{version}. If CI fails, the last release stays in place."]
    body += ["", "Opened by the update job, with `tools/update-pins.py` of the core."]
    report = "\n".join(body) + "\n"
    if body_file:
        body_file.write_text(report, encoding="utf-8")
    print(report)
    return 0


if __name__ == "__main__":
    sys.exit(main())
