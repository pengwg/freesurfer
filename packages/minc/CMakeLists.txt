project(minc)

include_directories(${FS_INCLUDE_DIRS})

set(SOURCES
  minc_convenience.c
  minc_files.c
  minc_gaussian.c
  minc_image_conversion.c
  minc_input_free.c
  minc_input_mnc.c
  minc_inverse.c
  minc_multidim_arrays.c
  minc_netcdf_convenience.c
  minc_output_mnc.c
  minc_output_volume.c
  minc_progress.c
  minc_time.c
  minc_value_conversion.c
  minc_volume_io.c
)

add_library(minc STATIC ${SOURCES})
