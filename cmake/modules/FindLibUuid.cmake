#  LIBUUID_FOUND     - System has libuuid
#  LIBUUID_INCLUDE_DIR - The libuuid include directories
#  LIBUUID_LIBRARIES    - The libraries needed to use libuuid

find_path(LIBUUID_INCLUDE_DIR
    NAMES
      uuid/uuid.h
    PATHS
      /usr/include
      /usr/local/include
      /opt/local/include
  )

find_library(LIBUUID_LIBRARY
    NAMES
      libuuid
      uuid
    PATHS
      /usr/lib
      /usr/local/lib
      /opt/local/lib
  )

# handle the QUIETLY and REQUIRED arguments and set NF_FOUND to TRUE
# if all listed variables are TRUE
include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(LibUuid
	REQUIRED_VARS LIBUUID_LIBRARY LIBUUID_INCLUDE_DIR
)

set(LIBUUID_LIBRARIES ${LIBUUID_LIBRARY})
set(LIBUUID_INCLUDE_DIRS ${LIBUUID_INCLUDE_DIR})
mark_as_advanced(LIBUUID_INCLUDE_DIR LIBUUID_LIBRARY)
