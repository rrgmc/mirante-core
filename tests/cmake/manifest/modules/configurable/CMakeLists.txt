# Declares build options, which the manifest's folder entry can set.
mirante_add_module(configurable
    API     ${MIRANTE_MODULE_API_VERSION}
    SOURCES Configurable.cpp
    OPTIONS MAX_ITEMS=25 COMPACT=OFF)

# The values reach the module's folder as variables, which run_test.cmake reads.
message(STATUS "configurable MAX_ITEMS=${MIRANTE_OPTION_MAX_ITEMS} COMPACT=${MIRANTE_OPTION_COMPACT}")
