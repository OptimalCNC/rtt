################################################################################
#
# CMake script for finding Log4cpp.
# The default CMake search process is used to locate files.
#
# This script creates the following variables:
#  LOG4CPP_FOUND: Boolean that indicates if the package was found
#  LOG4CPP_INCLUDE_DIRS: Paths to the necessary header files
#  LOG4CPP_LIBRARIES: Package libraries
#  LOG4CPP_LIBRARY_DIRS: Path to package libraries
#
################################################################################

include(FindPackageHandleStandardArgs)

# Find headers and libraries. Respect LOG4CPP_ROOT when provided so staged
# builds do not fall through to an installed prefix.
if(LOG4CPP_ROOT)
  find_path(LOG4CPP_INCLUDE_DIR NAMES log4cpp/Category.hh
    HINTS "${LOG4CPP_ROOT}/include"
    PATH_SUFFIXES orocos
    NO_DEFAULT_PATH)
  find_library(LOG4CPP_LIBRARY NAMES orocos-log4cpp
    HINTS "${LOG4CPP_ROOT}/lib"
    NO_DEFAULT_PATH)
endif()

if(NOT LOG4CPP_INCLUDE_DIR)
  find_path(LOG4CPP_INCLUDE_DIR NAMES log4cpp/Category.hh PATH_SUFFIXES orocos)
endif()
if(NOT LOG4CPP_LIBRARY)
  find_library(LOG4CPP_LIBRARY NAMES orocos-log4cpp)
endif()

# Set LOG4CPP_FOUND honoring the QUIET and REQUIRED arguments.
# RTT calls find_package(Log4cpp), so suppress the legacy module name mismatch.
find_package_handle_standard_args(LOG4CPP
  REQUIRED_VARS
    LOG4CPP_LIBRARY
    LOG4CPP_INCLUDE_DIR
  NAME_MISMATCHED)

# Output variables
if(LOG4CPP_FOUND)
  # Include dirs
  set(LOG4CPP_INCLUDE_DIRS ${LOG4CPP_INCLUDE_DIR})

  # Libraries
  set(LOG4CPP_LIBRARIES ${LOG4CPP_LIBRARY})

  # Link dirs
  get_filename_component(LOG4CPP_LIBRARY_DIRS ${LOG4CPP_LIBRARY} PATH)
endif()

# Advanced options for not cluttering the cmake UIs
mark_as_advanced(LOG4CPP_INCLUDE_DIR LOG4CPP_LIBRARY)
