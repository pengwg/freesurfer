project(vtkutils)

if(VTK_FOUND)

  include_directories(
    ${FS_INCLUDE_DIRS}
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/packages/tiff
    ${X11_INCLUDE_DIR}
  )

  set(SOURCES
    vtkFreesurferLookupTable.cxx
    vtkArrowPipeline.cxx
    vtkRGBATransferFunction.cxx
    vtkRGBAColorTransferFunction.cxx
    vtkInflatePolyData.cxx
    # vtkFDTensorGlyph.cxx
    # vtkODFGlyph.cxx
  )

  add_compile_options(-Wno-deprecated -Wno-inconsistent-missing-override -fpermissive)

  add_library(vtkutils STATIC ${SOURCES})
  target_include_directories(vtkutils PRIVATE $<TARGET_PROPERTY:VTK::CommonCore,INTERFACE_INCLUDE_DIRECTORIES>)
  target_link_libraries(vtkutils tiff)
endif()
