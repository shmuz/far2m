#  LIBLUA_FOUND     - System has liblua
#  LIBLUA_INCLUDE_DIR - The liblua include directories
#  LIBLUA_LIBRARIES    - The libraries needed to use liblua

find_path(LIBLUA_INCLUDE_DIR
    NAMES
      lua.h
    PATHS
      /usr/include
      /usr/local/include
      /opt/local/include
    PATH_SUFFIXES
      luajit-2.1
      luajit-2.0
      lua5.1
  )

find_library(LIBLUA_LIBRARY
    NAMES
      libluajit-5.1
      luajit-5.1
      liblua5.1
      lua5.1
    PATHS
      /usr/lib
      /usr/local/lib
      /opt/local/lib
  )

# handle the QUIETLY and REQUIRED arguments and set NF_FOUND to TRUE
# if all listed variables are TRUE
include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(LibLua
	REQUIRED_VARS LIBLUA_LIBRARY LIBLUA_INCLUDE_DIR
)

set(LIBLUA_LIBRARIES ${LIBLUA_LIBRARY})
set(LIBLUA_INCLUDE_DIRS ${LIBLUA_INCLUDE_DIR})
mark_as_advanced(LIBLUA_INCLUDE_DIR LIBLUA_LIBRARY)
