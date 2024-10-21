project(nifti)

add_library(nifti STATIC nifti1_io.c znzlib.c)

target_link_libraries(nifti z)
target_compile_options(nifti PRIVATE -Wno-error=int-conversion)
