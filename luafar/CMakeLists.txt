
project(luafar)

find_library(LUALIB NAMES luajit-5.1 lua5.1)
if (${LUALIB} STREQUAL LUALIB-NOTFOUND)
  message(STATUS "${ColorRed}Neither luajit nor lua5.1 was found${ColorNormal}")
  return()
endif()

FindLuaIncludes()
if (DEFINED LUAINC)
  include_directories(${LUAINC})
else()
  return()
endif()

add_executable (minilua src/minilua.c)
target_link_libraries(minilua PRIVATE ${LUALIB})

set(SDK "${CMAKE_SOURCE_DIR}/far/far2sdk")
set(MAKEFLAGS "${CMAKE_CURRENT_SOURCE_DIR}/src/makeflags.lua")

add_custom_command(
    OUTPUT  farflags.c    # Treated as relative to CMAKE_CURRENT_BINARY_DIR
    COMMAND minilua ${MAKEFLAGS} ${SDK} > farflags.c
    DEPENDS ${SDK}/farplug-wide.h ${SDK}/farcolor.h ${SDK}/farkeys.h ${MAKEFLAGS}
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
  src/ustring.c
  src/util.c
  src/farhost.cpp
)
if (NOT ${CMAKE_SYSTEM_NAME} MATCHES "FreeBSD")
  list(APPEND SOURCES src/mytimer.c)
endif()

add_library (${PROJECT_NAME} SHARED ${SOURCES})
set_property(TARGET ${PROJECT_NAME} PROPERTY C_STANDARD 11)

if (${CMAKE_SYSTEM_NAME} MATCHES "FreeBSD")
  target_link_libraries(${PROJECT_NAME} utils)
else()
  target_link_libraries(${PROJECT_NAME} uuid utils)
endif()

target_compile_definitions(${PROJECT_NAME} PRIVATE -DWINPORT_DIRECT -DFAR_DONT_USE_INTERNALS)

target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_SOURCE_DIR}/far/far2sdk)
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
