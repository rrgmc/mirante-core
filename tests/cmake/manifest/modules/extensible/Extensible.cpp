#include "Greeting.h"

namespace mirante {
class ModuleRegistrar;
namespace modules::extensible {
void registerModule(ModuleRegistrar &) { (void)greeting(); }
}
}
