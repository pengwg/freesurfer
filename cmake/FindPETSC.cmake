# PETSC Find Module

if(NOT PETSC_DIR)
  set(PETSC_DIR /usr/lib/petscdir/petsc3.12/x86_64-linux-gnu-real/)
endif()

find_path(PETSC_INCLUDE_DIR HINTS ${PETSC_DIR} NAMES petsc.h PATH_SUFFIXES include)
find_package_handle_standard_args(PETSC DEFAULT_MSG PETSC_INCLUDE_DIR)

find_package(MPI COMPONENTS CXX)

library_paths(
  NAME PETSC_LIBRARIES
  LIBDIR ${PETSC_DIR}/lib
  LIBRARIES
  petsc_real
)
