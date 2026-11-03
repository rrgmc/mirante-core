#include <QTest>

namespace mirante::modules::uses {
int usesListed();
}

// Links only because DEPENDS links the module listed of the other repository.
class TestUses : public QObject
{
    Q_OBJECT
private slots:
    void reachesTheOtherRepository() { QCOMPARE(mirante::modules::uses::usesListed(), 42); }
};

QTEST_APPLESS_MAIN(TestUses)
#include "tst_uses.moc"
