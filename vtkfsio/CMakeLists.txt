project(vtkfsio)

if(VTK_FOUND)

  include_directories(${FS_INCLUDE_DIRS})

  set(SOURCES
    vtkFSSurfaceLabelSource.cxx
    vtkFSSurfaceWriter.cxx
    vtkFSSurfaceScalarsReader.cxx
    vtkFSSurfaceSource.cxx
    vtkFSVolumeSource.cxx
  )

  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-deprecated")
  include(${VTK_USE_FILE})
  add_library(vtkfsio STATIC ${SOURCES})

endif()
