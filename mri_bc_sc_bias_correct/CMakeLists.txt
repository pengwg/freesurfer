project(mri_bc_sc_bias_correct)

include_directories(${FS_INCLUDE_DIRS})

add_executable(mri_bc_sc_bias_correct mri_bc_sc_bias_correct.c)
target_link_libraries(mri_bc_sc_bias_correct utils)

install(TARGETS mri_bc_sc_bias_correct DESTINATION bin)
