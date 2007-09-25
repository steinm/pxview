PROJECT(pxview C)
OPTION(MEMORY_DEBUGGING   "Enable extra code for memory debugging" OFF)
OPTION(ENABLE_SQLITE      "Enable output of sqlite databases" ON)

if(CMAKE_COMPILER_IS_GNUCC)
  add_definitions(-Wall -Wpointer-arith)
	set(all_LIBS px)
else(CMAKE_COMPILER_IS_GNUCC)
	set(all_LIBS pxlib)
endif(CMAKE_COMPILER_IS_GNUCC)

ADD_DEFINITIONS(-DHAVE_CONFIG_H)

set(CPACK_PACKAGE_VERSION_MAJOR "0")
set(CPACK_PACKAGE_VERSION_MINOR "2")
set(CPACK_PACKAGE_VERSION_PATCH "6")
set(CPACK_PACKAGE_VERSION "${CPACK_PACKAGE_VERSION_MAJOR}.${CPACK_PACKAGE_VERSION_MINOR}.${CPACK_PACKAGE_VERSION_PATCH}")

include(CheckFunctionExists)
include(CheckLibraryExists)
include(CheckIncludeFile)

#check system for includes
check_include_file("sqlite.h"           HAVE_SQLITE_H)
check_include_file("locale.h"           HAVE_LOCALE_H)
check_include_file("iconv.h"            HAVE_ICONV_H)
check_include_file("libintl.h"          HAVE_LIBINTL_H)
check_include_file("regex.h"            HAVE_REGEX_H)
check_include_file("stdarg.h"           HAVE_STDARG_H)
check_include_file("stdlib.h"           HAVE_STDLIB_H)
check_include_file("getopt.h"           HAVE_GETOPT_H)
check_include_file("paradox.h"          HAVE_PARADOX_H)

# Checking for right version of pxlib
if(NOT HAVE_PARADOX_H)
	MESSAGE(FATAL_ERROR "Could not find header file for pxlib")
ENDIF(NOT HAVE_PARADOX_H)

FIND_LIBRARY(HAVE_PXLIB px)
IF(HAVE_PXLIB)
	MESSAGE(STATUS "Found pxlib in ${HAVE_PXLIB}")
	CHECK_LIBRARY_EXISTS(px "PX_timestamp2string" "/usr /usr/local" HAVE_PXLIB_SYMBOL)
	IF(HAVE_PXLIB_SYMBOL)
		MESSAGE(STATUS "Good! pxlib is a recent version")
	ELSE(HAVE_PXLIB_SYMBOL)
		MESSAGE(FATAL_ERROR "Could not find function PX_timestamp2string() in pxlib")
	ENDIF(HAVE_PXLIB_SYMBOL)

ELSE(HAVE_PXLIB)
	MESSAGE(FATAL_ERROR "Could not find library pxlib")
ENDIF(HAVE_PXLIB)

# Check if sqlite is available
IF(ENABLE_SQLITE)
	if(HAVE_SQLITE_H)
		FIND_LIBRARY(HAVE_SQLITE sqlite)
		IF(HAVE_SQLITE)
			MESSAGE(STATUS "Found sqlite library in ${HAVE_SQLITE}")
		ELSE(HAVE_SQLITE)
			MESSAGE(STATUS "Could not find sqlite library")
		ENDIF(HAVE_SQLITE)
	endif(HAVE_SQLITE_H)
	if(HAVE_SQLITE)
		set(all_LIBS ${all_LIBS} sqlite)
	endif(HAVE_SQLITE)
ENDIF(ENABLE_SQLITE)

INCLUDE_DIRECTORIES( . )

configure_file(${CMAKE_SOURCE_DIR}/cmakeconfig.h.in ${CMAKE_BINARY_DIR}/config.h)

if(CMAKE_COMPILER_IS_GNUCC)
	set(pxview_FILES src/main.c)
else(CMAKE_COMPILER_IS_GNUCC)
	set(pxview_FILES src/main.c getopt/my_getopt.c)
endif(CMAKE_COMPILER_IS_GNUCC)

add_executable(pxview ${pxview_FILES})
target_link_libraries(pxview ${all_LIBS})

#install(TARGETS draw RUNTIME DESTINATION ${CMAKE_INSTALL_SBINDIR})

