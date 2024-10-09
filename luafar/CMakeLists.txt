
project(luafar)

add_executable (minilua src/minilua.c)
target_include_directories(minilua PRIVATE ${LIBLUA_INCLUDE_DIRS})
target_link_libraries(minilua PRIVATE ${LIBLUA_LIBRARIES})

set(FARSRC "${CMAKE_SOURCE_DIR}/far/src")
set(SDK "${CMAKE_SOURCE_DIR}/far/far2sdk")
set(MAKEFLAGS "${CMAKE_CURRENT_SOURCE_DIR}/src/makeflags.lua")

add_custom_command(
    OUTPUT  farflags.c    # Treated as relative to CMAKE_CURRENT_BINARY_DIR
    COMMAND minilua ${MAKEFLAGS} ${SDK} > farflags.c
    DEPENDS ${SDK}/farplug-wide.h ${SDK}/farcolor.h ${SDK}/farkeys.h ${MAKEFLAGS} ${FARSRC}/DlgGuid.hpp
)

set(SOURCES
  ${CMAKE_CURRENT_BINARY_DIR}/farflags.c
  src/bit64.c
  src/exported.c
  src/lregex.c
  src/luamacro.c
  src/lusercontrol.c
  src/lutf8lib.c
  src/mytimer.c
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

add_library (${PROJECT_NAME} SHARED ${SOURCES})
set_property(TARGET ${PROJECT_NAME} PROPERTY C_STANDARD 11)
target_include_directories(${PROJECT_NAME} PRIVATE ${LIBLUA_INCLUDE_DIRS})

if (LibUuid_FOUND)
  set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DHAS_UUID")
  target_include_directories(${PROJECT_NAME} PRIVATE ${LIBUUID_INCLUDE_DIRS})
  target_link_libraries(${PROJECT_NAME} ${LIBUUID_LIBRARIES} utils)
else()
  target_link_libraries(${PROJECT_NAME} utils)
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
