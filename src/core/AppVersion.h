#pragma once

#include <QString>

namespace mirante {

// The version as the About dialog shows it: "0.1.0", or with an edition
// "0.1.0 (work)". See docs/decisions/0131-editions.md.
QString displayVersion(const QString &version, const QString &edition);

} // namespace mirante
