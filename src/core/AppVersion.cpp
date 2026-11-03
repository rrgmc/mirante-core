#include "core/AppVersion.h"

namespace mirante {

QString displayVersion(const QString &version, const QString &edition)
{
    if (edition.isEmpty())
        return version;
    return QStringLiteral("%1 (%2)").arg(version, edition);
}

} // namespace mirante
