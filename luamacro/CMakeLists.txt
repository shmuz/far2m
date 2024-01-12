
project(luamacro)

include_directories(${LIBLUA_INCLUDE_DIRS})

set(SOURCES
  src/GlobalInfo.c
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
)

target_include_directories(${PROJECT_NAME} PRIVATE ../far/far2sdk)

set(INSTALL_DIR "${INSTALL_DIR}/Plugins/luafar/${PROJECT_NAME}")

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
  farcmds.lua
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
