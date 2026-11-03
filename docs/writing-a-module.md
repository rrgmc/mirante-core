# Writing a module

This guide takes you from an empty repository to a module that an app built from [`mirante-app-template`](https://github.com/rrgmc/mirante-app-template) can list. It is for authors outside this project. [Modules](modules.md) is the full contract between the core and the modules, and this guide links to it for the details.

A **module** is a folder of C++ code that adds item types, and sometimes settings, to Mirante. It is compiled into the app and runs inside it. A **repository of modules** holds one or more module folders. An **app repository** lists the modules to build in its manifest, `mirante.json`.

## Before you start

You need:

- Qt 6.8, with the Qt modules that [building](../BUILDING.md) lists;
- CMake 3.21 or later, and Ninja;
- a C++17 compiler: MSVC 2022 on Windows, Clang on macOS, GCC or Clang on Linux;
- Git, and a GitHub account if you want CI.

## Choose an id

The module id names your module everywhere: in the build, in its item types and in users' data.

- It has lowercase letters and digits only.
- It starts with your GitHub name or your organisation's, in lowercase without separators, so it cannot clash with another author's. The module `greeting` of the owner `acme` is `acmegreeting`.
- It never changes, because users' items and settings store it. A renamed module shows its items as a missing module.
- Every id your module registers is the module id followed by a dot and more, such as `acmegreeting.card`.

The ids of the first-party modules, such as `gitlab` and `web`, and the prefix `dash`, are reserved. See [ids](modules.md#ids).

This guide uses the owner `acme` and the module `acmegreeting` as its example.

## The repository

```
acme-mirante-modules/
  .gitignore              /build/, the build folders
  CMakeLists.txt          the repository's own build, for CI and for working
  CMakePresets.json
  ci/manifest.json        the modules that the repository's own build lists
  .github/workflows/ci.yml
  LICENSE
  README.md
  acmegreeting/           the module
    CMakeLists.txt
    README.md
    GreetingModule.cpp
    icons/greeting.svg
    tests/tst_register.cpp
    translations/
```

The module's own `README.md` lists its item types, its settings and its settings screens. The top `README.md` says what the repository holds, and what [people who build with it should know](#what-people-who-build-with-your-module-should-know).

Keep each module in a folder of its own, even if the repository holds only one. An app never runs the top `CMakeLists.txt`: the core adds each listed folder by itself. See [decision 0125](decisions/0125-how-modules-are-fetched.md).

### The repository's own build

The top `CMakeLists.txt` makes the repository an app whose modules are its own folders. `FETCHCONTENT_SOURCE_DIR_ACME_MODULES` tells the core to take the folders from this checkout, not from GitHub. Its name is `FETCHCONTENT_SOURCE_DIR_` and the manifest's `name` in capitals.

```cmake
cmake_minimum_required(VERSION 3.21)

project(mirante LANGUAGES NONE)
enable_testing()

set(MIRANTE_MANIFEST "${CMAKE_CURRENT_SOURCE_DIR}/ci/manifest.json")
set(FETCHCONTENT_SOURCE_DIR_ACME_MODULES "${CMAKE_CURRENT_SOURCE_DIR}")
file(READ "${MIRANTE_MANIFEST}" manifest)
string(JSON core_repo GET "${manifest}" core repo)
string(JSON core_ref GET "${manifest}" core ref)

include(FetchContent)
FetchContent_Declare(mirante_core GIT_REPOSITORY "${core_repo}" GIT_TAG "${core_ref}")
FetchContent_MakeAvailable(mirante_core)
```

`ci/manifest.json` pins the core and lists the folders. The module entry's `repo` and `ref` are not used, because the folders come from the checkout, but they must be there. For the core's `ref`, use the one that the template's `mirante.json` pins, or the newest commit of the core's default branch, which `git ls-remote https://github.com/rrgmc/mirante-core HEAD` prints. Use the whole 40-character hash.

```json
{
  "qt": [],
  "core": { "repo": "https://github.com/rrgmc/mirante-core", "ref": "<tag or commit>" },
  "modules": [
    {
      "name": "acme_modules",
      "repo": "this repository",
      "ref": "this commit",
      "folders": ["acmegreeting"]
    }
  ]
}
```

`CMakePresets.json` needs a preset named `ci`, which CI uses. Turn warnings into errors there, so your own CI finds them. An app that lists your module builds it without that option.

```json
{
  "version": 3,
  "cmakeMinimumRequired": { "major": 3, "minor": 21, "patch": 0 },
  "configurePresets": [
    { "name": "base", "hidden": true, "generator": "Ninja", "binaryDir": "${sourceDir}/build/${presetName}" },
    { "name": "dev", "inherits": "base", "cacheVariables": { "CMAKE_BUILD_TYPE": "Debug" } },
    { "name": "ci", "inherits": "base",
      "cacheVariables": { "CMAKE_BUILD_TYPE": "Release", "MIRANTE_MODULES_WERROR": "ON" } }
  ],
  "buildPresets": [
    { "name": "dev", "configurePreset": "dev" },
    { "name": "ci", "configurePreset": "ci" }
  ],
  "testPresets": [
    { "name": "dev", "configurePreset": "dev", "output": { "outputOnFailure": true } },
    { "name": "ci", "configurePreset": "ci", "output": { "outputOnFailure": true } }
  ]
}
```

## The module

### Its build

`acmegreeting/CMakeLists.txt` calls one function:

```cmake
mirante_add_module(acmegreeting
    API       0.1
    SOURCES   GreetingModule.cpp
    RESOURCES icons/greeting.svg
    TESTS     tests/tst_register.cpp)
```

- **`API`** is the version of the module interface that the module is written for. While the interface is 0.x, it must match the core's exactly, or configuration stops. [The interface changelog](../src/module_api/CHANGELOG.md) says what changed in each version.
- **`RESOURCES`** are compiled in, served as `:/modules/acmegreeting/<path>`.
- **`TESTS`** become CTest tests named `module_acmegreeting_<file>`.
- **`QT_LIBS`** names further Qt modules, such as `PdfWidgets`. Each must also be in the manifest's `qt` list. Most modules need none.
- **`DEPENDS`** names other modules whose headers this one uses. Most modules need none.

See [building](modules.md#building) for every argument.

### Its code

The core calls one function of each module, `mirante::modules::<id>::registerModule()`. It adds the module's parts to a `ModuleRegistrar`.

```cpp
// The acmegreeting module: a card that shows a greeting.
#include <mirante/Fields.h>
#include <mirante/ItemContext.h>
#include <mirante/ItemType.h>
#include <mirante/Module.h>

#include <QCoreApplication>
#include <QLabel>
#include <QUuid>

#include <memory>
#include <vector>

namespace mirante::modules::acmegreeting {

namespace {

class CardItemType : public ItemType
{
public:
    QString id() const override { return QStringLiteral("acmegreeting.card"); }
    QString name() const override { return QCoreApplication::translate("acmegreeting", "Greeting card"); }
    QIcon icon() const override { return QIcon(QStringLiteral(":/modules/acmegreeting/icons/greeting.svg")); }

    std::vector<Field> fields() const override
    {
        Field text;
        text.key = QStringLiteral("text");
        text.label = QCoreApplication::translate("acmegreeting", "Greeting");
        text.defaultValue = QStringLiteral("Hello");
        text.required = true;
        return {text};
    }

    QWidget *createView(ItemContext &context, QWidget *parent) const override
    {
        auto *label = new QLabel(context.config().value(QStringLiteral("text")).toString(), parent);
        label->setAlignment(Qt::AlignCenter);
        return label;
    }
};

} // namespace

void registerModule(ModuleRegistrar &registrar)
{
    registrar.setModuleName(QCoreApplication::translate("acmegreeting", "Greeting"));
    registrar.setModuleDescription(QCoreApplication::translate("acmegreeting", "A card that shows a greeting."));
    // Generated once for this module, and never changed.
    registrar.setModuleUuid(QUuid(QStringLiteral("b5084c2a-ed31-4a3a-abf8-e830fd088856")));
    registrar.setModuleVersion(QStringLiteral("0.1.0"));
    registrar.setModuleAuthor(QStringLiteral("Acme"));
    registrar.addItemType(std::make_unique<CardItemType>());
}

} // namespace mirante::modules::acmegreeting
```

Replace the UUID with one you generate, such as with `uuidgen` or PowerShell's `New-Guid`. No two modules share one, and the app stops at start on a null UUID.

The rules for module code:

- Include only `<mirante/...>` headers, Qt, the standard library and your module's own files. CI refuses any other core header, and any path with `..`.
- Mark every overridden method `override`. The build fails without it on GCC and Clang.
- Every text the user sees uses `QCoreApplication::translate()` with the module id as its context, never `tr()`.
- Everything runs on the main thread.

See [rules for module code](modules.md#rules-for-module-code). What else a module can register, such as settings screens, a server with a token, status and quick add, is in [what a module registers](modules.md#what-a-module-registers) and [item context](modules.md#item-context).

### Its tests

Tests link the module and the testing library, not the core. `TestRegistrar` registers one module, as the core would:

```cpp
#include <mirante/testing/TestRegistrar.h>

#include <QTest>

namespace mirante::modules::acmegreeting {
void registerModule(mirante::ModuleRegistrar &registrar);
}

class TestRegister : public QObject
{
    Q_OBJECT

private slots:
    void registersTheCard()
    {
        mirante::testing::TestRegistrar registry;
        registry.registerModule(QStringLiteral("acmegreeting"), &mirante::modules::acmegreeting::registerModule);
        QVERIFY(registry.contains(QStringLiteral("acmegreeting.card")));
    }
};

QTEST_MAIN(TestRegister)
#include "tst_register.moc"
```

See [testing](modules.md#testing) for what else the testing library holds.

### Its translations

The app speaks English and Brazilian Portuguese. A module with a folder `translations/` carries its own texts. A module without the folder shows its English texts in every language. See [texts](modules.md#texts).

1. Make the empty folder `acmegreeting/translations/`.
2. Configure again, with `cmake --preset dev`, so the build sees the folder.
3. Build the target `update_translations`: `cmake --build --preset dev --target update_translations`. It writes `acmegreeting_en.ts` and `acmegreeting_pt_BR.ts` from your sources. It also updates the core's own files inside the build folder and prints their counts. That output is the core's, and you can ignore it.
4. Translate the texts in `acmegreeting_pt_BR.ts`, with Qt Linguist or by hand.
5. Commit both files. The English file holds only plural forms, so it is empty if your texts have none.

Run step 3 again whenever you add or change a text.

## Building and testing

```sh
cmake --preset dev -DCMAKE_PREFIX_PATH=<Qt folder>
cmake --build --preset dev
ctest --preset dev
```

`<Qt folder>` is the folder of Qt's compiler build, such as `C:\Qt\6.8.3\msvc2022_64`. The first configure needs the network, because it fetches the core.

On Windows:

- **Use the 64-bit compiler.** Run the commands in a Developer PowerShell for Visual Studio 2022 started for 64 bits. In a script, that is `Launch-VsDevShell.ps1 -Arch amd64`, from the `Common7\Tools` folder of Visual Studio. Without `-Arch amd64`, it starts the 32-bit compiler, which does not match Qt.
- **Keep the path short.** The build folder nests about 150 characters of its own, and Windows stops at 260 by default. Keep the checkout at a short path, such as `C:\src\acme-mirante-modules`, or set `binaryDir` to a short folder in a `CMakeUserPresets.json` of your own. A path that is too long fails to compile with `C1083: ... Invalid argument`.
- **Put Qt's `bin` folder on `PATH`** before `ctest`, so the tests find Qt's DLLs.
- **Ninja** must be on `PATH`. Visual Studio installs it with its CMake tools, and it is also a single download from [ninja-build.org](https://ninja-build.org).

The build makes the whole app, with your module in it, as `build/dev/bin/mirante`. To try it without touching your own data, start it with its own settings and data folders:

```sh
build/dev/bin/mirante --settings try/settings.ini --web-data try/web-data --no-keychain
```

## CI

`.github/workflows/ci.yml` calls the core's workflow, which builds, tests and packages on Windows, macOS and Linux. With `check-modules`, it also checks your includes and your translations. GitHub needs a literal ref after `uses:`, so the core's ref is written here too, and the build fails if it differs from the manifest's.

```yaml
name: CI

on:
  push:
    branches: [master]
  pull_request:

permissions:
  contents: read

jobs:
  build:
    uses: rrgmc/mirante-core/.github/workflows/build-app.yml@<the core ref in ci/manifest.json>
    with:
      manifest: ci/manifest.json
      check-modules: true
```

Each run also lists, on its summary page, which modules use the keychain, the network, processes and web storage. See [reviewing a module](modules.md#reviewing-a-module).

## Adding the module to an app

In the app's `mirante.json`, add your repository with a pin and the folder:

```json
{
  "name": "acme_modules",
  "repo": "https://github.com/acme/acme-mirante-modules",
  "ref": "v0.1.0",
  "folders": ["acmegreeting"]
}
```

Pin a tag to give apps only your releases. The template's update job moves a tag pin to your newest release tag, `vX.Y.Z`, and a commit pin to the newest commit of your default branch. If your module needs Qt modules beyond the core's, the app adds them to its `qt` list.

## When the interface changes

The core's module interface has a version. A new core with a new version stops every module that states the old one, with a message that names it. Read [the interface changelog](../src/module_api/CHANGELOG.md), change your code, raise `API`, and release a new tag. An app can take the new core only when all its modules have moved. See [decision 0119](decisions/0119-module-interface-versions-and-checks.md).

## What people who build with your module should know

Say these in your README, because they apply to every module:

- **A module is trusted code.** It runs in the app's process, with everything the app can reach: the tokens in the keychain, the web logins, the files, the network and other programs. Mirante has no sandbox for modules. See [decision 0121](decisions/0121-modules-are-trusted-code.md).
- **Ask for narrow tokens.** If your module talks to a server, tell people which token permissions it needs, and recommend the narrowest token that works, such as read-only access to the projects it shows.
- **Your licence applies to the binary.** An app that lists your module ships it compiled in, so whoever publishes that app must meet your licence's terms. Choose a licence, put it in `LICENSE`, and say it in your README. The core and the first-party modules use the MIT License.
- **Builds are unsigned.** An app built with the template is not signed unless its owner adds their own certificates. Windows warns when such a package is first opened, and macOS refuses it until it is allowed in System Settings.
