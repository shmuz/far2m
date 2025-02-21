project(hlfviewer)

set(TOOLS "${CMAKE_BINARY_DIR}/tools" CACHE STRING "")
set(FARLUATOOL "${TOOLS}/lua")

include_directories(${LIBLUA_INCLUDE_DIRS})

set(SOURCES
  src/GlobalInfo.c
  ${CMAKE_SOURCE_DIR}/luafar/src/luaplug.c
)

add_library (${PROJECT_NAME} MODULE ${SOURCES})
set_property(TARGET ${PROJECT_NAME} PROPERTY C_STANDARD 11)

set(EXPNAMES
  OPENPLUGIN PROCESSEDITORINPUT CONFIGURE
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

if (${CMAKE_SYSTEM_NAME} MATCHES "Android")
    target_link_libraries(${PROJECT_NAME} PUBLIC luafar)
    set(LINK_FLAGS "-Wl,-rpath,../../")
else()
    set(LINK_FLAGS "")
endif()

set_target_properties(${PROJECT_NAME}
  PROPERTIES
    LIBRARY_OUTPUT_DIRECTORY "${INSTALL_DIR}/plug"
    LINK_FLAGS "${LINK_FLAGS}"
    PREFIX ""
    SUFFIX ".far-plug-wide")

set(MY_LIST
  hlfviewer.lua
  HlfViewerUkr.hlf
  HlfViewerRus.hlf
  HlfViewerPol.hlf
  HlfViewerEng.hlf
  LICENSE
)

foreach(elem ${MY_LIST})
  configure_file("src/${elem}" "${INSTALL_DIR}/plug/${elem}" COPYONLY)
endforeach()

set (MAKELANG ${FARLUATOOL} ${CMAKE_SOURCE_DIR}/luafar/lua_share/makelang.lua)
set (TEMPL    ${CMAKE_CURRENT_SOURCE_DIR}/src/hv_lang.templ)
add_custom_target(${PROJECT_NAME}_lang ALL
  ${MAKELANG} ${TEMPL} ${INSTALL_DIR}/plug
)
