#include <mirante/Module.h>

#include <QIcon>

#include <memory>

namespace mirante::modules::minimal {

namespace {

class MinimalItemType : public ItemType
{
public:
    QString id() const override { return QStringLiteral("minimal.item"); }
    QString name() const override { return QStringLiteral("Minimal item"); }
    QIcon icon() const override { return QIcon(); }
};

} // namespace

void registerModule(ModuleRegistrar &registrar)
{
    registrar.addItemType(std::make_unique<MinimalItemType>());
}

} // namespace mirante::modules::minimal
