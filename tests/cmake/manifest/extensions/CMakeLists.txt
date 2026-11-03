# A repository of extensions: its top file is for its own CI, and must not run
# in an app.
message(FATAL_ERROR "The top CMakeLists.txt of an extension repository ran in an app.")
