# DEFAULT OFF has no effect on a folder that the manifest lists.
mirante_add_module(listed
    API     ${MIRANTE_MODULE_API_VERSION}
    SOURCES Listed.cpp
    TESTS   tests/tst_listed.cpp
    DEFAULT OFF)
