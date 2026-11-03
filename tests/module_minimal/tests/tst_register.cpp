// Registers the minimal module with the testing library alone, as a module in
// another repository does.
#include <mirante/ModuleApiVersion.h>
#include <mirante/testing/TestRegistrar.h>

#include <QTest>

namespace mirante::modules::minimal {
void registerModule(mirante::ModuleRegistrar &registrar);
}

class TestRegister : public QObject
{
    Q_OBJECT

private slots:
    void registersItsType()
    {
        mirante::testing::TestRegistrar registry;
        registry.registerModule(QStringLiteral("minimal"), &mirante::modules::minimal::registerModule);
        QVERIFY(registry.contains(QStringLiteral("minimal.item")));
    }

    void seesTheVersion() { QVERIFY(MIRANTE_MODULE_API_VERSION_MAJOR >= 0); }
};

QTEST_MAIN(TestRegister)
#include "tst_register.moc"
