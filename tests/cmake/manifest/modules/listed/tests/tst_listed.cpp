#include <QTest>

// Its moc file comes from the AUTOMOC that the core's scope turns on.
class TestListed : public QObject
{
    Q_OBJECT
private slots:
    void runs() { QVERIFY(true); }
};

QTEST_APPLESS_MAIN(TestListed)
#include "tst_listed.moc"
