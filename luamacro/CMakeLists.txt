
project(luamacro)

if (NOT DEFINED USE_LUAJIT OR USE_LUAJIT)
  include_directories("/usr/include/luajit-2.1")
else()
  include_directories("/usr/include/lua5.1")
endif()

set(SYS_ID 0x4EBBEFC8)

set(SOURCES
  ${CMAKE_SOURCE_DIR}/luafar/src/luaplug.c
)

add_library (luamacro MODULE ${SOURCES})
set_property(TARGET luamacro PROPERTY C_STANDARD 11)

set(EXPORTS
  CLOSEPLUGIN COMPARE DELETEFILES GETFILES GETFINDDATA
  GETOPENPLUGININFO PROCESSHOSTFILE PROCESSEVENT PROCESSKEY PUTFILES
  SETDIRECTORY SETFINDLIST EXITFAR OPENPLUGIN OPENFILEPLUGIN PROCESSEDITOREVENT
  PROCESSEDITORINPUT PROCESSVIEWEREVENT PROCESSDIALOGEVENT PROCESSCONSOLEINPUT
)

list(TRANSFORM EXPORTS PREPEND "-DEXPORT_")

target_link_libraries(luamacro PRIVATE luafar2l)

target_compile_definitions(luamacro PRIVATE ${EXPORTS} "-DSYS_ID=${SYS_ID}")

target_include_directories(luamacro PRIVATE ../far2l/far2sdk)
target_include_directories(luamacro PRIVATE ../WinPort)

set(INSTALL_DIR "${INSTALL_DIR}/Plugins/luafar/luamacro")

set_target_properties(luamacro
  PROPERTIES
    LIBRARY_OUTPUT_DIRECTORY "${INSTALL_DIR}/plug"
    LINK_FLAGS "-Wl,-rpath,../../"
    PREFIX ""
    SUFFIX ".far-plug-wide")

set(MY_LIST
  luamacro.example.ini
  lm_bel.lng
  lm_eng.lng
  lm_ita.lng
  lm_pol.lng
  lm_rus.lng
  lm_sky.lng
  lm_spa.lng
  api.lua
  farapi.lua
  keymacro.lua
  lang.lua
  luamacro.lua
  macrotest.lua
  mbrowser.lua
  opcodes.lua
  panelsort.lua
  settings.lua
  utils.lua
  winapi.lua
)

foreach(elem ${MY_LIST})
  configure_file("src/${elem}" "${INSTALL_DIR}/plug/${elem}" COPYONLY)
endforeach()
