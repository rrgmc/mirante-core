# The manifest of an app repository: which modules to fetch and build. See
# docs/decisions/0125-how-modules-are-fetched.md.

include(FetchContent)

# mirante_manifest_qt_modules(<out>)
#
# Reads the "qt" list of MIRANTE_MANIFEST, which comes before find_package(Qt6).
function(mirante_manifest_qt_modules out)
    set(modules "")
    if(MIRANTE_MANIFEST)
        if(NOT EXISTS "${MIRANTE_MANIFEST}")
            message(FATAL_ERROR "The manifest ${MIRANTE_MANIFEST} does not exist.")
        endif()
        file(READ "${MIRANTE_MANIFEST}" manifest)
        string(JSON count ERROR_VARIABLE error LENGTH "${manifest}" qt)
        if(error)
            message(FATAL_ERROR "${MIRANTE_MANIFEST} has no \"qt\" list: ${error}")
        endif()
        if(count GREATER 0)
            math(EXPR last "${count} - 1")
            foreach(index RANGE ${last})
                string(JSON module GET "${manifest}" qt ${index})
                list(APPEND modules "${module}")
            endforeach()
        endif()
    endif()
    set(${out} "${modules}" PARENT_SCOPE)
endfunction()

# Stops configuration if a JSON object has a field that is not in <known>. A
# misspelt field would otherwise build the wrong thing without saying so.
function(_mirante_manifest_check_fields json where known)
    string(JSON count LENGTH "${json}")
    if(count EQUAL 0)
        return()
    endif()
    math(EXPR last "${count} - 1")
    foreach(index RANGE ${last})
        string(JSON field MEMBER "${json}" ${index})
        if(NOT field IN_LIST known)
            message(FATAL_ERROR "${MIRANTE_MANIFEST}: ${where} has the unknown field \"${field}\". "
                "The fields are: ${known}")
        endif()
    endforeach()
endfunction()

# Reads one required field.
function(_mirante_manifest_get out json where field)
    string(JSON value ERROR_VARIABLE error GET "${json}" "${field}")
    if(error)
        message(FATAL_ERROR "${MIRANTE_MANIFEST}: ${where} has no field \"${field}\".")
    endif()
    set(${out} "${value}" PARENT_SCOPE)
endfunction()

# Fetches one repository of the manifest and adds its listed folders. The
# repository's top CMakeLists.txt is for its own CI and must not run here, so
# SOURCE_SUBDIR names a folder that does not exist, and FetchContent adds nothing.
function(_mirante_add_manifest_repository entry where)
    _mirante_manifest_check_fields("${entry}" "${where}" "name;repo;ref;folders")
    _mirante_manifest_get(name "${entry}" "${where}" name)
    _mirante_manifest_get(repo "${entry}" "${where}" repo)
    _mirante_manifest_get(ref "${entry}" "${where}" ref)
    _mirante_manifest_get(folders "${entry}" "${where}" folders)

    if(NOT name MATCHES "^[a-z][a-z0-9_]*$")
        message(FATAL_ERROR "${MIRANTE_MANIFEST}: ${where} has the name \"${name}\", "
            "which must have lowercase letters, digits and underscores only.")
    endif()
    if(name STREQUAL "mirante_core")
        message(FATAL_ERROR "${MIRANTE_MANIFEST}: ${where} has the name \"${name}\", which is the core's.")
    endif()

    string(JSON folder_count LENGTH "${folders}")
    if(folder_count EQUAL 0)
        message(FATAL_ERROR "${MIRANTE_MANIFEST}: ${where} lists no folders.")
    endif()

    FetchContent_Declare(${name}
        GIT_REPOSITORY "${repo}"
        GIT_TAG "${ref}"
        SOURCE_SUBDIR .mirante-adds-the-folders)
    FetchContent_MakeAvailable(${name})
    FetchContent_GetProperties(${name} SOURCE_DIR source_dir)

    # What the About dialog shows. A checkout on disk has no pin to show.
    string(TOUPPER "${name}" upper_name)
    set(commit "unknown")
    if(FETCHCONTENT_SOURCE_DIR_${upper_name})
        set(commit "local")
    else()
        find_package(Git QUIET)
        if(GIT_EXECUTABLE)
            execute_process(COMMAND "${GIT_EXECUTABLE}" rev-parse --short=12 HEAD
                WORKING_DIRECTORY "${source_dir}"
                OUTPUT_VARIABLE head OUTPUT_STRIP_TRAILING_WHITESPACE ERROR_QUIET)
            if(head)
                set(commit "${head}")
            endif()
        endif()
    endif()

    math(EXPR last "${folder_count} - 1")
    foreach(index RANGE ${last})
        string(JSON folder GET "${folders}" ${index})
        if(folder STREQUAL ".")
            set(folder_dir "${source_dir}")
            set(binary_dir "${CMAKE_BINARY_DIR}/modules/${name}")
        else()
            set(folder_dir "${source_dir}/${folder}")
            set(binary_dir "${CMAKE_BINARY_DIR}/modules/${name}/${folder}")
        endif()
        if(NOT EXISTS "${folder_dir}/CMakeLists.txt")
            message(FATAL_ERROR "${MIRANTE_MANIFEST}: ${where} lists the folder \"${folder}\", "
                "which has no CMakeLists.txt in ${source_dir}.")
        endif()

        get_property(modules_before GLOBAL PROPERTY MIRANTE_ALL_MODULES)
        add_subdirectory("${folder_dir}" "${binary_dir}")
        get_property(modules_after GLOBAL PROPERTY MIRANTE_ALL_MODULES)
        if("${modules_before}" STREQUAL "${modules_after}")
            message(FATAL_ERROR "${MIRANTE_MANIFEST}: ${where} lists the folder \"${folder}\", "
                "which adds no module.")
        endif()
        foreach(module IN LISTS modules_after)
            if(NOT module IN_LIST modules_before)
                set_property(GLOBAL PROPERTY MIRANTE_MODULE_REPOSITORY_${module} "${repo}")
                set_property(GLOBAL PROPERTY MIRANTE_MODULE_COMMIT_${module} "${commit}")
            endif()
        endforeach()
        # For tools/scan-module-apis.py --build.
        file(APPEND "${CMAKE_BINARY_DIR}/mirante-module-folders.txt" "${name}/${folder}\t${folder_dir}\n")
    endforeach()
endfunction()

# mirante_add_manifest_modules()
#
# Fetches the repositories that MIRANTE_MANIFEST lists and adds their listed
# folders. Without MIRANTE_MANIFEST, the core builds with no modules. This runs
# in the core's scope, below its find_package(Qt6), since the targets that
# find_package() makes are visible only from there down.
function(mirante_add_manifest_modules)
    if(NOT MIRANTE_MANIFEST)
        return()
    endif()
    if(NOT EXISTS "${MIRANTE_MANIFEST}")
        message(FATAL_ERROR "The manifest ${MIRANTE_MANIFEST} does not exist.")
    endif()
    set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS "${MIRANTE_MANIFEST}")

    file(READ "${MIRANTE_MANIFEST}" manifest)
    string(JSON type ERROR_VARIABLE error TYPE "${manifest}")
    if(error OR NOT type STREQUAL "OBJECT")
        message(FATAL_ERROR "${MIRANTE_MANIFEST} is not a JSON object: ${error}")
    endif()
    _mirante_manifest_check_fields("${manifest}" "the manifest" "qt;core;modules")
    _mirante_manifest_get(core "${manifest}" "the manifest" core)
    _mirante_manifest_check_fields("${core}" "the core entry" "repo;ref")
    _mirante_manifest_get(repositories "${manifest}" "the manifest" modules)

    # A module in a listed folder is built, whatever its DEFAULT says.
    # mirante_add_module() reads this. A folder's scope inherits it.
    set(MIRANTE_LISTED_MODULE ON)

    # Each configuration writes the list of module folders again.
    file(WRITE "${CMAKE_BINARY_DIR}/mirante-module-folders.txt" "")

    string(JSON count LENGTH "${repositories}")
    if(count EQUAL 0)
        return()
    endif()
    math(EXPR last "${count} - 1")
    foreach(index RANGE ${last})
        string(JSON entry GET "${repositories}" ${index})
        _mirante_add_manifest_repository("${entry}" "module repository ${index}")
    endforeach()
endfunction()
