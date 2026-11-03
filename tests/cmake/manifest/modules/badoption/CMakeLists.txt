# An option whose name is not in capitals stops configuration.
mirante_add_module(badoption
    API     ${MIRANTE_MODULE_API_VERSION}
    SOURCES ../configurable/Configurable.cpp
    OPTIONS max_items=25)
