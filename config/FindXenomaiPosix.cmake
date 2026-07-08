################################################################################
#
# CMake script for finding the XENOMAI Posix skin.
# If the optional XENOMAI_ROOT_DIR environment variable exists, header files and
# libraries will be searched in the XENOMAI_ROOT_DIR/include and XENOMAI_ROOT_DIR/lib
# directories, respectively. Otherwise the default CMake search process will be
# used.
#
# This script creates the following variables:
#  XENOMAI_POSIX_FOUND: Boolean that indicates if the package was found
#  XENOMAI_POSIX_INCLUDE_DIRS: Paths to the necessary header files
#  XENOMAI_POSIX_LIBRARIES: Package libraries
#
################################################################################

include(LibFindMacros)

# Get hint from environment variable (if any)
if(NOT $ENV{XENOMAI_ROOT_DIR} STREQUAL "")
  set(XENOMAI_ROOT_DIR $ENV{XENOMAI_ROOT_DIR} CACHE PATH "Xenomai Posix base directory location (optional, used for nonstandard installation paths)" FORCE)
  mark_as_advanced(XENOMAI_ROOT_DIR)
endif()

if ( XenomaiPosix_FIND_QUIETLY )
  set( XENOMAI_POSIX_FIND_QUIETLY "QUIET")
endif()

if ( XenomaiPosix_FIND_REQUIRED )
  set( XENOMAI_POSIX_FIND_REQUIRED "REQUIRED")
endif()

# Find headers and libraries
if(XENOMAI_ROOT_DIR)
  # Use location specified by environment variable
  find_program(XENOMAI_XENO_CONFIG NAMES xeno-config  PATHS ${XENOMAI_ROOT_DIR}/bin NO_DEFAULT_PATH)
else()
  # Use default CMake search process
  find_program(XENOMAI_XENO_CONFIG NAMES xeno-config )
endif()

if(XENOMAI_XENO_CONFIG)
  execute_process(COMMAND ${XENOMAI_XENO_CONFIG} --skin=posix --cflags
    RESULT_VARIABLE XENOMAI_POSIX_CFLAGS_RESULT
    OUTPUT_VARIABLE XENOMAI_POSIX_CFLAGS
    OUTPUT_STRIP_TRAILING_WHITESPACE)
  execute_process(COMMAND ${XENOMAI_XENO_CONFIG} --skin=posix --no-auto-init --ldflags
    RESULT_VARIABLE XENOMAI_POSIX_LDFLAGS_RESULT
    OUTPUT_VARIABLE XENOMAI_POSIX_LDFLAGS
    OUTPUT_STRIP_TRAILING_WHITESPACE)

  if(NOT XENOMAI_POSIX_CFLAGS_RESULT EQUAL 0 OR NOT XENOMAI_POSIX_LDFLAGS_RESULT EQUAL 0)
    set(XENOMAI_POSIX_FOUND FALSE)
    return()
  endif()

  separate_arguments(XENOMAI_POSIX_CFLAGS_LIST UNIX_COMMAND "${XENOMAI_POSIX_CFLAGS}")
  separate_arguments(XENOMAI_POSIX_LDFLAGS_LIST UNIX_COMMAND "${XENOMAI_POSIX_LDFLAGS}")

  set(XENOMAI_POSIX_INCLUDE_DIR)
  foreach(flag ${XENOMAI_POSIX_CFLAGS_LIST})
    if(flag MATCHES "^-I(.+)")
      list(APPEND XENOMAI_POSIX_INCLUDE_DIR "${CMAKE_MATCH_1}")
    endif()
  endforeach()
  list(REMOVE_DUPLICATES XENOMAI_POSIX_INCLUDE_DIR)

  set(XENOMAI_POSIX_LIBRARY ${XENOMAI_POSIX_LDFLAGS_LIST})
else()
  # Legacy fallback for installations without xeno-config.
  set(header_NAME pthread.h)
  find_path(XENOMAI_POSIX_INCLUDE_DIR NAMES ${header_NAME} PATH_SUFFIXES xenomai/posix xenomai)
  find_library(XENOMAI_POSIX_LIBRARY NAMES pthread_rt)
endif()

# Set the include dir variables and the libraries and let libfind_process do the rest.
# NOTE: Singular variables for this library, plural for libraries this this lib depends on.
set(XENOMAI_POSIX_PROCESS_INCLUDES XENOMAI_POSIX_INCLUDE_DIR)
set(XENOMAI_POSIX_PROCESS_LIBS XENOMAI_POSIX_LIBRARY)

message("Found XenomaiPosix in ${XENOMAI_POSIX_INCLUDE_DIR}")

libfind_process(XENOMAI_POSIX)
