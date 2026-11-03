#!/usr/bin/env python3
"""Reports which modules use sensitive APIs, as a help for reviewing them.

    python tools/scan-module-apis.py <folder>...
    python tools/scan-module-apis.py --build <build folder>

Each folder is one module. With --build, the folders are those of the modules
that the build's manifest added, from mirante-module-folders.txt, which
configuration writes into the build folder.

The scan looks for four kinds of use, in the .h and .cpp files of a module,
except in its tests/, in build/ and in folders whose name starts with a dot:

- keychain: tokens, through Secret fields, a server type or the keychain itself
- network: requests and sockets, also through the module's server
- processes: starting programs, terminals and shell commands, and opening URLs
- web storage: web views and profiles, which hold logins and cookies

A module is trusted code, so it can reach all of these without the module
interface (decision 0121). The scan only finds names in the text, so it misses
what is hidden, and it reports the module interface's own safe paths too. It
helps a reviewer choose what to read first. It is not a check.

Prints a Markdown report and always exits with status 0, unless the arguments
are wrong.
"""

import pathlib
import re
import sys

SOURCE_SUFFIXES = {".h", ".cpp"}

# Each kind lists names that show a use. A name matches as a whole word.
KINDS = {
    "keychain": [
        "Secret", "SecretState", "secrets", "removedSecrets", "tokenKey",
        "ServerType", "setServerType", "QKeychain", "keychain",
        "CredRead", "CredWrite", "SecItemCopyMatching", "SecItemAdd", "secret_password_lookup",
    ],
    "network": [
        "QNetworkAccessManager", "QNetworkRequest", "QNetworkReply", "QTcpSocket", "QUdpSocket",
        "QSslSocket", "QWebSocket", "QLocalSocket", "QTcpServer", "QLocalServer", "QHttpServer",
        "ApiClient", "server", "socket", "WinHttpOpen", "InternetOpen", "curl_easy_init",
    ],
    "processes": [
        "QProcess", "launchTerminal", "startProgram", "showFolder", "QDesktopServices",
        "system", "popen", "_popen", "fork", "execv", "execvp", "execl", "posix_spawn",
        "CreateProcess", "CreateProcessW", "ShellExecute", "ShellExecuteW", "ShellExecuteEx", "WinExec",
    ],
    "web storage": [
        "createWebView", "showPage", "QWebEngineProfile", "QWebEngineCookieStore", "QWebEnginePage",
        "QWebEngineView", "QWebEngineScript", "QWebChannel", "QWebEngineUrlSchemeHandler",
    ],
}

# Names that are too common as words: they count only as a call or a member.
CALLS_ONLY = {"server", "socket", "system", "popen", "_popen", "fork", "secrets", "keychain"}

COMMENT = re.compile(r"^\s*(//|\*|/\*)")


def patterns():
    """Returns one regular expression for each kind."""
    result = {}
    for kind, names in KINDS.items():
        plain = [re.escape(name) for name in names if name not in CALLS_ONLY]
        calls = [re.escape(name) for name in names if name in CALLS_ONLY]
        parts = []
        if plain:
            parts.append(r"\b(?:" + "|".join(plain) + r")\b")
        if calls:
            # ::name( or name( or ->name( or .name(, but not some_name(.
            parts.append(r"(?<![\w])(?:" + "|".join(calls) + r")\s*\(")
        result[kind] = re.compile("|".join(parts))
    return result


def is_source(file, module):
    """Returns whether a file is shipped code of the module."""
    if not file.is_file() or file.suffix not in SOURCE_SUFFIXES:
        return False
    folders = file.relative_to(module).parts[:-1]
    return not any(folder in ("build", "tests") or folder.startswith(".") for folder in folders)


def scan(module, kinds):
    """Returns, for each kind, the files that use it and the names found in each."""
    found = {kind: {} for kind in kinds}
    for file in sorted(p for p in module.rglob("*") if is_source(p, module)):
        name = file.relative_to(module).as_posix()
        text = file.read_text(encoding="utf-8", errors="replace")
        for line in text.splitlines():
            if COMMENT.match(line) or line.lstrip().startswith("#include"):
                continue
            for kind, pattern in kinds.items():
                for match in pattern.finditer(line):
                    found[kind].setdefault(name, set()).add(match.group(0).rstrip("(").strip())
    return found


def folders_from_build(build):
    """Returns (label, folder) for each module folder that the build added."""
    listing = build / "mirante-module-folders.txt"
    if not listing.is_file():
        print(f"{listing} does not exist. Configure the build with a manifest first.", file=sys.stderr)
        sys.exit(2)
    result = []
    for line in listing.read_text(encoding="utf-8").splitlines():
        if line.strip():
            label, folder = line.split("\t", 1)
            result.append((label, pathlib.Path(folder)))
    return result


def main():
    arguments = sys.argv[1:]
    if arguments[:1] == ["--build"] and len(arguments) == 2:
        modules = folders_from_build(pathlib.Path(arguments[1]).resolve())
    elif arguments and not arguments[0].startswith("-"):
        modules = [(pathlib.Path(a).resolve().name, pathlib.Path(a).resolve()) for a in arguments]
    else:
        print(__doc__.strip(), file=sys.stderr)
        return 2

    kinds = patterns()
    results = [(label, scan(folder, kinds)) for label, folder in modules]

    print("## Sensitive APIs in modules\n")
    print("Uses found by name, as a help for reviews. See `tools/scan-module-apis.py`.\n")
    # The table counts the files of each module that use each kind.
    print("| Module | " + " | ".join(kinds) + " |")
    print("|---|" + "---|" * len(kinds))
    for label, found in results:
        cells = [str(len(found[kind])) if found[kind] else "" for kind in kinds]
        print(f"| {label} | " + " | ".join(cells) + " |")
    for label, found in results:
        if not any(found.values()):
            continue
        print(f"\n### {label}\n")
        for kind in kinds:
            if found[kind]:
                files = [f"{name} ({', '.join(f'`{n}`' for n in sorted(names))})"
                         for name, names in found[kind].items()]
                print(f"- **{kind}:** " + "; ".join(files))
    return 0


if __name__ == "__main__":
    sys.exit(main())
