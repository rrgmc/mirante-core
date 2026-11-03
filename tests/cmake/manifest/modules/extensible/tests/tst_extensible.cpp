#include <QFile>
#include <QTest>

namespace mirante::modules::extensible {
const char *greeting();
}

// run_test.cmake says which greeting to expect, and whether the extension's
// resource is compiled in.
class TestExtensible : public QObject
{
    Q_OBJECT
private slots:
    void hasTheExpectedGreeting()
    {
        QCOMPARE(QString::fromLatin1(mirante::modules::extensible::greeting()),
                 qEnvironmentVariable("EXPECTED_GREETING"));
    }

    void hasTheExtensionsResource()
    {
        QCOMPARE(QFile::exists(QStringLiteral(":/modules/extensible/hello.txt")),
                 qEnvironmentVariable("EXPECTED_RESOURCE") == QLatin1String("1"));
    }
};

QTEST_APPLESS_MAIN(TestExtensible)
#include "tst_extensible.moc"
