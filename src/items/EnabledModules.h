#pragma once

#include <QList>
#include <QString>
#include <QStringList>

// Implemented by enabled_modules.cpp, which CMake generates in the build folder
// from cmake/enabled_modules.cpp.in. See docs/modules.md#building.

namespace mirante {

class ItemTypeRegistry;

// Where one module of this build came from.
struct ModuleSource
{
    QString id;
    QString repository;
    // A short commit, or "local" for a checkout on disk.
    QString commit;
    // What the About dialog shows: the release X.Y.Z of a tag pin, or else the
    // commit.
    QString release;
};

// Registers every module in this build, in alphabetical order of module id.
void registerEnabledModules(ItemTypeRegistry &registry);

// The ids of the modules in this build, sorted.
QStringList enabledModuleIds();

// Where each module in this build came from, sorted by id.
QList<ModuleSource> enabledModuleSources();

} // namespace mirante
