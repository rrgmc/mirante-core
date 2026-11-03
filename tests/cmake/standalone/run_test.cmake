# Tests that src/module_api configures without the rest of the core, and that a
# module implementing only the required methods compiles and passes its tests.
#
#   cmake -DSOURCE=<this folder> -DBINARY=<a folder> -DGENERATOR=<generator>
#         -DCOMPILER=<compiler> -DPREFIX=<Qt> -P run_test.cmake
#
# It builds a project of its own, so on Windows it needs the Visual Studio
# environment, which tools/build.ps1 sets up. Without it the test is skipped.

if(WIN32 AND NOT DEFINED ENV{VCToolsInstallDir})
    message(STATUS "SKIPPED: this needs the Visual Studio environment")
    return()
endif()

file(REMOVE_RECURSE "${BINARY}")
execute_process(
    COMMAND "${CMAKE_COMMAND}" -S "${SOURCE}" -B "${BINARY}" -G "${GENERATOR}"
        "-DCMAKE_CXX_COMPILER=${COMPILER}" "-DCMAKE_PREFIX_PATH=${PREFIX}" -DCMAKE_BUILD_TYPE=Release
    RESULT_VARIABLE result OUTPUT_VARIABLE output ERROR_VARIABLE output)
if(NOT result EQUAL 0)
    message(FATAL_ERROR "Configuring failed:\n${output}")
endif()

execute_process(COMMAND "${CMAKE_COMMAND}" --build "${BINARY}"
    RESULT_VARIABLE result OUTPUT_VARIABLE output ERROR_VARIABLE output)
if(NOT result EQUAL 0)
    message(FATAL_ERROR "The module did not build:\n${output}")
endif()

execute_process(COMMAND "${CMAKE_CTEST_COMMAND}" --test-dir "${BINARY}" --output-on-failure
    RESULT_VARIABLE result OUTPUT_VARIABLE output ERROR_VARIABLE output)
if(NOT result EQUAL 0 OR NOT output MATCHES "module_minimal_tst_register")
    message(FATAL_ERROR "ctest did not pass the module's test:\n${output}")
endif()
