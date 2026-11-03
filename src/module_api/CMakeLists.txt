# The only headers a module may include. See docs/modules.md#the-module-interface.
#
# This folder also configures on its own, so a module can compile and run its
# tests without the core: its project fetches mirante-core with SOURCE_SUBDIR
# src/module_api. See docs/modules.md#building-a-module-alone. The core sets
# MIRANTE_CORE_DIR, so this part runs only then.
if(NOT MIRANTE_CORE_DIR)
    cmake_minimum_required(VERSION 3.21)
    get_filename_component(_mirante_core_dir "${CMAKE_CURRENT_SOURCE_DIR}/../.." ABSOLUTE)
    set(MIRANTE_CORE_DIR "${_mirante_core_dir}" CACHE INTERNAL "The folder of the core")
    option(MIRANTE_WERROR "Treat compiler warnings as errors" OFF)
    option(MIRANTE_PCH "Compile with precompiled Qt and standard headers" OFF)
    # A module's project finds Qt itself, because the targets find_package()
    # makes here would not reach it. This is for configuring this folder alone.
    if(NOT TARGET Qt6::Core)
        project(mirante_module_api LANGUAGES CXX)
        set(CMAKE_CXX_STANDARD 17)
        set(CMAKE_CXX_STANDARD_REQUIRED ON)
        find_package(Qt6 6.8 REQUIRED COMPONENTS Core Gui Widgets Network Test)
        qt_standard_project_setup()
    endif()
    # A module's project can name its further Qt modules in MIRANTE_MODULE_QT.
    set(MIRANTE_QT_MODULES Core Gui Widgets Network Test ${MIRANTE_MODULE_QT} CACHE INTERNAL
        "The Qt modules that a module may name in QT_LIBS")
    include("${CMAKE_CURRENT_SOURCE_DIR}/cmake/MiranteTargets.cmake")
    include("${CMAKE_CURRENT_SOURCE_DIR}/cmake/MiranteModule.cmake")
endif()

add_library(mirante_module_api INTERFACE)
add_library(mirante::module_api ALIAS mirante_module_api)
target_include_directories(mirante_module_api INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}")
# Widgets for ItemEditor.h, Network for ApiClient.h.
target_link_libraries(mirante_module_api INTERFACE Qt6::Core Qt6::Gui Qt6::Widgets Qt6::Network)

add_subdirectory(widgets)
target_link_libraries(mirante_module_api INTERFACE mirante_module_widgets)

# Fakes of the module interface for module tests: <mirante/testing/...>.
# mirante_add_module() links it to a module's tests only, so module code
# cannot include it.
add_library(mirante_module_api_testing INTERFACE)
add_library(mirante::module_api_testing ALIAS mirante_module_api_testing)
target_include_directories(mirante_module_api_testing INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}/testing")
target_link_libraries(mirante_module_api_testing INTERFACE mirante::module_api)
