message(STATUS "generating headers and languages")

if (CMAKE_SIZEOF_VOID_P EQUAL 8)
    set(LBITS 64)
    set(DIRBIT 64)
else ()
    set(LBITS 32)
    set(DIRBIT 32)
endif ()

# Make TOOLS folder overridable in command line when configuring build.
# This will allow specifying host far2m tools location when doing cross-platform builds.
# Cross compile of far2m requires tools to be pre-compiled for host architecture.
set(TOOLS "${CMAKE_BINARY_DIR}/tools" CACHE STRING "")
set(FARLUATOOL "${TOOLS}/lua")

set(BOOTSTRAP "${PROJECT_BINARY_DIR}/bootstrap")
set(SCRIPTS "${CMAKE_CURRENT_SOURCE_DIR}/scripts")

set(HLFFILES
    "${BOOTSTRAP}/FarEng.hlf"
    "${BOOTSTRAP}/FarRus.hlf"
    "${BOOTSTRAP}/FarHun.hlf"
    "${BOOTSTRAP}/FarUkr.hlf"
)

add_custom_target(bootstrap
  DEPENDS lua farversion.inc lang.inc ${HLFFILES}
)

set(DEPENDENCIES
    "${SCRIPTS}/farlang.templ.m4"
    "${SCRIPTS}/farlang.lua"
    "${SCRIPTS}/bootstrap.lua"
)

set(LUACMD
  ${FARLUATOOL}
  "-eARCH=[[${CMAKE_SYSTEM_NAME} ${CMAKE_SYSTEM_PROCESSOR}]]"
  "-eMAJOR=${VERSION_MAJOR}"
  "-eMINOR=${VERSION_MINOR}"
  "-ePATCH=[[${VERSION_PATCH}]]"
  "${SCRIPTS}/bootstrap.lua"
)

add_custom_command(OUTPUT "${BOOTSTRAP}/farversion.inc"
    COMMAND ${LUACMD} --farversion > "${BOOTSTRAP}/farversion.inc"
    WORKING_DIRECTORY "${SCRIPTS}"
    DEPENDS "${SCRIPTS}/bootstrap.lua"
    COMMENT "generating farversion and copyright header"
)

# List of langs for help files
set(LANGS "Eng" "Rus" "Hun" "Ukr")
foreach(LANG ${LANGS})
  add_custom_command(OUTPUT "${BOOTSTRAP}/Far${LANG}.hlf"
    COMMAND ${LUACMD} --mkhlf "${SCRIPTS}/Far${LANG}.hlf.m4" > "${BOOTSTRAP}/Far${LANG}.hlf"
    WORKING_DIRECTORY "${SCRIPTS}"
    DEPENDS "${SCRIPTS}/bootstrap.lua" "${SCRIPTS}/Far${LANG}.hlf.m4"
    COMMENT "generating help ${LANG}"
  )
endforeach()

add_custom_command(OUTPUT "${BOOTSTRAP}/farlang.templ"
   COMMAND ${LUACMD} --farlang "${SCRIPTS}/farlang.templ.m4" > "${BOOTSTRAP}/farlang.templ"
   WORKING_DIRECTORY "${BOOTSTRAP}"
   DEPENDS ${DEPENDENCIES}
   COMMENT "generating language template"
)

add_custom_command(OUTPUT "${BOOTSTRAP}/lang.inc"
   COMMAND "${FARLUATOOL}" "${SCRIPTS}/farlang.lua" -i "${BOOTSTRAP}/farlang.templ" -o "${BOOTSTRAP}" -h lang.inc -v
   WORKING_DIRECTORY "${BOOTSTRAP}"
   DEPENDS "${BOOTSTRAP}/farlang.templ"
   COMMENT "generating languages"
)

# older cmake versions does not support copying of multiple files,
# so we use simple stupid /bin/cp and wildcards

add_custom_command(TARGET bootstrap
    POST_BUILD
    COMMAND "cp" "${BOOTSTRAP}/*.lng" "${INSTALL_DIR}"
)

add_custom_command(TARGET bootstrap
    POST_BUILD
    COMMAND "cp" "${BOOTSTRAP}/*.hlf" "${INSTALL_DIR}"
)

add_custom_command(TARGET bootstrap
    POST_BUILD
    COMMAND "cp" "${CMAKE_CURRENT_SOURCE_DIR}/*.sh" "${INSTALL_DIR}"
    COMMAND "cp" "${CMAKE_CURRENT_SOURCE_DIR}/*.vbs" "${INSTALL_DIR}"
    COMMAND "cp" "${CMAKE_CURRENT_SOURCE_DIR}/*.ini" "${INSTALL_DIR}"
)
