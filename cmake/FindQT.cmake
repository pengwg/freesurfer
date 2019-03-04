# Qt Find Module

# find Qt components
set(_qt_components Core Widgets)
if(NOT APPLE)
  set(_qt_components ${_qt_components} X11Extras)
endif()

find_package(Qt5 COMPONENTS ${_qt_components})

# cmake doesn't easily provide us with a cross-platform path to
# root qt install directory, so we'll use the hidden QtCore prefix
set(Qt5_INSTALL_DIR ${_qt5Core_install_prefix})

# install the shared libraries to the freesurfer lib directory
if(Qt5_FOUND AND NOT APPLE)
  file(GLOB _qt_libs_to_install "${Qt5_INSTALL_DIR}/lib/lib*.so*")
  if(_qt_libs_to_install)
    install(PROGRAMS ${_qt_libs_to_install} DESTINATION lib/qt/lib)
    # add Qt library directory to rpath
    set(CMAKE_INSTALL_RPATH "$ORIGIN/../lib/qt/lib:${CMAKE_INSTALL_RPATH}")
    # install the platform plugins as well, and make sure executables know
    # where to find the plugins directory
    if(EXISTS ${Qt5_INSTALL_DIR}/plugins/platforms)
      install(DIRECTORY ${Qt5_INSTALL_DIR}/plugins/platforms DESTINATION lib/qt/plugins)
      install(FILES ${CMAKE_SOURCE_DIR}/qt/qt.conf DESTINATION bin)
    endif()
  endif()
endif()
