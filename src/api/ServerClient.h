#pragma once

#include <QJsonObject>
#include <QList>
#include <QObject>
#include <QPair>
#include <QString>
#include <QStringList>

#include <memory>
#include <tuple>

class QNetworkAccessManager;

namespace mirante {

class ApiClient;
class Model;
class ModuleDirectory;
class SecretStore;

// One module's server: the account of a module in a profile. The default
// account has an empty accountId. See decision 0146.
struct ServerId
{
    QString profileId;
    QString moduleId;
    QString accountId;

    friend bool operator<(const ServerId &a, const ServerId &b)
    {
        return std::tie(a.profileId, a.moduleId, a.accountId) < std::tie(b.profileId, b.moduleId, b.accountId);
    }
    friend bool operator==(const ServerId &a, const ServerId &b)
    {
        return a.profileId == b.profileId && a.moduleId == b.moduleId && a.accountId == b.accountId;
    }
};

// The modules' servers of the open workspace: one ApiClient for each account
// of each profile and module with a ServerType, made on first use, sharing one
// QNetworkAccessManager. Each client queues at most four requests at once,
// keeps ETags, follows Link pages, waits on 429 and on GitHub's rate-limited
// 403, and pauses while its token
// is refused. See the M5 plan, part 3, and docs/integrations.md.
// Implemented in ServerClient.cpp.
class ServerClients : public QObject
{
    Q_OBJECT

public:
    // Without a network manager, makes its own. Tests pass one that answers
    // from a table. Everything passed must outlive this object.
    ServerClients(const Model &model, const ModuleDirectory &modules, SecretStore &secrets,
                  QNetworkAccessManager *network = nullptr, QObject *parent = nullptr);
    ~ServerClients() override;

    // The module's server in the profile, with the account's values and
    // token, or null if the module registered no ServerType. An empty
    // accountId is the default account. The client lives until clear() or
    // this object goes.
    ApiClient *client(const QString &profileId, const QString &moduleId, const QString &accountId = QString());

    // A client of its own for the module's server in the profile, with the
    // values of an open settings dialog instead of the stored ones. Its token
    // is the one typed for the ServerType's token field in typedSecrets, or
    // none if removedSecrets names that field, or else the stored one. It
    // shares nothing with client(): no ETags, and a refused token does not
    // mark the profile's server as failing. Null if the module registered no
    // ServerType. The caller owns it, and it must not outlive this object.
    // See decision 0138.
    std::unique_ptr<ApiClient> clientFor(const QString &profileId, const QString &moduleId,
                                         const QString &accountId, const QJsonObject &values,
                                         const QJsonObject &typedSecrets, const QStringList &removedSecrets);

    // True while the client's token is refused. See failingChanged.
    bool failing(const QString &profileId, const QString &moduleId, const QString &accountId = QString()) const;
    // The failing servers.
    QList<ServerId> failingServers() const;

    // Drops every client, for another workspace: the workspace id is part of
    // every token key. Requests in flight end without a callback.
    void clear();

signals:
    // A client started or stopped failing.
    void failingChanged(const mirante::ServerId &server);

private:
    struct Private;
    std::unique_ptr<Private> d;
};

} // namespace mirante
