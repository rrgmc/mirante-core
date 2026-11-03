# What a module's own repository holds to compile and test the module without the
# core: it fetches mirante-core with SOURCE_SUBDIR src/module_api. See
# run_test.cmake. The module here is tests/module_minimal.
cmake_minimum_required(VERSION 3.21)
project(MiranteStandaloneTest LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
find_package(Qt6 6.8 REQUIRED COMPONENTS Core Gui Widgets Network Test)
qt_standard_project_setup()
enable_testing()

get_filename_component(repo "${CMAKE_CURRENT_SOURCE_DIR}/../../.." ABSOLUTE)
include(FetchContent)
FetchContent_Declare(mirante_core SOURCE_DIR "${repo}" SOURCE_SUBDIR src/module_api)
FetchContent_MakeAvailable(mirante_core)

add_subdirectory("${repo}/tests/module_minimal" module_minimal)
