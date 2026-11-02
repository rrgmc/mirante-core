#include <QString>

namespace mirante {
class ModuleRegistrar;
namespace modules::listed {
void registerModule(ModuleRegistrar &)
{
    // Uses Qt, which a module reaches only from the core's scope.
    (void)QString();
}
}
}
