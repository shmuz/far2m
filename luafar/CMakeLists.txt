
project(luafar)

set(TOOLS "${CMAKE_BINARY_DIR}/tools" CACHE STRING "")
set(FARLUATOOL "${TOOLS}/lua")

set(FARSRC "${CMAKE_SOURCE_DIR}/far/src")
set(SDK "${CMAKE_SOURCE_DIR}/far/far2sdk")
set(MAKEFLAGS "${CMAKE_CURRENT_SOURCE_DIR}/src/makeflags.lua")

add_custom_command(
    OUTPUT  farflags.c    # Treated as relative to CMAKE_CURRENT_BINARY_DIR
    COMMAND ${FARLUATOOL} ${MAKEFLAGS} ${SDK} > farflags.c
    DEPENDS ${SDK}/farplug-wide.h ${SDK}/farcolor.h ${SDK}/farkeys.h ${MAKEFLAGS} ${FARSRC}/DlgGuid.hpp
    DEPENDS ${FARLUATOOL}
)

set(SOURCES
  ${CMAKE_CURRENT_BINARY_DIR}/farflags.c
  src/bit64.c
  src/exported.c
  src/lregex.c
  src/luamacro.c
  src/lusercontrol.c
  src/lutf8lib.c
  src/service.c
  src/slnunico.c
  src/sysutils.c
  src/ustring.c
  src/util.c
  src/win.c
  src/farhost.cpp
  src/farlibs.cpp
  src/LPeg/lpcap.c
  src/LPeg/lpcode.c
  src/LPeg/lpcset.c
  src/LPeg/lpprint.c
  src/LPeg/lptree.c
  src/LPeg/lpvm.c
)

if (NOT (${CMAKE_SYSTEM_NAME} MATCHES "DragonFly" OR
         ${CMAKE_SYSTEM_NAME} MATCHES "Android" OR
         ${CMAKE_SYSTEM_NAME} MATCHES "Darwin"))
  set(SOURCES ${SOURCES} src/mytimer.c)
endif()

add_library (${PROJECT_NAME} SHARED ${SOURCES})
set_property(TARGET ${PROJECT_NAME} PROPERTY C_STANDARD 11)
target_include_directories(${PROJECT_NAME} PRIVATE ${LIBLUA_INCLUDE_DIRS})

target_link_libraries(${PROJECT_NAME} utils)

if (${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
  target_link_libraries(${PROJECT_NAME} far2m ${LIBLUA_LIBRARIES})
endif()

if (LibUuid_FOUND)
  set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DHAS_UUID")
  target_include_directories(${PROJECT_NAME} PRIVATE ${LIBUUID_INCLUDE_DIRS})
  target_link_libraries(${PROJECT_NAME} ${LIBUUID_LIBRARIES})
endif()

target_compile_definitions(${PROJECT_NAME} PRIVATE -DWINPORT_DIRECT -DFAR_DONT_USE_INTERNALS)

target_include_directories(${PROJECT_NAME} PRIVATE ${SDK})
target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_SOURCE_DIR}/WinPort)

set_target_properties(${PROJECT_NAME}
  PROPERTIES
    LIBRARY_OUTPUT_DIRECTORY "${INSTALL_DIR}/Plugins/luafar"
    PREFIX ""
    SUFFIX ".so")

foreach(elem "luafar2m_manual.chm" "macroapi_manual_linux.chm")
  configure_file("manuals/${elem}" "${INSTALL_DIR}/Manuals/${elem}" COPYONLY)
endforeach()

add_custom_target(lua_share ALL
  COMMAND "cp" "-rf" "${CMAKE_CURRENT_SOURCE_DIR}/lua_share" "${INSTALL_DIR}/Plugins/luafar/"
)

set(LUAFAR_OK "OK" PARENT_SCOPE)
