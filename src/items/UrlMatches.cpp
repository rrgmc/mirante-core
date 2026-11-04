#include "items/UrlMatches.h"

#include "core/Accounts.h"
#include "core/Model.h"
#include "items/ItemTypeRegistry.h"

#include <mirante/ApiClient.h>
#include <mirante/ItemType.h>

#include <algorithm>
#include <map>

namespace mirante::urlmatches {

QUrl webAddress(const QString &text)
{
    const QString trimmed = text.trimmed();
    if (trimmed.isEmpty() || trimmed.contains(QLatin1Char(' ')) || trimmed.contains(QLatin1Char('\n')))
        return {};
    const QUrl url(trimmed, QUrl::StrictMode);
    const QString scheme = url.scheme().toLower();
    if (!url.isValid() || url.host().isEmpty()
        || (scheme != QLatin1String("http") && scheme != QLatin1String("https")))
        return {};
    return url;
}

QString accountFor(const ItemTypeRegistry &types, const Model &model, const QString &profileId,
                   const QString &moduleId, const QUrl &url)
{
    const ServerType *server = types.serverType(moduleId);
    if (!server || url.host().isEmpty())
        return {};
    const auto onHost = [&](const QJsonObject &values) {
        return server->baseUrl(values).host().compare(url.host(), Qt::CaseInsensitive) == 0;
    };
    if (onHost(model.moduleSettings(profileId, moduleId, server->screenId())))
        return {};
    for (const Account &account : accounts::others(model, profileId, moduleId)) {
        if (onHost(account.values))
            return account.id;
    }
    return {};
}

std::vector<TypeMatch> match(const ItemTypeRegistry &types, const Model *model, const QUrl &url,
                             const QString &profileId)
{
    std::vector<TypeMatch> result;
    if (url.isEmpty())
        return result;
    std::map<QString, QString> accountOf;   // by module id
    for (const ItemType *type : types.types()) {
        const QString moduleId = type->id().section(QLatin1Char('.'), 0, 0);
        auto account = accountOf.find(moduleId);
        if (account == accountOf.end())
            account = accountOf.emplace(moduleId, model ? accountFor(types, *model, profileId, moduleId, url)
                                                        : QString()).first;
        std::optional<UrlMatch> found = type->fromUrlForAccount(url, profileId, account->second);
        if (!found)
            continue;
        TypeMatch entry;
        entry.type = type;
        entry.match = *found;
        entry.accountId = account->second;
        result.push_back(std::move(entry));
    }
    // The registry lists types by name; generic types go last.
    std::stable_partition(result.begin(), result.end(), [](const TypeMatch &entry) { return !entry.match.generic; });
    return result;
}

} // namespace mirante::urlmatches
