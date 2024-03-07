
project(luamacro)

include_directories(${LIBLUA_INCLUDE_DIRS})

set(SOURCES
  src/GlobalInfo.c
  ${CMAKE_SOURCE_DIR}/luafar/src/luaplug.c
)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DNOSETPACKAGEPATH")

add_library (${PROJECT_NAME} MODULE ${SOURCES})
set_property(TARGET ${PROJECT_NAME} PROPERTY C_STANDARD 11)

set(EXPNAMES
  CLOSEPLUGIN COMPARE CONFIGURE DELETEFILES GETFILES GETFINDDATA GETLINKTARGET
  MAYEXITFAR GETOPENPLUGININFO PROCESSHOSTFILE PROCESSEVENT PROCESSKEY PUTFILES
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

###  Generate constants.lua ----------------------------------------------------
set(TRG    "${INSTALL_DIR}/plug/constants.lua")
set(SCRIPT ${CMAKE_CURRENT_SOURCE_DIR}/generators/make_const.lua)
set(FAR    ${CMAKE_SOURCE_DIR}/far)
set(MACR   ${FAR}/src/macro)
set(INCL   ${FAR} ${MACR} ${FAR}/src/console ${CMAKE_SOURCE_DIR}/WinPort)
set(INPUT  ${MACR}/macroopcode.hpp ${MACR}/macrovalues.hpp)

set(GEN gen_const)
set(SRC ${GEN}.cpp)

add_custom_command(OUTPUT ${SRC}
                   COMMAND minilua ${SCRIPT} ${SRC} ${INPUT}
                   DEPENDS ${INPUT}
                   )

add_executable(${GEN} ${SRC})
target_include_directories(${GEN} PRIVATE ${INCL})

add_custom_target(luamacro_const ALL ${GEN} ${TRG})
###-----------------------------------------------------------------------------

set(MY_LIST
  luamacro.example.ini
  api.lua
  farapi.lua
  farcmds.lua
  keymacro.lua
  luamacro.lua
  macrotest.lua
  mbrowser.lua
  panelsort.lua
  settings.lua
  utils.lua
  winapi.lua
)

foreach(elem ${MY_LIST})
  configure_file("src/${elem}" "${INSTALL_DIR}/plug/${elem}" COPYONLY)
endforeach()

set (MAKELANG minilua ${CMAKE_SOURCE_DIR}/luafar/lua_share/makelang.lua)
set (TEMPL    ${CMAKE_CURRENT_SOURCE_DIR}/src/lm_lang.templ)
add_custom_target(${PROJECT_NAME}_lang ALL
  ${MAKELANG} ${TEMPL} ${INSTALL_DIR}/plug
)
