// Tests asking the item types about a pasted address: which text is a web
// address, the order of the types that take it, and the profile each type is
// given to read its server from.
#include "core/Accounts.h"
#include "core/Model.h"
#include "items/ItemTypeRegistry.h"
#include "items/UrlMatches.h"

#include <mirante/ApiClient.h>
#include <mirante/ItemType.h>
#include <mirante/Module.h>

#include <QTest>

#include <memory>

using namespace mirante;

namespace {

// The profile the server type was last given.
QString lastProfile;

// Takes any address below the server of the profile "work", as a module
// that reads its base URL from its settings would.
class ServerItemType : public ItemType
{
public:
    QString id() const override { return QStringLiteral("testmod.server"); }
    QString name() const override { return QStringLiteral("Zeta server"); }
    QIcon icon() const override { return {}; }

    std::optional<UrlMatch> fromUrl(const QUrl &url, const QString &profileId) const override
    {
        lastProfile = profileId;
        if (profileId != QLatin1String("work"))
            return std::nullopt;
        const QUrl base(QStringLiteral("https://srv.example"));
        const std::optional<QString> path = pathBelow(url, base);
        if (!path)
            return std::nullopt;
        UrlMatch match;
        match.config.insert(QStringLiteral("path"), *path);
        match.baseUrl = base;
        return match;
    }
};

// Takes every address, as a generic match. Its name sorts first.
class AnyType : public ItemType
{
public:
    QString id() const override { return QStringLiteral("testmod.any"); }
    QString name() const override { return QStringLiteral("Alpha any"); }
    QIcon icon() const override { return {}; }

    std::optional<UrlMatch> fromUrl(const QUrl &, const QString &) const override
    {
        UrlMatch match;
        match.generic = true;
        return match;
    }
};

// Takes nothing, as the default.
class NoneType : public ItemType
{
public:
    QString id() const override { return QStringLiteral("testmod.none"); }
    QString name() const override { return QStringLiteral("Beta none"); }
    QIcon icon() const override { return {}; }
};

void registerTestModule(ModuleRegistrar &registrar)
{
    registrar.addItemType(std::make_unique<ServerItemType>());
    registrar.addItemType(std::make_unique<AnyType>());
    registrar.addItemType(std::make_unique<NoneType>());
}

// A module with a server, whose base URL is "baseUrl" of its screen
// "general", and a type that takes any address and says which account it
// was read in.
class AccountServer : public mirante::ServerType
{
public:
    QString screenId() const override { return QStringLiteral("general"); }
    QString tokenKey() const override { return QStringLiteral("token"); }
    QUrl baseUrl(const QJsonObject &values) const override
    {
        return QUrl(values.value(QStringLiteral("baseUrl")).toString());
    }
    void authorize(QNetworkRequest &, const QString &, const QJsonObject &) const override {}
};

class AccountType : public ItemType
{
public:
    QString id() const override { return QStringLiteral("acc.item"); }
    QString name() const override { return QStringLiteral("Account item"); }
    QIcon icon() const override { return {}; }

    std::optional<UrlMatch> fromUrlForAccount(const QUrl &, const QString &, const QString &accountId) const override
    {
        UrlMatch match;
        match.config.insert(QStringLiteral("readIn"), accountId);
        return match;
    }
};

void registerAccountModule(ModuleRegistrar &registrar)
{
    registrar.setServerType(std::make_unique<AccountServer>());
    registrar.addItemType(std::make_unique<AccountType>());
}

} // namespace

class TestUrlMatches : public QObject
{
    Q_OBJECT

    ItemTypeRegistry types;

private slots:
    void initTestCase() { types.registerModule(QStringLiteral("testmod"), registerTestModule); }

    void webAddress()
    {
        QCOMPARE(urlmatches::webAddress(QStringLiteral("  https://example.com/a?b=1 \n")),
                 QUrl(QStringLiteral("https://example.com/a?b=1")));
        QCOMPARE(urlmatches::webAddress(QStringLiteral("http://intranet")), QUrl(QStringLiteral("http://intranet")));
        QVERIFY(urlmatches::webAddress(QString()).isEmpty());
        QVERIFY(urlmatches::webAddress(QStringLiteral("example.com")).isEmpty());
        QVERIFY(urlmatches::webAddress(QStringLiteral("ftp://example.com")).isEmpty());
        QVERIFY(urlmatches::webAddress(QStringLiteral("mailto:a@example.com")).isEmpty());
        QVERIFY(urlmatches::webAddress(QStringLiteral("https://a.com and more")).isEmpty());
        QVERIFY(urlmatches::webAddress(QStringLiteral("two\nhttps://lines.com")).isEmpty());
    }

    // Types of modules come first, then generic ones; the rest are left out.
    void order()
    {
        const QString work = QStringLiteral("work");
        const auto matches = urlmatches::match(types, nullptr, QUrl(QStringLiteral("https://srv.example/x/y")), work);
        QCOMPARE(matches.size(), size_t(2));
        QCOMPARE(matches.at(0).type->id(), QStringLiteral("testmod.server"));
        QCOMPARE(matches.at(0).match.config.value(QStringLiteral("path")).toString(), QStringLiteral("x/y"));
        QCOMPARE(matches.at(1).type->id(), QStringLiteral("testmod.any"));

        // An address of no known server leaves the generic type only.
        const auto other = urlmatches::match(types, nullptr, QUrl(QStringLiteral("https://other.example/")), work);
        QCOMPARE(other.size(), size_t(1));
        QCOMPARE(other.at(0).type->id(), QStringLiteral("testmod.any"));

        // No address, no matches.
        QVERIFY(urlmatches::match(types, nullptr, QUrl(), work).empty());
    }

    // A link is read in the first account whose server is on its host: the
    // default first, then the others by name. See decision 0146.
    void accountByHost()
    {
        ItemTypeRegistry registry;
        registry.registerModule(QStringLiteral("acc"), registerAccountModule);
        Model model;
        const QString profile = model.defaultProfile();
        const QString module = QStringLiteral("acc");
        model.setModuleSettings(profile, module, QStringLiteral("general"),
                                {{QStringLiteral("baseUrl"), QStringLiteral("https://github.com")}});
        Account bot;
        bot.id = QStringLiteral("bot");
        bot.name = QStringLiteral("Bot");
        bot.values.insert(QStringLiteral("baseUrl"), QStringLiteral("https://github.com"));
        Account enterprise;
        enterprise.id = QStringLiteral("ent");
        enterprise.name = QStringLiteral("Enterprise");
        enterprise.values.insert(QStringLiteral("baseUrl"), QStringLiteral("https://GHE.example/sub"));
        accounts::set(model, profile, module, bot);
        accounts::set(model, profile, module, enterprise);

        const auto accountOf = [&](const QString &url) {
            return urlmatches::accountFor(registry, model, profile, module, QUrl(url));
        };
        QCOMPARE(accountOf(QStringLiteral("https://github.com/a/b/pull/1")), QString());
        QCOMPARE(accountOf(QStringLiteral("https://ghe.example/a/b")), QStringLiteral("ent"));
        QCOMPARE(accountOf(QStringLiteral("https://elsewhere.example/")), QString());
        QCOMPARE(urlmatches::accountFor(registry, model, profile, QStringLiteral("none"),
                                        QUrl(QStringLiteral("https://ghe.example/"))),
                 QString());

        const auto matches = urlmatches::match(registry, &model, QUrl(QStringLiteral("https://ghe.example/x")), profile);
        QCOMPARE(matches.size(), size_t(1));
        QCOMPARE(matches.at(0).accountId, QStringLiteral("ent"));
        QCOMPARE(matches.at(0).match.config.value(QStringLiteral("readIn")).toString(), QStringLiteral("ent"));
        // Without a model, in the default account.
        QCOMPARE(urlmatches::match(registry, nullptr, QUrl(QStringLiteral("https://ghe.example/x")), profile)
                     .at(0)
                     .accountId,
                 QString());
    }

    // Each type is given the profile, and reads its own server there.
    void profileIsPassed()
    {
        const auto inDefault =
            urlmatches::match(types, nullptr, QUrl(QStringLiteral("https://srv.example/x/y")), kDefaultProfileId());
        QCOMPARE(lastProfile, QStringLiteral("default"));
        QCOMPARE(inDefault.size(), size_t(1));
    }

private:
    static QString kDefaultProfileId() { return QStringLiteral("default"); }
};

QTEST_MAIN(TestUrlMatches)
#include "tst_url_matches.moc"
