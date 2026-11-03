# A module that implements only the required methods, and uses no default of the
# interface on purpose. Keep it unchanged as the interface grows: a change that
# needs a change here breaks every module. See decision 0119.
mirante_add_module(minimal
    API     ${MIRANTE_MODULE_API_VERSION}
    SOURCES MinimalModule.cpp
    TESTS   tests/tst_register.cpp)
