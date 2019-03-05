# PETSC Find Module

if(NOT PETSC_DIR)
  set(PETSC_DIR 
      /usr/lib/petscdir/3.7.7/x86_64-linux-gnu-real
      /usr/lib/petscdir/petsc3.9/x86_64-linux-gnu-real)
endif()

find_path(PETSC_INCLUDE_DIR HINTS ${PETSC_DIR} NAMES petsc.h PATH_SUFFIXES include)
find_path(MPI_INCLUDE_DIR NAMES mpi.h PATH_SUFFIXES openmpi)

find_package_handle_standard_args(PETSC DEFAULT_MSG PETSC_INCLUDE_DIR)

library_paths(
  NAME PETSC_LIBRARIES
  LIBDIR ${PETSC_DIR}/lib
  LIBRARIES
  petsc_real
  mpi
)
