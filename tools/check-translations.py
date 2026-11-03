#!/usr/bin/env python3
"""Checks that every text of the app has its translation.

    python tools/check-translations.py [folder]

Without a folder, it checks the core's translations/. With one, it checks a
repository of modules: <module>/translations/ for each module, or translations/
for a repository of one module.

Run it after lupdate, which `task translations` runs, so the translation files
hold every text of the sources. It fails when:

- a message in a .ts file is unfinished or empty;
- a translation lacks a placeholder of its source, such as %1 or %n, or has
  one the source has not;
- a translation lacks the keyboard accelerator (&) or the trailing ellipsis
  of its source;
- module code calls tr(), whose context lupdate cannot know. Modules use
  QCoreApplication::translate() with their module id instead.

See translations/README.md and decision 0086.
"""

import re
import sys
import xml.etree.ElementTree as ET
from pathlib import Path

REPO = Path(__file__).resolve().parent.parent
PLACEHOLDER = re.compile(r"%(?:n|L?[1-9][0-9]?)")
# An accelerator is an & before a letter or digit; && is a literal &.
ACCELERATOR = re.compile(r"(?<!&)&(?!&)[\w]")
BARE_TR = re.compile(r"(?<![\w:.>])tr\(")


def placeholders(text):
    return sorted(set(m.replace("%L", "%") for m in PLACEHOLDER.findall(text)))


def check_form(where, source, form, problems, plural=False):
    if not form.strip():
        problems.append(f"{where}: empty translation")
        return
    expected = placeholders(source)
    found = placeholders(form)
    # A plural form may say "one" instead of %n.
    if plural and "%n" not in found:
        expected = [p for p in expected if p != "%n"]
    if expected != found:
        problems.append(f"{where}: placeholders {expected} became {found}")
    if bool(ACCELERATOR.search(source)) != bool(ACCELERATOR.search(form)):
        problems.append(f"{where}: the keyboard accelerator (&) is missing or added")
    if source.endswith("…") != form.endswith("…"):
        problems.append(f"{where}: the trailing ellipsis is missing or added")


def check_ts(path, problems):
    root = ET.parse(path).getroot()
    count = 0
    for context in root.iter("context"):
        name = context.findtext("name")
        for message in context.iter("message"):
            source = message.findtext("source") or ""
            translation = message.find("translation")
            kind = translation.get("type") if translation is not None else None
            if kind in ("obsolete", "vanished"):
                continue
            count += 1
            where = f"{path.name}: {name}: \"{source}\""
            if translation is None or kind == "unfinished":
                problems.append(f"{where}: unfinished")
                continue
            if message.get("numerus") == "yes":
                forms = [form.text or "" for form in translation.iter("numerusform")]
                if not forms:
                    problems.append(f"{where}: no plural forms")
                for form in forms:
                    check_form(where, source, form, problems, plural=True)
            else:
                check_form(where, source, translation.text or "", problems)
    return count


def is_skipped(path, root):
    """Returns whether a path is in a build folder or a folder that starts with a dot."""
    return any(part == "build" or part.startswith(".") for part in path.relative_to(root).parts[:-1])


def check_modules(root, problems):
    for path in sorted(root.rglob("*")):
        if path.suffix not in (".cpp", ".h", ".mm") or "tests" in path.parts or is_skipped(path, root):
            continue
        for number, line in enumerate(path.read_text(encoding="utf-8").splitlines(), 1):
            if BARE_TR.search(line):
                problems.append(f"{path.relative_to(root)}:{number}: module code calls tr(); use "
                                f"QCoreApplication::translate(\"<module id>\", ...)")


def main():
    if len(sys.argv) > 2:
        print(__doc__.strip(), file=sys.stderr)
        return 2
    root = Path(sys.argv[1]).resolve() if len(sys.argv) == 2 else REPO
    problems = []
    files = sorted(p for p in root.rglob("translations/*.ts") if not is_skipped(p, root))
    if not files:
        print(f"No translation files in {root}.")
        return 1
    for path in files:
        count = check_ts(path, problems)
        print(f"{path.relative_to(root)}: {count} texts")
    if len(sys.argv) == 2:
        check_modules(root, problems)
    for problem in problems:
        print(problem)
    if problems:
        print(f"{len(problems)} problems. Translate the new texts in translations/, as translations/README.md says.")
        return 1
    print("Every text is translated.")
    return 0


if __name__ == "__main__":
    sys.exit(main())
