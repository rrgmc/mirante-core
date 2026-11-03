# Names a Qt module that the test's manifest does not list.
mirante_add_module(needsqt API ${MIRANTE_MODULE_API_VERSION} SOURCES ../listed/Listed.cpp QT_LIBS Pdf)
