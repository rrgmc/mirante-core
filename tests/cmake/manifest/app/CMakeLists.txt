# What an app repository's CMakeLists.txt holds, with the manifest given by the
# test. See ../run_test.cmake. LANGUAGES NONE leaves the first project() with a
# language to the core, whose policies must be set before it.
cmake_minimum_required(VERSION 3.21)
project(mirante LANGUAGES NONE)
enable_testing()

set(MIRANTE_MANIFEST "${TEST_MANIFEST}")
file(READ "${MIRANTE_MANIFEST}" manifest)
string(JSON core_repo GET "${manifest}" core repo)
string(JSON core_ref GET "${manifest}" core ref)

include(FetchContent)
FetchContent_Declare(mirante_core GIT_REPOSITORY "${core_repo}" GIT_TAG "${core_ref}")
FetchContent_MakeAvailable(mirante_core)
