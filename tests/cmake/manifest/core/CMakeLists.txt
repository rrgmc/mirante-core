# Stands in for the core: the real manifest and module code, in the same order,
# with a stub of the module interface. Qt is found here, as in the core, so its
# targets are visible only from here down.
cmake_minimum_required(VERSION 3.21)
project(mirante_core LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
get_filename_component(repo "${CMAKE_CURRENT_SOURCE_DIR}/../../../.." ABSOLUTE)
list(APPEND CMAKE_MODULE_PATH "${repo}/cmake" "${repo}/src/module_api/cmake")
include(MiranteManifest)

mirante_manifest_qt_modules(manifest_qt)
set(MIRANTE_QT_MODULES Core Gui Widgets Test ${manifest_qt} CACHE INTERNAL "")
find_package(Qt6 6.8 REQUIRED COMPONENTS ${MIRANTE_QT_MODULES})
qt_standard_project_setup()

set(MIRANTE_PCH OFF)
set(MIRANTE_CORE_DIR "${CMAKE_CURRENT_SOURCE_DIR}" CACHE INTERNAL "")
include(MiranteTargets)
include(MiranteModule)

add_library(mirante_module_api INTERFACE)
add_library(mirante::module_api ALIAS mirante_module_api)
target_link_libraries(mirante_module_api INTERFACE Qt6::Core)
add_library(mirante_module_api_testing INTERFACE)
add_library(mirante::module_api_testing ALIAS mirante_module_api_testing)

mirante_add_manifest_modules()

add_executable(manifest_app main.cpp)
mirante_finish_modules(manifest_app)
