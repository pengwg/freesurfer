project(vtkutils)

if(VTK_FOUND)

  include_directories(
    ${FS_INCLUDE_DIRS}
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/packages/tiff
    ${X11_INCLUDE_DIR}
  )

  set(CMAKE_CXX_STANDARD 98)

  set(SOURCES
    vtkFreesurferLookupTable.cxx
    vtkArrowPipeline.cxx
    vtkRGBATransferFunction.cxx
    vtkRGBAColorTransferFunction.cxx
    vtkFDTensorGlyph.cxx
    vtkODFGlyph.cxx
    vtkInflatePolyData.cxx
  )

  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-deprecated")
  include(${VTK_USE_FILE})
  add_library(vtkutils STATIC ${SOURCES})
  target_link_libraries(vtkutils tiff)

endif()
