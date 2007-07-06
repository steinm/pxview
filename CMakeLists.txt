PROJECT(pxview C)
OPTION(MEMORY_DEBUGGING   "Enable extra code for memory debugging" OFF)

if(CMAKE_COMPILER_IS_GNUCC)
  add_definitions(-Wall -Wpointer-arith)
	set(all_LIBS px)
else(CMAKE_COMPILER_IS_GNUCC)
	set(all_LIBS pxlib)
endif(CMAKE_COMPILER_IS_GNUCC)

set(CPACK_PACKAGE_VERSION_MAJOR "0")
set(CPACK_PACKAGE_VERSION_MINOR "2")
set(CPACK_PACKAGE_VERSION_PATCH "6")
set(CPACK_PACKAGE_VERSION "${CPACK_PACKAGE_VERSION_MAJOR}.${CPACK_PACKAGE_VERSION_MINOR}.${CPACK_PACKAGE_VERSION_PATCH}")

#check system for includes
include(CheckIncludeFile)
check_include_file("sqlite.h"           HAVE_SQLITE_H)
check_include_file("locale.h"           HAVE_LOCALE_H)
check_include_file("libintl.h"          HAVE_LIBINTL_H)
check_include_file("regex.h"            HAVE_REGEX_H)
check_include_file("stdarg.h"           HAVE_STDARG_H)
check_include_file("stdlib.h"           HAVE_STDLIB_H)
check_include_file("getopt.h"           HAVE_GETOPT_H)
check_include_file("paradox.h"          HAVE_PARADOX_H)

if(NOT HAVE_PARADOX_H)
	MESSAGE(FATAL_ERROR "Could not find header file for pxlib")
endif(NOT HAVE_PARADOX_H)

FIND_LIBRARY(HAVE_PXLIB px)
IF(HAVE_PXLIB)
	MESSAGE(STATUS "Found pxlib in ${HAVE_PXLIB}")
ELSE(HAVE_PXLIB)
	MESSAGE(FATAL_ERROR "Could not find pxlib")
ENDIF(HAVE_PXLIB)

if(HAVE_SQLITE_H)
	FIND_LIBRARY(HAVE_SQLITE sqlite)
	IF(HAVE_SQLITE)
		MESSAGE(STATUS "Found sqlite libraray in ${HAVE_PXLIB}")
	ELSE(HAVE_SQLITE)
		MESSAGE(STATUS "Could not find sqlite library")
	ENDIF(HAVE_SQLITE)
endif(HAVE_SQLITE_H)

FILE(WRITE /tmp/conftest.c.in
"#ifdef __cplusplus
extern \"C\"
#endif
char \@SYMBOL_NAME\@ ();
int main () {
return \@SYMBOL_NAME\@ ();
  ;
  return 0;
}
")
SET(SYMBOL_NAME "PX_get_data_bcd")
SET(LINK_LIBRARIES "px")
CONFIGURE_FILE(/tmp/conftest.c.in /tmp/conftest.c @ONLY)
TRY_COMPILE(HAVE_GSF /tmp /tmp/conftest.c OUTPUT_VARIABLE TEST_OUTPUT)
MESSAGE(STATUS ${TEST_OUTPUT})
#FILE(REMOVE /tmp/conftest.c)

INCLUDE_DIRECTORIES( . )

configure_file(${CMAKE_SOURCE_DIR}/cmakeconfig.h.in ${CMAKE_BINARY_DIR}/config.h)

set(pxview_FILES src/main.c)
add_executable(pxview ${pxview_FILES})
target_link_libraries(pxview ${all_LIBS})
if(HAVE_SQLITE)
	target_link_libraries(pxview sqlite)
endif(HAVE_SQLITE)

#install(TARGETS draw RUNTIME DESTINATION ${CMAKE_INSTALL_SBINDIR})

