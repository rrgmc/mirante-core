# Overrides a method without saying override, so it must not compile on GCC and
# Clang.
mirante_add_module(delta API ${MIRANTE_MODULE_API_VERSION} SOURCES Delta.cpp)
