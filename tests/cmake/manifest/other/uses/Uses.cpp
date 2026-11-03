// Includes and calls the module listed, from another repository, by file name.
#include "Listed.h"

namespace mirante {
class ModuleRegistrar;
namespace modules::uses {
void registerModule(ModuleRegistrar &) {}

int usesListed() { return listed::answer(); }
}
}
