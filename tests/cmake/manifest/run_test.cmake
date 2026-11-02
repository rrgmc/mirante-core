# Tests the manifest with an app as the top project, as in an app repository:
# only listed folders run, a repository's top file does not, modules see Qt and
# AUTOMOC from the core's scope, and ctest in the top folder finds their tests.
# Mistakes in the manifest stop configuration. Nothing is downloaded: every
# repository comes from a folder here, through FETCHCONTENT_SOURCE_DIR_<NAME>.
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

# Writes a manifest with these entries in "modules" and configures the app.
function(configure_app name modules_json)
    set(binary "${BINARY}/${name}")
    file(REMOVE_RECURSE "${binary}")
    set(manifest "${BINARY}/${name}.json")
    file(WRITE "${manifest}" "{
  \"qt\": [],
  \"core\": { \"repo\": \"unused\", \"ref\": \"unused\" },
  \"modules\": ${modules_json}
}")
    execute_process(
        COMMAND "${CMAKE_COMMAND}" -S "${SOURCE}/app" -B "${binary}" -G "${GENERATOR}"
            "-DCMAKE_CXX_COMPILER=${COMPILER}" "-DCMAKE_PREFIX_PATH=${PREFIX}" -DCMAKE_BUILD_TYPE=Release
            "-DTEST_MANIFEST=${manifest}"
            "-DFETCHCONTENT_SOURCE_DIR_MIRANTE_CORE=${SOURCE}/core"
            "-DFETCHCONTENT_SOURCE_DIR_TEST_MODULES=${SOURCE}/modules"
            "-DFETCHCONTENT_SOURCE_DIR_TEST_SINGLE=${SOURCE}/single"
        RESULT_VARIABLE result OUTPUT_VARIABLE output ERROR_VARIABLE output)
    set(result "${result}" PARENT_SCOPE)
    set(output "${output}" PARENT_SCOPE)
endfunction()

# Configuration must stop, with this message.
function(expect_refusal name modules_json expected)
    configure_app(${name} "${modules_json}")
    if(result EQUAL 0)
        message(FATAL_ERROR "${name}: configuration did not stop.")
    endif()
    if(NOT output MATCHES "${expected}")
        message(FATAL_ERROR "${name}: the wrong message:\n${output}")
    endif()
endfunction()

set(modules_repo "\"name\": \"test_modules\", \"repo\": \"unused\", \"ref\": \"unused\"")
set(single_repo "\"name\": \"test_single\", \"repo\": \"unused\", \"ref\": \"unused\"")

configure_app(good "[
    { ${modules_repo}, \"folders\": [\"listed\"] },
    { ${single_repo}, \"folders\": [\".\"] }
  ]")
if(NOT result EQUAL 0)
    message(FATAL_ERROR "Configuring failed:\n${output}")
endif()
if(NOT output MATCHES "Module listed: on" OR NOT output MATCHES "Module single: on")
    message(FATAL_ERROR "A listed module is not on:\n${output}")
endif()
execute_process(COMMAND "${CMAKE_COMMAND}" --build "${BINARY}/good"
    RESULT_VARIABLE result OUTPUT_VARIABLE output ERROR_VARIABLE output)
if(NOT result EQUAL 0)
    message(FATAL_ERROR "The app did not build:\n${output}")
endif()

# ctest in the top folder runs the tests of a fetched module.
execute_process(COMMAND "${CMAKE_CTEST_COMMAND}" --test-dir "${BINARY}/good" --output-on-failure
    RESULT_VARIABLE result OUTPUT_VARIABLE output ERROR_VARIABLE output)
if(NOT result EQUAL 0 OR NOT output MATCHES "module_listed_tst_listed")
    message(FATAL_ERROR "ctest did not run the module's test:\n${output}")
endif()

expect_refusal(unknown_field "[{ ${modules_repo}, \"folder\": [\"listed\"], \"folders\": [\"listed\"] }]"
    "unknown field \"folder\"")
expect_refusal(missing_field "[{ \"name\": \"test_modules\", \"repo\": \"unused\", \"folders\": [\"listed\"] }]"
    "no field \"ref\"")
expect_refusal(no_folders "[{ ${modules_repo}, \"folders\": [] }]" "lists no folders")
expect_refusal(no_such_folder "[{ ${modules_repo}, \"folders\": [\"missing\"] }]"
    "folder \"missing\", which has no CMakeLists.txt")
expect_refusal(no_module "[{ ${modules_repo}, \"folders\": [\"empty\"] }]"
    "folder \"empty\", which adds no module")
expect_refusal(bad_name "[{ \"name\": \"Bad-Name\", \"repo\": \"unused\", \"ref\": \"unused\", \"folders\": [\"listed\"] }]"
    "name \"Bad-Name\"")
