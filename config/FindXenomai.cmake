################################################################################
#
# CMake script for finding the XENOMAI native/alchemy compatibility skin.
# If the optional XENOMAI_ROOT_DIR environment variable exists, header files and
# libraries will be searched in the XENOMAI_ROOT_DIR/include and XENOMAI_ROOT_DIR/lib
# directories, respectively. Otherwise the default CMake search process will be
# used.
#
# This script creates the following variables:
#  XENOMAI_FOUND: Boolean that indicates if the package was found
#  XENOMAI_INCLUDE_DIRS: Paths to the necessary header files
#  XENOMAI_LIBRARIES: Package libraries
#
################################################################################

include(LibFindMacros)

# Get hint from environment variable (if any)
if(NOT $ENV{XENOMAI_ROOT_DIR} STREQUAL "")
  set(XENOMAI_ROOT_DIR $ENV{XENOMAI_ROOT_DIR} CACHE PATH "Xenomai base directory location (optional, used for nonstandard installation paths)" FORCE)
  mark_as_advanced(XENOMAI_ROOT_DIR)
endif()

if ( Xenomai_FIND_QUIETLY )
  set( XENOMAI_FIND_QUIETLY "QUIET")
endif()

if ( Xenomai_FIND_REQUIRED )
  set( XENOMAI_FIND_REQUIRED "REQUIRED")
endif()

set(XENOMAI_AUTO_INIT_MODE "auto-init-solib" CACHE STRING "Xenomai auto-initialization link mode")
set_property(CACHE XENOMAI_AUTO_INIT_MODE PROPERTY STRINGS auto-init auto-init-solib no-auto-init)

# Find headers and libraries
if(XENOMAI_ROOT_DIR)
  # Use location specified by environment variable
  find_program(XENOMAI_XENO_CONFIG NAMES xeno-config  PATHS ${XENOMAI_ROOT_DIR}/bin NO_DEFAULT_PATH)
else()
  # Use default CMake search process
  find_program(XENOMAI_XENO_CONFIG NAMES xeno-config )
endif()

if(XENOMAI_XENO_CONFIG)
  execute_process(COMMAND ${XENOMAI_XENO_CONFIG} --version
    OUTPUT_VARIABLE XENOMAI_VERSION
    OUTPUT_STRIP_TRAILING_WHITESPACE)
  execute_process(COMMAND ${XENOMAI_XENO_CONFIG} --skin=native --cflags
    RESULT_VARIABLE XENOMAI_CFLAGS_RESULT
    OUTPUT_VARIABLE XENOMAI_CFLAGS
    OUTPUT_STRIP_TRAILING_WHITESPACE)

  if(XENOMAI_AUTO_INIT_MODE STREQUAL "auto-init")
    set(XENOMAI_AUTO_INIT_FLAG "--auto-init")
  elseif(XENOMAI_AUTO_INIT_MODE STREQUAL "no-auto-init")
    set(XENOMAI_AUTO_INIT_FLAG "--no-auto-init")
  else()
    set(XENOMAI_AUTO_INIT_FLAG "--auto-init-solib")
  endif()

  execute_process(COMMAND ${XENOMAI_XENO_CONFIG} --skin=native ${XENOMAI_AUTO_INIT_FLAG} --ldflags
    RESULT_VARIABLE XENOMAI_LDFLAGS_RESULT
    OUTPUT_VARIABLE XENOMAI_LDFLAGS
    OUTPUT_STRIP_TRAILING_WHITESPACE)

  if(NOT XENOMAI_CFLAGS_RESULT EQUAL 0 OR NOT XENOMAI_LDFLAGS_RESULT EQUAL 0)
    message(SEND_ERROR "Your Xenomai installation is broken: xeno-config could not determine native compatibility cflags/ldflags.")
  endif()

  separate_arguments(XENOMAI_CFLAGS_LIST UNIX_COMMAND "${XENOMAI_CFLAGS}")
  separate_arguments(XENOMAI_LDFLAGS_LIST UNIX_COMMAND "${XENOMAI_LDFLAGS}")

  set(XENOMAI_INCLUDE_DIR)
  foreach(flag ${XENOMAI_CFLAGS_LIST})
    if(flag MATCHES "^-I(.+)")
      list(APPEND XENOMAI_INCLUDE_DIR "${CMAKE_MATCH_1}")
    endif()
  endforeach()
  list(REMOVE_DUPLICATES XENOMAI_INCLUDE_DIR)

  set(XENOMAI_LIBRARY ${XENOMAI_LDFLAGS_LIST})
else()
  # Legacy fallback for installations without xeno-config.
  set(header_NAME native/task.h)
  find_path(XENOMAI_INCLUDE_DIR NAMES ${header_NAME} PATH_SUFFIXES xenomai)
  find_library(XENOMAI_LIBRARY NAMES native xenomai)
endif()


# Set the include dir variables and the libraries and let libfind_process do the rest.
# NOTE: Singular variables for this library, plural for libraries this this lib depends on.
set(XENOMAI_PROCESS_INCLUDES XENOMAI_INCLUDE_DIR)
set(XENOMAI_PROCESS_LIBS XENOMAI_LIBRARY)

libfind_process(XENOMAI)
