// Tests the other accounts of a module in a profile: how they are stored, read
// and removed, and which account an item uses. See decision 0144.
#include "core/Accounts.h"
#include "core/Model.h"

#include <QJsonArray>
#include <QTest>

using namespace mirante;

namespace {

const QString kWork = QStringLiteral("work");
const QString kModule = QStringLiteral("github");
const QString kServerScreen = QStringLiteral("general");

Account account(const QString &id, const QString &name, const QString &baseUrl)
{
    Account a;
    a.id = id;
    a.name = name;
    a.values.insert(QStringLiteral("baseUrl"), baseUrl);
    return a;
}

} // namespace

class TestAccounts : public QObject
{
    Q_OBJECT

private slots:
    void init()
    {
        m_model = std::make_unique<Model>();
        Profile work;
        work.id = kWork;
        work.name = QStringLiteral("Work");
        m_model->setProfile(work);
        m_model->setModuleSettings(kWork, kModule, kServerScreen,
                                   {{QStringLiteral("baseUrl"), QStringLiteral("https://github.com")}});
    }

    void storedInTheProfile()
    {
        accounts::set(*m_model, kWork, kModule, account(QStringLiteral("b"), QStringLiteral("Bot"),
                                                         QStringLiteral("https://github.com")));
        const Profile *profile = m_model->profile(kWork);
        QVERIFY(profile);
        const QJsonArray stored = profile->accounts.value(kModule).toArray();
        QCOMPARE(stored.size(), 1);
        QCOMPARE(stored.at(0).toObject().value(QStringLiteral("id")).toString(), QStringLiteral("b"));
        QCOMPARE(stored.at(0).toObject().value(QStringLiteral("name")).toString(), QStringLiteral("Bot"));

        // The profile's JSON keeps them, and reads them back.
        const std::optional<Profile> read = Profile::fromJson(profile->toJson());
        QVERIFY(read);
        QCOMPARE(read->accounts, profile->accounts);
        QVERIFY(read->extra.isEmpty());
    }

    void othersByNameSkippingBadEntries()
    {
        Profile profile = *m_model->profile(kWork);
        QJsonArray list;
        list.append(account(QStringLiteral("z"), QStringLiteral("zed"), QString()).toJson());
        list.append(QJsonObject{{QStringLiteral("name"), QStringLiteral("No id")}});
        list.append(account(QStringLiteral("a"), QStringLiteral("Alpha"), QString()).toJson());
        list.append(account(QStringLiteral("z"), QStringLiteral("Twice"), QString()).toJson());
        profile.accounts.insert(kModule, list);
        m_model->setProfile(profile);

        const std::vector<Account> others = accounts::others(*m_model, kWork, kModule);
        QCOMPARE(others.size(), size_t(2));
        QCOMPARE(others.at(0).id, QStringLiteral("a"));
        QCOMPARE(others.at(1).id, QStringLiteral("z"));
        QCOMPARE(others.at(1).name, QStringLiteral("zed"));
        QVERIFY(accounts::others(*m_model, kWork, QStringLiteral("gitlab")).empty());
        QVERIFY(accounts::others(*m_model, QStringLiteral("missing"), kModule).empty());
    }

    void unknownKeysAreKept()
    {
        QJsonObject json = account(QStringLiteral("b"), QStringLiteral("Bot"), QString()).toJson();
        json.insert(QStringLiteral("future"), 1);
        const std::optional<Account> read = Account::fromJson(json);
        QVERIFY(read);
        QCOMPARE(read->toJson(), json);
    }

    void screenValuesOfAnAccount()
    {
        accounts::set(*m_model, kWork, kModule, account(QStringLiteral("e"), QStringLiteral("Enterprise"),
                                                         QStringLiteral("https://ghe.example")));
        m_model->setModuleSettings(kWork, kModule, QStringLiteral("other"), {{QStringLiteral("x"), 1}});

        const auto baseUrl = [this](const QString &accountId, const QString &screenId) {
            return accounts::screenValues(*m_model, kWork, kModule, accountId, kServerScreen, screenId)
                .value(QStringLiteral("baseUrl"))
                .toString();
        };
        QCOMPARE(baseUrl(QString(), kServerScreen), QStringLiteral("https://github.com"));
        QCOMPARE(baseUrl(QStringLiteral("e"), kServerScreen), QStringLiteral("https://ghe.example"));
        // An account the profile does not have reads the default's values.
        QCOMPARE(baseUrl(QStringLiteral("gone"), kServerScreen), QStringLiteral("https://github.com"));
        // Other screens are the profile's, whatever the account.
        QCOMPARE(accounts::screenValues(*m_model, kWork, kModule, QStringLiteral("e"), kServerScreen,
                                        QStringLiteral("other"))
                     .value(QStringLiteral("x"))
                     .toInt(),
                 1);
    }

    void namesAndTheDefault()
    {
        accounts::set(*m_model, kWork, kModule, account(QStringLiteral("b"), QStringLiteral("Bot"), QString()));
        QCOMPARE(accounts::nameOf(*m_model, kWork, kModule, QStringLiteral("b")), QStringLiteral("Bot"));
        QCOMPARE(accounts::nameOf(*m_model, kWork, kModule, QString()), accounts::defaultName());
        QCOMPARE(accounts::nameOf(*m_model, kWork, kModule, QStringLiteral("gone")), accounts::defaultName());
    }

    void setReplacesAndRemoveDrops()
    {
        accounts::set(*m_model, kWork, kModule, account(QStringLiteral("b"), QStringLiteral("Bot"), QString()));
        accounts::set(*m_model, kWork, kModule, account(QStringLiteral("b"), QStringLiteral("Robot"), QString()));
        QCOMPARE(accounts::others(*m_model, kWork, kModule).size(), size_t(1));
        QCOMPARE(accounts::find(*m_model, kWork, kModule, QStringLiteral("b"))->name, QStringLiteral("Robot"));

        // Nothing happens for an unknown profile or an account without an id.
        accounts::set(*m_model, QStringLiteral("missing"), kModule, account(QStringLiteral("c"), {}, {}));
        accounts::set(*m_model, kWork, kModule, account(QString(), QStringLiteral("No id"), QString()));
        QCOMPARE(accounts::others(*m_model, kWork, kModule).size(), size_t(1));

        QVERIFY(!accounts::remove(*m_model, kWork, kModule, QStringLiteral("gone")));
        QVERIFY(accounts::remove(*m_model, kWork, kModule, QStringLiteral("b")));
        QVERIFY(accounts::others(*m_model, kWork, kModule).empty());
        // The module's entry goes with its last account.
        QVERIFY(m_model->profile(kWork)->accounts.isEmpty());
    }

    // An item uses the account it names when its profile has it, else the
    // default.
    void effectiveAccountOfAnItem()
    {
        accounts::set(*m_model, kWork, kModule, account(QStringLiteral("b"), QStringLiteral("Bot"), QString()));
        Group group;
        group.id = QStringLiteral("g");
        group.profile = kWork;
        m_model->setGroup(group);
        Project project;
        project.id = QStringLiteral("p");
        project.groupId = group.id;
        m_model->setProject(project);
        Task task;
        task.id = QStringLiteral("t");
        task.projectId = project.id;
        Item named;
        named.id = QStringLiteral("named");
        named.type = QStringLiteral("github.pr.summary");
        named.account = QStringLiteral("b");
        Item unknown = named;
        unknown.id = QStringLiteral("unknown");
        unknown.account = QStringLiteral("gone");
        Item plain = named;
        plain.id = QStringLiteral("plain");
        plain.account.clear();
        Item otherModule = named;
        otherModule.id = QStringLiteral("other");
        otherModule.type = QStringLiteral("gitlab.mr.summary");
        task.items = {named, unknown, plain, otherModule};
        m_model->setTask(task);

        QCOMPARE(accounts::effective(*m_model, task.id, named.id), QStringLiteral("b"));
        QCOMPARE(accounts::effective(*m_model, task.id, unknown.id), QString());
        QCOMPARE(accounts::effective(*m_model, task.id, plain.id), QString());
        // The id belongs to another module's accounts.
        QCOMPARE(accounts::effective(*m_model, task.id, otherModule.id), QString());
        QCOMPARE(accounts::effective(*m_model, task.id, QStringLiteral("missing")), QString());
    }

    void itemKeepsItsAccountInJson()
    {
        Item item;
        item.id = QStringLiteral("i");
        item.type = QStringLiteral("github.pr.summary");
        QVERIFY(!item.toJson().contains(QStringLiteral("account")));
        item.account = QStringLiteral("b");
        const std::optional<Item> read = Item::fromJson(item.toJson());
        QVERIFY(read);
        QCOMPARE(read->account, QStringLiteral("b"));
        QVERIFY(read->extra.isEmpty());
    }

private:
    std::unique_ptr<Model> m_model;
};

QTEST_MAIN(TestAccounts)
#include "tst_accounts.moc"
