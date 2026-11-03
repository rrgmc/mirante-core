cmake_minimum_required(VERSION 3.21)

# Lets CMAKE_MSVC_DEBUG_INFORMATION_FORMAT below take effect. It must come
# before project().
if(POLICY CMP0141)
    cmake_policy(SET CMP0141 NEW)
endif()

# The release workflow passes the tag's version, for example
# -DMIRANTE_VERSION=1.2.0. Raise the default after each release. See
# docs/decisions/0098-packages-from-cmake-install.md.
set(MIRANTE_VERSION "0.6.0" CACHE STRING "Mirante's version, as X.Y.Z")
if(NOT MIRANTE_VERSION MATCHES "^[0-9]+\.[0-9]+\.[0-9]+$")
    message(FATAL_ERROR "MIRANTE_VERSION is \"${MIRANTE_VERSION}\", not X.Y.Z")
endif()

project(mirante_core
    VERSION ${MIRANTE_VERSION}
    DESCRIPTION "A personal all-work desktop dashboard"
    HOMEPAGE_URL "https://github.com/rrgmc/mirante"
    LANGUAGES CXX)

# Functions of the core run in a module's scope too, where PROJECT_SOURCE_DIR
# may be the module's. An app fetches the core, so the core's folder is not the
# top folder either. See docs/decisions/0125-how-modules-are-fetched.md.
set(MIRANTE_CORE_DIR "${CMAKE_CURRENT_SOURCE_DIR}" CACHE INTERNAL "The folder of the core")
option(MIRANTE_CORE_TESTS "Build the core's own tests. Module tests are always built" ${PROJECT_IS_TOP_LEVEL})
set(MIRANTE_MANIFEST "" CACHE FILEPATH "The manifest of the modules to build. Empty builds no modules")

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Each worktree has its own build folder, since a CMake build folder belongs to
# one source folder. ccache, where it is installed, shares the compiling between
# them. It cannot cache MSVC's separate debug files, so objects hold their debug
# information. ccache caches files compiled with a precompiled header only
# with this sloppiness, which ccache 4.8 and later take on the command line.
find_program(MIRANTE_CCACHE ccache)
if(MIRANTE_CCACHE)
    message(STATUS "Compiling through ${MIRANTE_CCACHE}")
    set(CMAKE_CXX_COMPILER_LAUNCHER "${MIRANTE_CCACHE}"
        "sloppiness=pch_defines,time_macros,include_file_mtime,include_file_ctime")
    set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT "$<$<CONFIG:Debug,RelWithDebInfo>:Embedded>")
endif()

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake" "${CMAKE_CURRENT_SOURCE_DIR}/src/module_api/cmake")
include(MiranteManifest)

# What the core uses, and what the manifest adds for its modules.
mirante_manifest_qt_modules(manifest_qt)
set(MIRANTE_QT_MODULES Core Gui Widgets Network Svg WebChannel WebEngineWidgets Test LinguistTools ${manifest_qt}
    CACHE INTERNAL "The Qt modules that a module may name in QT_LIBS")
find_package(Qt6 6.8 REQUIRED COMPONENTS ${MIRANTE_QT_MODULES})
# The source texts are English. Brazilian Portuguese is the one translation.
# See docs/decisions/0086-english-and-brazilian-portuguese.md.
qt_standard_project_setup(I18N_SOURCE_LANGUAGE en I18N_TRANSLATED_LANGUAGES pt_BR)

# Qt 6.8's FindWrapOpenGL links the AGL framework, which Xcode 26 no longer
# ships. Nothing here uses AGL, so drop it.
if(APPLE AND TARGET WrapOpenGL::WrapOpenGL)
    get_target_property(_gl_libs WrapOpenGL::WrapOpenGL INTERFACE_LINK_LIBRARIES)
    list(FILTER _gl_libs EXCLUDE REGEX "AGL")
    set_target_properties(WrapOpenGL::WrapOpenGL PROPERTIES INTERFACE_LINK_LIBRARIES "${_gl_libs}")
endif()

option(MIRANTE_WERROR "Treat compiler warnings as errors" OFF)
option(MIRANTE_PCH "Compile with precompiled Qt and standard headers" ON)
set(MIRANTE_MACOS_SIGN_IDENTITY "$ENV{MIRANTE_MACOS_SIGN_IDENTITY}" CACHE STRING
    "macOS: the certificate that signs the app bundles. Empty uses MIRANTE_MACOS_DEFAULT_SIGN_IDENTITY when the keychain has it")
# The maintainer's development certificate. A certificate's name is not a
# secret. The build uses it only where the keychain has it, so other Macs still
# build with an ad hoc signature. See decision 0091.
set(MIRANTE_MACOS_DEFAULT_SIGN_IDENTITY "Apple Development: Rangel Reale (B6EDST9Q92)")
set(MIRANTE_MACOS_SIGN_IDENTITY_USED "${MIRANTE_MACOS_SIGN_IDENTITY}")
if(APPLE AND NOT MIRANTE_MACOS_SIGN_IDENTITY_USED)
    execute_process(COMMAND security find-identity -v -p codesigning
        OUTPUT_VARIABLE _identities ERROR_QUIET)
    string(FIND "${_identities}" "\"${MIRANTE_MACOS_DEFAULT_SIGN_IDENTITY}\"" _found)
    if(_found GREATER -1)
        set(MIRANTE_MACOS_SIGN_IDENTITY_USED "${MIRANTE_MACOS_DEFAULT_SIGN_IDENTITY}")
    endif()
endif()
if(APPLE)
    if(MIRANTE_MACOS_SIGN_IDENTITY_USED)
        message(STATUS "Signing the app bundles with ${MIRANTE_MACOS_SIGN_IDENTITY_USED}")
    else()
        message(STATUS "Signing the app bundles ad hoc")
    endif()
endif()

include(MiranteTargets)
include(Dependencies)
include(MiranteModule)
include(MiranteDeploy)
include(MiranteInstall)

enable_testing()

configure_file(cmake/Version.h.in "${CMAKE_CURRENT_BINARY_DIR}/generated/Version.h" @ONLY)

add_subdirectory(src)
if(MIRANTE_CORE_TESTS)
    add_subdirectory(tests)
endif()
