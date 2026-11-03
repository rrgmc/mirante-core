# A module that depends on a module of another repository, test_modules.
mirante_add_module(uses
    API     ${MIRANTE_MODULE_API_VERSION}
    SOURCES Uses.cpp
    TESTS   tests/tst_uses.cpp
    DEPENDS listed)
