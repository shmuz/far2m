
project(luamacro)

FindLuaIncludes()
if (DEFINED LUAINC)
  include_directories(${LUAINC})
else()
  return()
endif()

set(SOURCES
  ${CMAKE_SOURCE_DIR}/luafar/src/luaplug.c
)

add_library (${PROJECT_NAME} MODULE ${SOURCES})
set_property(TARGET ${PROJECT_NAME} PROPERTY C_STANDARD 11)

set(EXPNAMES
  CLOSEPLUGIN COMPARE CONFIGURE DELETEFILES GETFILES GETFINDDATA MAYEXITFAR
  GETOPENPLUGININFO PROCESSHOSTFILE PROCESSEVENT PROCESSKEY PUTFILES
  SETDIRECTORY SETFINDLIST EXITFAR OPENPLUGIN OPENFILEPLUGIN PROCESSEDITOREVENT
  PROCESSEDITORINPUT PROCESSVIEWEREVENT PROCESSDIALOGEVENT PROCESSCONSOLEINPUT
)

set(EXPORTS "")
foreach(f ${EXPNAMES})
  list(APPEND EXPORTS "-DEXPORT_${f}")
endforeach()

target_compile_definitions(${PROJECT_NAME}
  PRIVATE ${EXPORTS}
  PRIVATE "-DSYS_ID=0x4EBBEFC8"
  PRIVATE "-DPLUG_VERSION=3,0,0,765"
  PRIVATE "-DPLUG_TITLE=\"LuaMacro\""
  PRIVATE "-DPLUG_DESCRIPTION=\"Far macros in Lua\""
  PRIVATE "-DPLUG_AUTHOR=\"Shmuel Zeigerman & Far Group\""
)

target_include_directories(${PROJECT_NAME} PRIVATE ../far/far2sdk)
target_include_directories(${PROJECT_NAME} PRIVATE ../WinPort)

set(INSTALL_DIR "${INSTALL_DIR}/Plugins/luafar/luamacro")

set_target_properties(${PROJECT_NAME}
  PROPERTIES
    LIBRARY_OUTPUT_DIRECTORY "${INSTALL_DIR}/plug"
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
