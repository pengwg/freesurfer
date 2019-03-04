# GTS Find Module
# note: only used by mris_decimate

find_path(GTS_INCLUDE_DIR NAMES gts.h)
find_library(GTS_LIBRARY NAMES libgts.a)

# Let's include glib-2.0 here since it's required by GTS.
# Correctly linking glib on mac is very messy, so all the glib
# headers and libraries (and the libintl.a dependency) have been copied
# into the GTS installation

find_path(GLIB_INCLUDE_DIR NAMES glib.h PATH_SUFFIXES glib-2.0)
find_path(GLIB_CONFIG_INCLUDE_DIR NAMES glibconfig.h HINTS /usr/lib/x86_64-linux-gnu PATH_SUFFIXES glib-2.0/include)
find_library(GLIB_LIBRARY NAMES libglib-2.0.a glib-2.0)

if(APPLE)
  find_library(INTL_LIBRARY NAMES libintl.a PATH_SUFFIXES lib)
  find_library(ICONV_LIBRARY NAMES iconv)
  set(APPLE_GLIB_DEPENDENCIES ICONV_LIBRARY INTL_LIBRARY)
endif()

find_package_handle_standard_args(GTS DEFAULT_MSG
  GTS_INCLUDE_DIR
  GTS_LIBRARY
  GLIB_INCLUDE_DIR
  GLIB_CONFIG_INCLUDE_DIR
  GLIB_LIBRARY
  ${APPLE_GLIB_DEPENDENCIES}
)

set(GTS_INCLUDE_DIRS ${GTS_INCLUDE_DIR} ${GLIB_INCLUDE_DIR} ${GLIB_CONFIG_INCLUDE_DIR})
set(GTS_LIBRARIES ${GTS_LIBRARY} ${GLIB_LIBRARY} ${ICONV_LIBRARY} ${INTL_LIBRARY})
