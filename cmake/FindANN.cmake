# ANN Find Module

find_path(ANN_INCLUDE_DIR NAMES ANN)
find_library(ANN_LIBRARIES NAMES libann.a)
find_package_handle_standard_args(ANN DEFAULT_MSG ANN_INCLUDE_DIR ANN_LIBRARIES)
