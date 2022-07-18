
project(luafar2l)

if (NOT DEFINED USE_LUAJIT OR USE_LUAJIT)
  include_directories("/usr/include/luajit-2.1")
  set(LIB_LUA "luajit-5.1")
  set(LUAEXE  "luajit")
else()
  include_directories("/usr/include/lua5.1")
  set(LIB_LUA "lua5.1")
  set(LUAEXE  "lua")
endif()

set(SDK "${CMAKE_SOURCE_DIR}/far2l/far2sdk")

function(GenerateFile trg src script)
  add_custom_command(
      OUTPUT  ${trg}          # Treated as relative to CMAKE_CURRENT_BINARY_DIR
      COMMAND ${LUAEXE} ${CMAKE_CURRENT_SOURCE_DIR}/src/${script} ${SDK}/${src}
              > ${CMAKE_CURRENT_BINARY_DIR}/${trg}
      DEPENDS ${SDK}/${src}
  )
endfunction()

GenerateFile("farkeys.c"  "farkeys.h"      "makefarkeys.lua" )
GenerateFile("farcolor.c" "farcolor.h"     "makefarkeys.lua")
GenerateFile("farflags.c" "farplug-wide.h" "makeflags.lua")

set(SOURCES
  ${CMAKE_CURRENT_BINARY_DIR}/farkeys.c
  ${CMAKE_CURRENT_BINARY_DIR}/farcolor.c
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
  src/ustring.c
  src/util.c
)

add_library (luafar2l SHARED ${SOURCES})
set_property(TARGET luafar2l PROPERTY C_STANDARD 11)
target_link_libraries(luafar2l uuid)
# target_link_libraries(luafar2l ${LIB_LUA} uuid)

target_compile_definitions(luafar2l PRIVATE -DWINPORT_DIRECT -DFAR_DONT_USE_INTERNALS)

target_include_directories(luafar2l PRIVATE ../far2l/far2sdk)
target_include_directories(luafar2l PRIVATE ../WinPort)

set_target_properties(luafar2l
  PROPERTIES
    LIBRARY_OUTPUT_DIRECTORY "${INSTALL_DIR}/Plugins/luafar"
    PREFIX ""
    SUFFIX ".so")

set(MY_LIST
  luafar2l_manual.chm
  macroapi_manual_linux.chm
)

foreach(elem ${MY_LIST})
  configure_file("manuals/${elem}" "${INSTALL_DIR}/Manuals/${elem}" COPYONLY)
endforeach()
