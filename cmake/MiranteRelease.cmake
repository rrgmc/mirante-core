# mirante_release_of(<out> <ref> <commit>)
#
# What the About dialog shows as the version of a module or an extension: the
# release X.Y.Z, or X.Y.Z-rc.N, when the pin <ref> is a release tag vX.Y.Z, or
# else the short <commit>. A checkout on disk has the commit "local", which
# stays, since its code need not be at the tag.
function(mirante_release_of out ref commit)
    set(release "${commit}")
    if(NOT commit STREQUAL "local" AND ref MATCHES "^v([0-9]+\\.[0-9]+\\.[0-9]+(-rc\\.[0-9]+)?)$")
        set(release "${CMAKE_MATCH_1}")
    endif()
    set(${out} "${release}" PARENT_SCOPE)
endfunction()
