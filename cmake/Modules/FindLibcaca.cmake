# - Find the native caca includes and library
#
# This module defines
#  LIBCACA_INCLUDE_DIR, where to find caca.h, etc.
#  LIBCACA_LIBRARIES, the libraries to link against to use caca
#  LIBCACA_FOUND, If false, do not try to use caca
# also defined, but not for general use are
#  LIBCACA_LIBRARY, where to find the caca library

#=============================================================================
# Created using FindExiv2.cmake in darktable.
# Copyright 2010 henrik andersson
# Copyright 2015 Jean-Sébastien Pédron
#=============================================================================

include(LibFindMacros)

SET(LIBCACA_FIND_REQUIRED ${Libcaca_FIND_REQUIRED})

find_path(LIBCACA_INCLUDE_DIR NAMES caca.h)
mark_as_advanced(LIBCACA_INCLUDE_DIR)

set(LIBCACA_NAMES ${LIBCACA_NAMES} caca libcaca)
find_library(LIBCACA_LIBRARY NAMES ${LIBCACA_NAMES})
mark_as_advanced(LIBCACA_LIBRARY)

libfind_pkg_check_modules(Libcaca caca)

if(Libcaca_VERSION VERSION_LESS Libcaca_FIND_VERSION)
  message(FATAL_ERROR "Libcaca version check failed.  Version ${Libcaca_VERSION} was found, at least version ${Libcaca_FIND_VERSION} is required")
endif(Libcaca_VERSION VERSION_LESS Libcaca_FIND_VERSION)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(LIBCACA DEFAULT_MSG LIBCACA_LIBRARY LIBCACA_INCLUDE_DIR)

IF(LIBCACA_FOUND)
  SET(Libcaca_LIBRARIES ${LIBCACA_LIBRARY})
  SET(Libcaca_INCLUDE_DIRS ${LIBCACA_INCLUDE_DIR})
ENDIF(LIBCACA_FOUND)
