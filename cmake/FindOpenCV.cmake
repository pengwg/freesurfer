# OpenCS Find Module

# find the include dir
find_path(OpenCV_INCLUDE_DIR NAMES opencv opencv2 PATH_SUFFIXES include)

# find the libraries
foreach(LIB opencv_core opencv_imgproc opencv_highgui opencv_ml)
  find_library(tmp NAMES ${LIB} PATH_SUFFIXES lib)
  set(OpenCV_LIBRARIES ${OpenCV_LIBRARIES} ${tmp})
  unset(tmp CACHE)  # this is necessary for find_library to work (plus it clears it from the cache)
endforeach()

find_package_handle_standard_args(OpenCV DEFAULT_MSG OpenCV_INCLUDE_DIR OpenCV_LIBRARIES)
