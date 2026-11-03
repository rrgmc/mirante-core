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

# mirante_manifest_edition(<out>)
#
# Reads the "edition" of MIRANTE_MANIFEST, or empty for a build with none: a
# name of lowercase letters, digits and "-", from 1 to 32 characters, which
# starts and ends with a letter or a digit. See decision 0131.
function(mirante_manifest_edition out)
    set(edition "")
    if(MIRANTE_MANIFEST AND EXISTS "${MIRANTE_MANIFEST}")
        file(READ "${MIRANTE_MANIFEST}" manifest)
        string(JSON type ERROR_VARIABLE missing TYPE "${manifest}" edition)
        if(NOT missing)
            if(type STREQUAL "STRING")
                string(JSON edition GET "${manifest}" edition)
            endif()
            string(LENGTH "${edition}" length)
            if(NOT type STREQUAL "STRING" OR length GREATER 32
               OR NOT edition MATCHES "^[a-z0-9]([a-z0-9-]*[a-z0-9])?$")
                message(FATAL_ERROR "${MIRANTE_MANIFEST} has the edition \"${edition}\", which must have "
                    "lowercase letters, digits and \"-\" only, up to 32, and start and end with a letter or a digit.")
            endif()
        endif()
    endif()
    set(${out} "${edition}" PARENT_SCOPE)
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

# Reads one folder entry: a folder name, or an object with "folder" and its
# settings. Sets folder, optional (ON or OFF) and options (a JSON object, or
# empty) in the caller's scope.
function(_mirante_manifest_folder_entry folders index where)
    string(JSON type TYPE "${folders}" ${index})
    set(optional OFF)
    set(options "")
    if(type STREQUAL "STRING")
        string(JSON folder GET "${folders}" ${index})
    elseif(type STREQUAL "OBJECT")
        string(JSON entry GET "${folders}" ${index})
        set(entry_where "folder entry ${index} of ${where}")
        _mirante_manifest_check_fields("${entry}" "${entry_where}" "folder;optional;options")
        _mirante_manifest_get(folder "${entry}" "${entry_where}" folder)
        string(JSON optional_type ERROR_VARIABLE missing TYPE "${entry}" optional)
        if(NOT missing)
            if(NOT optional_type STREQUAL "BOOLEAN")
                message(FATAL_ERROR "${MIRANTE_MANIFEST}: ${entry_where} has \"optional\", "
                    "which must be true or false.")
            endif()
            string(JSON optional GET "${entry}" optional)
        endif()
        string(JSON options_type ERROR_VARIABLE missing TYPE "${entry}" options)
        if(NOT missing)
            if(NOT options_type STREQUAL "OBJECT")
                message(FATAL_ERROR "${MIRANTE_MANIFEST}: ${entry_where} has \"options\", "
                    "which must be an object of names and texts.")
            endif()
            string(JSON options GET "${entry}" options)
        endif()
    else()
        message(FATAL_ERROR "${MIRANTE_MANIFEST}: folder entry ${index} of ${where} "
            "must be a folder name or an object.")
    endif()
    set(folder "${folder}" PARENT_SCOPE)
    set(optional "${optional}" PARENT_SCOPE)
    set(options "${options}" PARENT_SCOPE)
endfunction()

# Returns the folder of the app repository that "local" names, which must stay
# inside the app repository, the top source folder.
function(_mirante_manifest_local_folder out local where)
    if(IS_ABSOLUTE "${local}" OR local MATCHES "^[A-Za-z]:" OR local MATCHES "\\\\")
        message(FATAL_ERROR "${MIRANTE_MANIFEST}: ${where} has \"local\": \"${local}\", "
            "which must be a folder of the app repository, written with /.")
    endif()
    get_filename_component(folder "${CMAKE_SOURCE_DIR}/${local}" ABSOLUTE)
    file(RELATIVE_PATH inside "${CMAKE_SOURCE_DIR}" "${folder}")
    if(inside MATCHES "^\\.\\.(/|$)")
        message(FATAL_ERROR "${MIRANTE_MANIFEST}: ${where} has \"local\": \"${local}\", "
            "which leaves the app repository.")
    endif()
    if(NOT IS_DIRECTORY "${folder}")
        message(FATAL_ERROR "${MIRANTE_MANIFEST}: ${where} has \"local\": \"${local}\", "
            "which is not a folder of the app repository.")
    endif()
    set(${out} "${folder}" PARENT_SCOPE)
endfunction()

# Fetches one repository of the manifest and adds its listed folders. The
# repository's top CMakeLists.txt is for its own CI and must not run here, so
# SOURCE_SUBDIR names a folder that does not exist, and FetchContent adds nothing.
# A repository with "local" is a folder of the app repository itself, which is
# not fetched. See decision 0130.
function(_mirante_add_manifest_repository entry where)
    _mirante_manifest_check_fields("${entry}" "${where}" "name;repo;ref;local;folders")
    _mirante_manifest_get(name "${entry}" "${where}" name)
    _mirante_manifest_get(folders "${entry}" "${where}" folders)
    string(JSON local ERROR_VARIABLE no_local GET "${entry}" local)
    if(no_local)
        _mirante_manifest_get(repo "${entry}" "${where}" repo)
        _mirante_manifest_get(ref "${entry}" "${where}" ref)
    else()
        string(JSON repo ERROR_VARIABLE no_repo GET "${entry}" repo)
        string(JSON ref ERROR_VARIABLE no_ref GET "${entry}" ref)
        if(NOT no_repo OR NOT no_ref)
            message(FATAL_ERROR "${MIRANTE_MANIFEST}: ${where} has \"local\", "
                "so it takes no \"repo\" or \"ref\".")
        endif()
    endif()

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

    # What the About dialog shows. A checkout on disk has no pin to show. A
    # local folder shows the app repository and its commit.
    string(TOUPPER "${name}" upper_name)
    set(commit "unknown")
    find_package(Git QUIET)
    if(no_local)
        FetchContent_Declare(${name}
            GIT_REPOSITORY "${repo}"
            GIT_TAG "${ref}"
            SOURCE_SUBDIR .mirante-adds-the-folders)
        FetchContent_MakeAvailable(${name})
        FetchContent_GetProperties(${name} SOURCE_DIR source_dir)
        if(FETCHCONTENT_SOURCE_DIR_${upper_name})
            set(commit "local")
        endif()
    else()
        _mirante_manifest_local_folder(source_dir "${local}" "${where}")
        set(repo "local")
        set(commit "local")
        if(GIT_EXECUTABLE)
            execute_process(COMMAND "${GIT_EXECUTABLE}" remote get-url origin
                WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
                OUTPUT_VARIABLE origin OUTPUT_STRIP_TRAILING_WHITESPACE ERROR_QUIET)
            if(origin)
                set(repo "${origin}")
            endif()
        endif()
    endif()
    if(NOT commit STREQUAL "local" OR NOT no_local)
        if(GIT_EXECUTABLE)
            execute_process(COMMAND "${GIT_EXECUTABLE}" rev-parse --short=12 HEAD
                WORKING_DIRECTORY "${source_dir}"
                OUTPUT_VARIABLE head OUTPUT_STRIP_TRAILING_WHITESPACE ERROR_QUIET RESULT_VARIABLE git_result)
            if(head AND git_result EQUAL 0)
                set(commit "${head}")
            endif()
        endif()
    endif()

    math(EXPR last "${folder_count} - 1")
    foreach(index RANGE ${last})
        _mirante_manifest_folder_entry("${folders}" ${index} "${where}")
        # What mirante_add_module() reads in the folder's scope, which
        # inherits these. See decision 0129.
        set(MIRANTE_LISTED_OPTIONAL "${optional}")
        set(MIRANTE_LISTED_OPTIONS "${options}")
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
    _mirante_manifest_check_fields("${manifest}" "the manifest" "edition;qt;core;modules")
    _mirante_manifest_get(core "${manifest}" "the manifest" core)
    _mirante_manifest_check_fields("${core}" "the core entry" "repo;ref")
    _mirante_manifest_get(repositories "${manifest}" "the manifest" modules)

    # A module in a listed folder is built, whatever its DEFAULT says, unless
    # its folder entry makes it optional. mirante_add_module() reads this. A
    # folder's scope inherits it.
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
