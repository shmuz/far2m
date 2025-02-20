cmake_minimum_required (VERSION 3.5)

# Make TOOLS folder overridable in command line when configuring build.
# This will allow specifying host far2m tools location when doing cross-platform builds.
# Cross compile of far2m requires tools to be pre-compiled for host architecture.
set(TOOLS "${CMAKE_BINARY_DIR}/tools" CACHE STRING "")
set(FARLNGTOOL "${TOOLS}/farlng")

add_custom_target(bootstrap
  DEPENDS farlng copyright.inc farversion.inc farlang.templ
)

message(STATUS "generating Lua")

add_executable (farlua
  lua/src/lua.c
  lua/src/lapi.c
  lua/src/lcode.c
  lua/src/ldebug.c
  lua/src/ldo.c
  lua/src/ldump.c
  lua/src/lfunc.c
  lua/src/lgc.c
  lua/src/llex.c
  lua/src/lmem.c
  lua/src/lobject.c
  lua/src/lopcodes.c
  lua/src/lparser.c
  lua/src/lstate.c
  lua/src/lstring.c
  lua/src/ltable.c
  lua/src/ltm.c
  lua/src/lundump.c
  lua/src/lvm.c
  lua/src/lzio.c
  lua/src/lauxlib.c
  lua/src/lbaselib.c
  lua/src/ldblib.c
  lua/src/liolib.c
  lua/src/lmathlib.c
  lua/src/loslib.c
  lua/src/ltablib.c
  lua/src/lstrlib.c
  lua/src/loadlib.c
  lua/src/linit.c
)
target_link_libraries(farlua PRIVATE m)

message(STATUS "generating headers and languages")

# todo: detect architecture 64 or 32
# LBITS := $(shell getconf LONG_BIT)

set(DIRBIT 64)

set(BOOTSTRAP "${PROJECT_BINARY_DIR}/bootstrap")
set(SCRIPTS "${CMAKE_CURRENT_SOURCE_DIR}/scripts")
set(DEPENDENCIES
    "${SCRIPTS}/farlang.templ.m4"
    "${SCRIPTS}/bootstrap.lua"
)

set(HLFFILES
    "${BOOTSTRAP}/FarEng.hlf"
    "${BOOTSTRAP}/FarRus.hlf"
    "${BOOTSTRAP}/FarHun.hlf"
    "${BOOTSTRAP}/FarUkr.hlf"
)
set(LNGFILES
    "${BOOTSTRAP}/FarEng.lng"
    "${BOOTSTRAP}/FarRus.lng"
    "${BOOTSTRAP}/FarHun.lng"
    "${BOOTSTRAP}/FarUkr.lng"
)

set(LUA
  farlua
  "-eARCH=[[${CMAKE_SYSTEM_NAME} ${CMAKE_SYSTEM_PROCESSOR}]]"
  "-eMAJOR=${VERSION_MAJOR}"
  "-eMINOR=${VERSION_MINOR}"
  "-ePATCH=[[${VERSION_PATCH}]]"
  "${SCRIPTS}/bootstrap.lua"
)

add_custom_command(OUTPUT "${BOOTSTRAP}/copyright.inc"
    COMMAND ${LUA} --copyright > "${BOOTSTRAP}/copyright.inc"
    WORKING_DIRECTORY "${SCRIPTS}"
    DEPENDS ${DEPENDENCIES}
    COMMENT "generating copyright header"
)

add_custom_command(OUTPUT "${BOOTSTRAP}/farversion.inc"
    COMMAND ${LUA} --farversion > "${BOOTSTRAP}/farversion.inc"
    WORKING_DIRECTORY "${SCRIPTS}"
    DEPENDS ${DEPENDENCIES}
    COMMENT "generating farversion header"
)

add_custom_command(OUTPUT "${BOOTSTRAP}/farlang.templ"
    COMMAND ${LUA} --farlang "${SCRIPTS}/farlang.templ.m4" > "${BOOTSTRAP}/farlang.templ"
    WORKING_DIRECTORY "${SCRIPTS}"
    DEPENDS ${DEPENDENCIES}
    COMMENT "generating language template"
)

add_custom_command(TARGET bootstrap
   COMMAND ${LUA} --mkhlf "${SCRIPTS}/FarEng.hlf.m4" > "${BOOTSTRAP}/FarEng.hlf"
   WORKING_DIRECTORY "${SCRIPTS}"
   DEPENDS ${DEPENDENCIES}
   COMMENT "generating help eng"
)

add_custom_command(TARGET bootstrap
   COMMAND ${LUA} --mkhlf "${SCRIPTS}/FarRus.hlf.m4" > "${BOOTSTRAP}/FarRus.hlf"
   WORKING_DIRECTORY "${SCRIPTS}"
   DEPENDS ${DEPENDENCIES}
   COMMENT "generating help rus"
)

add_custom_command(TARGET bootstrap
   COMMAND ${LUA} --mkhlf "${SCRIPTS}/FarHun.hlf.m4" > "${BOOTSTRAP}/FarHun.hlf"
   WORKING_DIRECTORY "${SCRIPTS}"
   DEPENDS ${DEPENDENCIES}
   COMMENT "generating help hun"
)

add_custom_command(TARGET bootstrap
   COMMAND ${LUA} --mkhlf "${SCRIPTS}/FarUkr.hlf.m4" > "${BOOTSTRAP}/FarUkr.hlf"
   WORKING_DIRECTORY "${SCRIPTS}"
   DEPENDS ${DEPENDENCIES}
   COMMENT "generating help ukr"
)

add_custom_command(TARGET bootstrap
   COMMAND "${FARLNGTOOL}" generator -nc -i "${BOOTSTRAP}/lang.ini" -ol "${BOOTSTRAP}" "${BOOTSTRAP}/farlang.templ"
   WORKING_DIRECTORY "${BOOTSTRAP}"
   DEPENDS "${BOOTSTRAP}/farlang.templ"
   COMMENT "generating languages"
)

# older cmake versions does not support copying of multiple files,
# so we use simple stupid /bin/cp and wildcards

add_custom_command(TARGET bootstrap
    POST_BUILD
    DEPENDS "${LNGFILES}"
    COMMAND "cp" "${BOOTSTRAP}/*.lng" "${INSTALL_DIR}"
)

add_custom_command(TARGET bootstrap
    POST_BUILD
    DEPENDS "${HLFFILES}"
    COMMAND "cp" "${BOOTSTRAP}/*.hlf" "${INSTALL_DIR}"
)

add_custom_command(TARGET bootstrap
    POST_BUILD
    COMMAND "cp" "${CMAKE_CURRENT_SOURCE_DIR}/*.sh" "${INSTALL_DIR}"
    COMMAND "cp" "${CMAKE_CURRENT_SOURCE_DIR}/*.vbs" "${INSTALL_DIR}"
    COMMAND "cp" "${CMAKE_CURRENT_SOURCE_DIR}/*.ini" "${INSTALL_DIR}"
)
