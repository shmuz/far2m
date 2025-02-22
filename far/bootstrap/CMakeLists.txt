cmake_minimum_required (VERSION 3.5)

# Make TOOLS folder overridable in command line when configuring build.
# This will allow specifying host far2m tools location when doing cross-platform builds.
# Cross compile of far2m requires tools to be pre-compiled for host architecture.
set(TOOLS "${CMAKE_BINARY_DIR}/tools" CACHE STRING "")
set(FARLUATOOL "${TOOLS}/lua")

add_custom_target(bootstrap
  DEPENDS lua copyright.inc farversion.inc farlang.templ
)

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

set(LUACMD
  ${FARLUATOOL}
  "-eARCH=[[${CMAKE_SYSTEM_NAME} ${CMAKE_SYSTEM_PROCESSOR}]]"
  "-eMAJOR=${VERSION_MAJOR}"
  "-eMINOR=${VERSION_MINOR}"
  "-ePATCH=[[${VERSION_PATCH}]]"
  "${SCRIPTS}/bootstrap.lua"
)

add_custom_command(OUTPUT "${BOOTSTRAP}/copyright.inc"
    COMMAND ${LUACMD} --copyright > "${BOOTSTRAP}/copyright.inc"
    WORKING_DIRECTORY "${SCRIPTS}"
    DEPENDS ${DEPENDENCIES}
    COMMENT "generating copyright header"
)

add_custom_command(OUTPUT "${BOOTSTRAP}/farversion.inc"
    COMMAND ${LUACMD} --farversion > "${BOOTSTRAP}/farversion.inc"
    WORKING_DIRECTORY "${SCRIPTS}"
    DEPENDS ${DEPENDENCIES}
    COMMENT "generating farversion header"
)

add_custom_command(OUTPUT "${BOOTSTRAP}/farlang.templ"
    COMMAND ${LUACMD} --farlang "${SCRIPTS}/farlang.templ.m4" > "${BOOTSTRAP}/farlang.templ"
    WORKING_DIRECTORY "${SCRIPTS}"
    DEPENDS ${DEPENDENCIES}
    COMMENT "generating language template"
)

add_custom_command(TARGET bootstrap
   COMMAND ${LUACMD} --mkhlf "${SCRIPTS}/FarEng.hlf.m4" > "${BOOTSTRAP}/FarEng.hlf"
   WORKING_DIRECTORY "${SCRIPTS}"
   DEPENDS ${DEPENDENCIES}
   COMMENT "generating help eng"
)

add_custom_command(TARGET bootstrap
   COMMAND ${LUACMD} --mkhlf "${SCRIPTS}/FarRus.hlf.m4" > "${BOOTSTRAP}/FarRus.hlf"
   WORKING_DIRECTORY "${SCRIPTS}"
   DEPENDS ${DEPENDENCIES}
   COMMENT "generating help rus"
)

add_custom_command(TARGET bootstrap
   COMMAND ${LUACMD} --mkhlf "${SCRIPTS}/FarHun.hlf.m4" > "${BOOTSTRAP}/FarHun.hlf"
   WORKING_DIRECTORY "${SCRIPTS}"
   DEPENDS ${DEPENDENCIES}
   COMMENT "generating help hun"
)

add_custom_command(TARGET bootstrap
   COMMAND ${LUACMD} --mkhlf "${SCRIPTS}/FarUkr.hlf.m4" > "${BOOTSTRAP}/FarUkr.hlf"
   WORKING_DIRECTORY "${SCRIPTS}"
   DEPENDS ${DEPENDENCIES}
   COMMENT "generating help ukr"
)

add_custom_command(TARGET bootstrap
   COMMAND "${FARLUATOOL}" "${SCRIPTS}/farlang.lua" -i "${BOOTSTRAP}/farlang.templ" -o "${BOOTSTRAP}"
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
