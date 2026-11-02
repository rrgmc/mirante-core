# Login profiles and web panels. See docs/web-views.md.
mirante_add_library(mirante_web LIBS mirante_core Qt6::Widgets Qt6::Network Qt6::WebEngineWidgets Qt6::WebChannel)

# The built-in panels' pages, served as dash://panels/<path> from
# :/panels/<path>. See docs/web-views.md#built-in-panels.
file(GLOB_RECURSE panel_files CONFIGURE_DEPENDS
    "${MIRANTE_CORE_DIR}/resources/panels/*.html"
    "${MIRANTE_CORE_DIR}/resources/panels/*.js"
    "${MIRANTE_CORE_DIR}/resources/panels/*.css"
    "${MIRANTE_CORE_DIR}/resources/panels/*.svg")
qt_add_resources(mirante_web panels
    PREFIX /panels
    BASE "${MIRANTE_CORE_DIR}/resources/panels"
    FILES ${panel_files})
