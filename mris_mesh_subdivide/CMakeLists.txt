project(mris_mesh_subdivide)

if(VTK_FOUND)

  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-deprecated")

  include_directories(${FS_INCLUDE_DIRS})
  include(${VTK_USE_FILE})
  add_executable(mris_mesh_subdivide mris_mesh_subdivide.cxx)
  target_link_libraries(mris_mesh_subdivide utils ${VTK_LIBRARIES})

  install(TARGETS mris_mesh_subdivide DESTINATION bin)

endif()
