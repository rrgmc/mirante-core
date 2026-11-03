// The options reach the code as compile definitions, as texts.
#ifndef MIRANTE_OPTION_MAX_ITEMS
#error "MIRANTE_OPTION_MAX_ITEMS is not defined"
#endif
#ifndef MIRANTE_OPTION_COMPACT
#error "MIRANTE_OPTION_COMPACT is not defined"
#endif

namespace mirante {
class ModuleRegistrar;
namespace modules::configurable {
const char *maxItems() { return MIRANTE_OPTION_MAX_ITEMS; }
void registerModule(ModuleRegistrar &) {}
}
}
