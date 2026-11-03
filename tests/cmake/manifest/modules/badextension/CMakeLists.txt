# An extension point whose name is not in lowercase stops configuration.
mirante_add_module(badextension
    API        ${MIRANTE_MODULE_API_VERSION}
    SOURCES    ../extensible/Extensible.cpp
    EXTENSIONS Greeting=../extensible/DefaultGreeting.cpp)
