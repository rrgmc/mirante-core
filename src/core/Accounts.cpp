#include "core/Accounts.h"

#include "core/Model.h"
#include "core/Profiles.h"

#include <QCoreApplication>
#include <QJsonArray>

#include <algorithm>
#include <set>

namespace mirante {

std::optional<Account> Account::fromJson(const QJsonObject &json)
{
    Account account;
    account.id = json.value(QStringLiteral("id")).toString();
    if (account.id.isEmpty())
        return std::nullopt;
    account.name = json.value(QStringLiteral("name")).toString();
    account.values = json.value(QStringLiteral("values")).toObject();
    account.extra = json;
    account.extra.remove(QStringLiteral("id"));
    account.extra.remove(QStringLiteral("name"));
    account.extra.remove(QStringLiteral("values"));
    return account;
}

QJsonObject Account::toJson() const
{
    QJsonObject json = extra;
    json.insert(QStringLiteral("id"), id);
    json.insert(QStringLiteral("name"), name);
    if (!values.isEmpty())
        json.insert(QStringLiteral("values"), values);
    return json;
}

bool operator==(const Account &a, const Account &b) { return a.toJson() == b.toJson(); }

namespace accounts {

namespace {

// The stored entries, in stored order, without those skipped.
std::vector<Account> stored(const Model &model, const QString &profileId, const QString &moduleId)
{
    std::vector<Account> result;
    const Profile *profile = model.profile(profileId);
    if (!profile || moduleId.isEmpty())
        return result;
    std::set<QString> seen;
    for (const QJsonValue &value : profile->accounts.value(moduleId).toArray()) {
        std::optional<Account> account = Account::fromJson(value.toObject());
        if (account && seen.insert(account->id).second)
            result.push_back(*account);
    }
    return result;
}

void store(Model &model, const Profile &profile, const QString &moduleId, const std::vector<Account> &list)
{
    Profile changed = profile;
    QJsonArray array;
    for (const Account &account : list)
        array.append(account.toJson());
    if (array.isEmpty())
        changed.accounts.remove(moduleId);
    else
        changed.accounts.insert(moduleId, array);
    model.setProfile(changed);
}

} // namespace

QString defaultName() { return QCoreApplication::translate("Accounts", "Default"); }

std::vector<Account> others(const Model &model, const QString &profileId, const QString &moduleId)
{
    std::vector<Account> result = stored(model, profileId, moduleId);
    std::stable_sort(result.begin(), result.end(), [](const Account &a, const Account &b) {
        return a.name.compare(b.name, Qt::CaseInsensitive) < 0;
    });
    return result;
}

std::optional<Account> find(const Model &model, const QString &profileId, const QString &moduleId,
                            const QString &accountId)
{
    if (accountId.isEmpty())
        return std::nullopt;
    for (const Account &account : stored(model, profileId, moduleId)) {
        if (account.id == accountId)
            return account;
    }
    return std::nullopt;
}

QString nameOf(const Model &model, const QString &profileId, const QString &moduleId, const QString &accountId)
{
    const std::optional<Account> account = find(model, profileId, moduleId, accountId);
    return account ? account->name : defaultName();
}

QString effective(const Model &model, const QString &profileId, const QString &moduleId,
                  const QString &accountId)
{
    return find(model, profileId, moduleId, accountId) ? accountId : QString();
}

QString effective(const Model &model, const QString &taskId, const QString &itemId)
{
    const Item *item = model.item(taskId, itemId);
    if (!item || item->account.isEmpty())
        return {};
    const QString moduleId = item->type.section(QLatin1Char('.'), 0, 0);
    return effective(model, profiles::effective(model, taskId, itemId), moduleId, item->account);
}

QJsonObject screenValues(const Model &model, const QString &profileId, const QString &moduleId,
                         const QString &accountId, const QString &serverScreenId, const QString &screenId)
{
    if (!accountId.isEmpty() && screenId == serverScreenId) {
        if (const std::optional<Account> account = find(model, profileId, moduleId, accountId))
            return account->values;
    }
    return model.moduleSettings(profileId, moduleId, screenId);
}

void set(Model &model, const QString &profileId, const QString &moduleId, const Account &account)
{
    const Profile *profile = model.profile(profileId);
    if (!profile || account.id.isEmpty() || moduleId.isEmpty())
        return;
    std::vector<Account> list = stored(model, profileId, moduleId);
    const auto it = std::find_if(list.begin(), list.end(), [&](const Account &a) { return a.id == account.id; });
    if (it != list.end())
        *it = account;
    else
        list.push_back(account);
    store(model, *profile, moduleId, list);
}

bool remove(Model &model, const QString &profileId, const QString &moduleId, const QString &accountId)
{
    const Profile *profile = model.profile(profileId);
    if (!profile)
        return false;
    std::vector<Account> list = stored(model, profileId, moduleId);
    const auto it = std::find_if(list.begin(), list.end(), [&](const Account &a) { return a.id == accountId; });
    if (it == list.end())
        return false;
    list.erase(it);
    store(model, *profile, moduleId, list);
    return true;
}

} // namespace accounts
} // namespace mirante
