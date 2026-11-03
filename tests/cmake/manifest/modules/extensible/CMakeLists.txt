# An extension point, greeting, with a default that an app can replace.
mirante_add_module(extensible
    API        ${MIRANTE_MODULE_API_VERSION}
    SOURCES    Extensible.cpp Greeting.h
    EXTENSIONS greeting=DefaultGreeting.cpp
    TESTS      tests/tst_extensible.cpp)
