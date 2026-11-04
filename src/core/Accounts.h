#pragma once

#include <QJsonObject>
#include <QString>

#include <optional>
#include <vector>

namespace mirante {

class Model;

// Another account of a module in a profile: one set of values of the screen
// that the module's ServerType names, with its own token. The default account
// is the screen's values in the profile's moduleSettings, and has no entry.
// See docs/data-model.md#accounts and decision 0144.
struct Account
{
    QString id;
    QString name;
    QJsonObject values;
    QJsonObject extra;   // keys this version does not know, kept

    static std::optional<Account> fromJson(const QJsonObject &json);
    QJsonObject toJson() const;
};

bool operator==(const Account &a, const Account &b);

// The accounts of the profiles. Implemented in Accounts.cpp.
namespace accounts {

// The name lists show for the default account: "Default".
QString defaultName();

// The other accounts of the module in the profile, by name, ignoring case. An
// entry without an id, or with an id already listed, is skipped.
std::vector<Account> others(const Model &model, const QString &profileId, const QString &moduleId);

// The other account with this id, if the profile has it.
std::optional<Account> find(const Model &model, const QString &profileId, const QString &moduleId,
                            const QString &accountId);

// The name to show for an account id: defaultName() for the empty id or one
// the profile does not have.
QString nameOf(const Model &model, const QString &profileId, const QString &moduleId, const QString &accountId);

// The account an item uses: accountId if the profile has it, else the empty
// id of the default account.
QString effective(const Model &model, const QString &profileId, const QString &moduleId,
                  const QString &accountId);
// The same for an item of a task, in its effective profile. Empty if the
// item is unknown.
QString effective(const Model &model, const QString &taskId, const QString &itemId);

// The values of a screen of the module for an account. For another account
// and the server screen, the account's own values; else the profile's, as
// Model::moduleSettings() gives them.
QJsonObject screenValues(const Model &model, const QString &profileId, const QString &moduleId,
                         const QString &accountId, const QString &serverScreenId, const QString &screenId);

// Adds another account, or replaces the one with its id. Nothing happens for
// an unknown profile or an account without an id.
void set(Model &model, const QString &profileId, const QString &moduleId, const Account &account);

// Removes another account. Items that name it fall back to the default.
// Returns false if the profile has no such account.
bool remove(Model &model, const QString &profileId, const QString &moduleId, const QString &accountId);

} // namespace accounts
} // namespace mirante
