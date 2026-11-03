# The module mechanism. See docs/modules.md#building.
#
# Each module folder calls mirante_add_module(). The top level then calls
# mirante_finish_modules() once, which checks the module list and generates the
# registration code. The module lists are kept in global properties:
#
#   MIRANTE_ALL_MODULES          every module folder, enabled or not
#   MIRANTE_ENABLED_MODULES      the enabled modules
#   MIRANTE_MODULE_DEPENDS_<id>  the DEPENDS of one module
#   MIRANTE_MODULE_REPOSITORY_<id>, MIRANTE_MODULE_COMMIT_<id>
#                                where one module came from, set by the manifest

# Warnings in a module fail the build only where the module's owner builds it.
# An app leaves this off, so a newer compiler cannot stop someone else's module.
option(MIRANTE_MODULES_WERROR "Treat compiler warnings in modules as errors" OFF)

# The version of the module interface, from the header that modules see. See
# docs/decisions/0119-module-interface-versions-and-checks.md.
set(_mirante_api_header "${CMAKE_CURRENT_LIST_DIR}/../mirante/ModuleApiVersion.h")
set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS "${_mirante_api_header}")
file(STRINGS "${_mirante_api_header}" _mirante_api_major REGEX "define MIRANTE_MODULE_API_VERSION_MAJOR")
file(STRINGS "${_mirante_api_header}" _mirante_api_minor REGEX "define MIRANTE_MODULE_API_VERSION_MINOR")
string(REGEX REPLACE ".* ([0-9]+)$" "\\1" _mirante_api_major "${_mirante_api_major}")
string(REGEX REPLACE ".* ([0-9]+)$" "\\1" _mirante_api_minor "${_mirante_api_minor}")
set(MIRANTE_MODULE_API_VERSION "${_mirante_api_major}.${_mirante_api_minor}" CACHE INTERNAL
    "The version of the module interface")

# Stops configuration if a module needs another version of the interface. While
# the interface is 0.x, any minor version may break modules, so it must match
# exactly. From 1.0, a minor version only adds things.
function(_mirante_check_api_version id wanted)
    if(NOT wanted MATCHES "^([0-9]+)\\.([0-9]+)$")
        message(FATAL_ERROR "Module ${id}: API is \"${wanted}\", not major.minor.")
    endif()
    set(wanted_major "${CMAKE_MATCH_1}")
    set(wanted_minor "${CMAKE_MATCH_2}")
    string(REPLACE "." ";" have "${MIRANTE_MODULE_API_VERSION}")
    list(GET have 0 have_major)
    list(GET have 1 have_minor)
    if(have_major EQUAL 0)
        set(fits OFF)
        if(wanted_major EQUAL 0 AND wanted_minor EQUAL have_minor)
            set(fits ON)
        endif()
    elseif(wanted_major EQUAL have_major AND NOT wanted_minor GREATER have_minor)
        set(fits ON)
    else()
        set(fits OFF)
    endif()
    if(NOT fits STREQUAL "ON")
        message(FATAL_ERROR "Module ${id} needs version ${wanted} of the module interface, "
            "but this core has version ${MIRANTE_MODULE_API_VERSION}. "
            "See src/module_api/CHANGELOG.md in mirante-core for what changed.")
    endif()
endfunction()

# Reads a module's OPTIONS, NAME=default each, and the manifest's values for
# them, a JSON object or empty. Sets option_names and option_value_<NAME> in the
# caller's scope.
function(_mirante_module_options id declared manifest_values)
    set(names "")
    foreach(declaration IN LISTS declared)
        if(NOT declaration MATCHES "^([A-Z][A-Z0-9_]*)=(.*)$")
            message(FATAL_ERROR "Module ${id}: the option \"${declaration}\" must be NAME=default, "
                "with a name of capital letters, digits and underscores.")
        endif()
        list(APPEND names "${CMAKE_MATCH_1}")
        set(value_${CMAKE_MATCH_1} "${CMAKE_MATCH_2}")
    endforeach()
    if(manifest_values)
        string(JSON count LENGTH "${manifest_values}")
        if(count GREATER 0)
            math(EXPR last "${count} - 1")
            foreach(index RANGE ${last})
                string(JSON name MEMBER "${manifest_values}" ${index})
                if(NOT name IN_LIST names)
                    set(takes "none")
                    if(names)
                        string(REPLACE ";" ", " takes "${names}")
                    endif()
                    message(FATAL_ERROR "${MIRANTE_MANIFEST}: module ${id} has no option \"${name}\". "
                        "Its options are: ${takes}")
                endif()
                string(JSON type TYPE "${manifest_values}" "${name}")
                if(NOT type STREQUAL "STRING")
                    message(FATAL_ERROR "${MIRANTE_MANIFEST}: the option ${name} of module ${id} "
                        "must be a text, such as \"50\".")
                endif()
                string(JSON value_${name} GET "${manifest_values}" "${name}")
            endforeach()
        endif()
    endif()
    set(option_names "${names}" PARENT_SCOPE)
    foreach(name IN LISTS names)
        set(option_value_${name} "${value_${name}}" PARENT_SCOPE)
    endforeach()
endfunction()

# mirante_add_module(<id>
#     API       <major.minor>       the version of the module interface it needs
#     SOURCES   <files...>
#     TESTS     <files...>
#     RESOURCES <files...>
#     DEPENDS   <module ids...>     links them and makes their headers available
#     QT_LIBS   <Qt components...>
#     OPTIONS   <NAME=default...>   build options that the manifest can set
#     DEFAULT   ON|OFF)
#
# QT_LIBS links further Qt components, such as PdfWidgets for the document
# module. Each must be in MIRANTE_QT_MODULES: what the core uses, and the "qt"
# list of the manifest.
#
# OPTIONS declares the module's build options. The "options" of its folder entry
# in the manifest replace the defaults, and an option the module does not
# declare stops configuration. Each option becomes the variable
# MIRANTE_OPTION_<NAME> in the module's folder and the compile definition
# MIRANTE_OPTION_<NAME>="<value>" on the module and its tests. See
# docs/decisions/0129-folder-entries-options-and-optional-modules.md.
function(mirante_add_module id)
    cmake_parse_arguments(ARG "" "DEFAULT;API" "SOURCES;TESTS;RESOURCES;DEPENDS;QT_LIBS;OPTIONS" ${ARGN})
    if(ARG_UNPARSED_ARGUMENTS)
        message(FATAL_ERROR "Module ${id}: unknown arguments: ${ARG_UNPARSED_ARGUMENTS}")
    endif()
    if(NOT ARG_API)
        message(FATAL_ERROR "Module ${id} states no version of the module interface. "
            "Add API ${MIRANTE_MODULE_API_VERSION} to its mirante_add_module().")
    endif()
    _mirante_check_api_version(${id} "${ARG_API}")
    foreach(qt_lib IN LISTS ARG_QT_LIBS)
        if(NOT qt_lib IN_LIST MIRANTE_QT_MODULES)
            message(FATAL_ERROR "Module ${id} needs the Qt module ${qt_lib}, which the manifest does not list. "
                "Add \"${qt_lib}\" to its \"qt\" list.")
        endif()
    endforeach()

    if(NOT id MATCHES "^[a-z0-9]+$")
        message(FATAL_ERROR "Module id \"${id}\" must have lowercase letters and digits only.")
    endif()
    if(id STREQUAL "dash")
        message(FATAL_ERROR "Module id \"dash\" is reserved for the core.")
    endif()
    get_property(all_modules GLOBAL PROPERTY MIRANTE_ALL_MODULES)
    if(id IN_LIST all_modules)
        message(FATAL_ERROR "Module ${id} is added twice.")
    endif()
    if(NOT ARG_SOURCES)
        message(FATAL_ERROR "Module ${id} has no SOURCES.")
    endif()
    if(NOT DEFINED ARG_DEFAULT)
        set(ARG_DEFAULT ON)
    endif()

    set_property(GLOBAL APPEND PROPERTY MIRANTE_ALL_MODULES ${id})
    set_property(GLOBAL PROPERTY MIRANTE_MODULE_DEPENDS_${id} ${ARG_DEPENDS})

    # The options, checked whether the module is on or off, so a mistake in the
    # manifest shows at once.
    _mirante_module_options(${id} "${ARG_OPTIONS}" "${MIRANTE_LISTED_OPTIONS}")
    foreach(option IN LISTS option_names)
        set(MIRANTE_OPTION_${option} "${option_value_${option}}" PARENT_SCOPE)
    endforeach()

    # MIRANTE_MODULES, when set, is the exact list of enabled modules.
    string(TOUPPER "${id}" upper_id)
    set(option_name MIRANTE_MODULE_${upper_id})
    set(option_help "Build the ${id} module")
    if(DEFINED MIRANTE_MODULES)
        if(id IN_LIST MIRANTE_MODULES)
            set(${option_name} ON CACHE BOOL "${option_help}" FORCE)
        else()
            set(${option_name} OFF CACHE BOOL "${option_help}" FORCE)
        endif()
    elseif(MIRANTE_LISTED_MODULE AND MIRANTE_LISTED_OPTIONAL)
        # Listed but optional: off unless the build turns it on.
        option(${option_name} "${option_help}" OFF)
    elseif(MIRANTE_LISTED_MODULE)
        set(${option_name} ON)
    else()
        option(${option_name} "${option_help}" ${ARG_DEFAULT})
    endif()

    if(NOT ${option_name})
        message(STATUS "Module ${id}: off")
        return()
    endif()
    message(STATUS "Module ${id}: on")
    set_property(GLOBAL APPEND PROPERTY MIRANTE_ENABLED_MODULES ${id})

    # Only the module interface and Qt. src/ is never on the include path, so a
    # module cannot include core headers.
    set(target mirante_module_${id})
    # What a module that lists this one in DEPENDS gets: its folder on the
    # include path. mirante_finish_modules() links it. The app does not get it.
    add_library(${target}_headers INTERFACE)
    target_include_directories(${target}_headers INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}")
    add_library(${target} STATIC ${ARG_SOURCES})
    target_link_libraries(${target} PUBLIC mirante::module_api Qt6::Core Qt6::Gui Qt6::Widgets)
    foreach(qt_lib IN LISTS ARG_QT_LIBS)
        target_link_libraries(${target} PUBLIC Qt6::${qt_lib})
    endforeach()
    # PUBLIC, so the module's tests see the same values.
    foreach(option IN LISTS option_names)
        target_compile_definitions(${target} PUBLIC "MIRANTE_OPTION_${option}=\"${option_value_${option}}\"")
    endforeach()
    set(no_werror NO_WERROR)
    if(MIRANTE_MODULES_WERROR)
        set(no_werror "")
    endif()
    mirante_set_warnings(${target} ${no_werror})
    # Without override, a changed signature in the interface leaves a method that
    # still compiles but is never called, so this is an error in every build.
    if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        target_compile_options(${target} PRIVATE -Werror=suggest-override)
    endif()
    mirante_precompile_headers(${target})
    mirante_autogen_tracks_sources(${target})

    # Qt 6 links the resources of a static library as object files, so no
    # Q_INIT_RESOURCE call is needed.
    if(ARG_RESOURCES)
        qt_add_resources(${target} ${target}_resources
            PREFIX "/modules/${id}"
            FILES ${ARG_RESOURCES})
    endif()

    # A module with a folder translations/ carries its own texts, as
    # :/i18n/<id>_<language>, which main.cpp loads. update_translations runs
    # lupdate on them, on this module's sources only: without SOURCE_TARGETS it
    # reads every target in the build. See translations/README.md in mirante-core.
    if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/translations" AND QT_I18N_TRANSLATED_LANGUAGES)
        qt_add_translations(${target}
            SOURCE_TARGETS ${target}
            TS_FILE_BASE ${id}
            TS_FILE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/translations"
            RESOURCE_PREFIX /i18n
            LUPDATE_OPTIONS -locations none -no-obsolete)
    endif()

    # The module's tests link the same libraries, so they share the first
    # test's precompiled header.
    set(pch_from "")
    foreach(test_source IN LISTS ARG_TESTS)
        get_filename_component(test_name "${test_source}" NAME_WE)
        mirante_add_test(module_${id}_${test_name}
            SOURCES "${test_source}"
            LIBS ${target} mirante::module_api_testing
            PCH_FROM ${pch_from}
            ${no_werror})
        if(NOT pch_from)
            set(pch_from "module_${id}_${test_name}")
        endif()
    endforeach()
endfunction()

# mirante_finish_modules(<executable>)
#
# Called once, after every module folder is added. Checks MIRANTE_MODULES and
# DEPENDS, generates enabled_modules.cpp from enabled_modules.cpp.in, and adds it
# and the module libraries to <executable>.
function(mirante_finish_modules target)
    get_property(all_modules GLOBAL PROPERTY MIRANTE_ALL_MODULES)
    get_property(enabled GLOBAL PROPERTY MIRANTE_ENABLED_MODULES)

    if(DEFINED MIRANTE_MODULES)
        foreach(id IN LISTS MIRANTE_MODULES)
            if(NOT id IN_LIST all_modules)
                message(FATAL_ERROR "MIRANTE_MODULES names the module \"${id}\", which does not exist. "
                    "The modules are: ${all_modules}")
            endif()
        endforeach()
    endif()

    foreach(id IN LISTS enabled)
        get_property(depends GLOBAL PROPERTY MIRANTE_MODULE_DEPENDS_${id})
        foreach(dependency IN LISTS depends)
            if(NOT dependency IN_LIST all_modules)
                message(FATAL_ERROR "Module ${id} depends on the module \"${dependency}\", which does not exist.")
            endif()
            if(NOT dependency IN_LIST enabled)
                message(FATAL_ERROR "Module ${id} depends on the module ${dependency}, which is disabled. "
                    "Enable ${dependency} or disable ${id}.")
            endif()
            # After the check, since the modules' folders are added in any order.
            target_link_libraries(mirante_module_${id} PUBLIC
                mirante_module_${dependency} mirante_module_${dependency}_headers)
        endforeach()
    endforeach()

    list(SORT enabled)
    set(MIRANTE_MODULE_DECLARATIONS "")
    set(MIRANTE_MODULE_CALLS "")
    set(MIRANTE_MODULE_REFERENCES "")
    set(MIRANTE_MODULE_SOURCES "")
    set(ids "")
    foreach(id IN LISTS enabled)
        get_property(repository GLOBAL PROPERTY MIRANTE_MODULE_REPOSITORY_${id})
        get_property(commit GLOBAL PROPERTY MIRANTE_MODULE_COMMIT_${id})
        string(APPEND MIRANTE_MODULE_SOURCES
            "        {QStringLiteral(\"${id}\"), QStringLiteral(\"${repository}\"), QStringLiteral(\"${commit}\")},\n")
        string(APPEND MIRANTE_MODULE_DECLARATIONS
            "namespace mirante::modules::${id} {\n"
            "void registerModule(mirante::ModuleRegistrar &registrar);\n"
            "}\n")
        string(APPEND MIRANTE_MODULE_CALLS
            "    registry.registerModule(QStringLiteral(\"${id}\"), &mirante::modules::${id}::registerModule);\n")
        string(APPEND MIRANTE_MODULE_REFERENCES
            "    reinterpret_cast<void *>(&mirante::modules::${id}::registerModule),\n")
        list(APPEND ids "QStringLiteral(\"${id}\")")
        target_link_libraries(${target} PRIVATE mirante_module_${id})
    endforeach()
    list(JOIN ids ", " MIRANTE_MODULE_IDS)

    set(generated "${PROJECT_BINARY_DIR}/generated/enabled_modules.cpp")
    configure_file("${MIRANTE_CORE_DIR}/cmake/enabled_modules.cpp.in" "${generated}" @ONLY)
    target_sources(${target} PRIVATE "${generated}")
endfunction()
