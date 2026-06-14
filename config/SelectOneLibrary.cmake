# As some libraries are actually a list of libraries (e.g. "debug;xxx-d.lib;optimized;xxx.lib")
# and we can't deal with that in pkgconfig, we take
# 1) the library for this build type, or
# 2) the library if only one is listed, or
# 3) the release version if available and the build type is one of the release types, or
# 4) we error out.
#
# EXPECTED USAGE
#
#  SELECT_ONE_LIBRARY("Boost_THREAD_LIBRARY" BOOST_THREAD_LIB)
#  LIST(APPEND OROCOS-RTT_USER_LINK_LIBS ${BOOST_THREAD_LIB})
#
macro( SELECT_ONE_LIBRARY NAME RETURN)

  set(${RETURN} "")
  if (DEFINED CACHE{${NAME}} AND "${${NAME}}" MATCHES ".*-NOTFOUND$")
    get_property(_select_one_cached_library CACHE ${NAME} PROPERTY VALUE)
    if (_select_one_cached_library AND NOT "${_select_one_cached_library}" MATCHES ".*-NOTFOUND$")
      set(${NAME} "${_select_one_cached_library}")
    endif()
  endif()

  STRING(TOUPPER "${CMAKE_BUILD_TYPE}" CMAKE_BUILD_TYPE_UPPER)
  SET(NAME_U "${NAME}_${CMAKE_BUILD_TYPE_UPPER}")
  if (DEFINED ${NAME_U} AND NOT "${${NAME_U}}" MATCHES ".*-NOTFOUND$")

	set(${RETURN} ${${NAME_U}})

  else()

	LIST(LENGTH ${NAME} COUNT)
	if (1 EQUAL COUNT)

	  if (NOT "${${NAME}}" MATCHES ".*-NOTFOUND$")
		set(${RETURN} ${${NAME}})
	  endif()

	else (1 EQUAL COUNT)

	  # found nothing particular to this build type, but
	  # for all release-related types, use release (if available)

	  # these two if's have to be done separately for some reason ... :-(
	  if (CMAKE_BUILD_TYPE_UPPER MATCHES "RELWITHDEBINFO|RELEASE|MINSIZEREL")
		if (DEFINED ${NAME}_RELEASE AND NOT "${${NAME}_RELEASE}" MATCHES ".*-NOTFOUND$")
		  MESSAGE(STATUS "Defaulting to release library for ${CMAKE_BUILD_TYPE_UPPER}")
		  set(${RETURN} ${${NAME}_RELEASE})
		endif ()
	  endif ()

	endif()

  endif()

  if ("${${RETURN}}" STREQUAL "")
	MESSAGE(FATAL_ERROR "Found multiple ${NAME} libraries, but not one specific to, or related to, the current build type '${CMAKE_BUILD_TYPE_UPPER}'.")
  endif()

endmacro()
