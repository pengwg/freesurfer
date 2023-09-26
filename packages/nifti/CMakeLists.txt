project(nifti)

add_library(nifti STATIC nifti1_io.c znzlib.c)

target_link_libraries(nifti z)
