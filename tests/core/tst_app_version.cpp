// The version that the About dialog shows, with and without an edition.
#include "core/AppVersion.h"

#include <QTest>

class TestAppVersion : public QObject
{
    Q_OBJECT

private slots:
    void withoutEdition() { QCOMPARE(mirante::displayVersion(QStringLiteral("0.1.0"), QString()), QStringLiteral("0.1.0")); }

    void withEdition()
    {
        QCOMPARE(mirante::displayVersion(QStringLiteral("0.1.0"), QStringLiteral("work")),
                 QStringLiteral("0.1.0 (work)"));
    }
};

QTEST_APPLESS_MAIN(TestAppVersion)
#include "tst_app_version.moc"
