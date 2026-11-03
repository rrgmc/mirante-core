# A module repository's top file is for its own CI, and must not run in an app.
message(FATAL_ERROR "The top CMakeLists.txt of a module repository ran in an app.")
