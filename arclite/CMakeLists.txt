
project(arclite)

set(SOURCES
    src/GlobalInfo.cpp
    src/archive.cpp
	src/attr.cpp
	src/cmdline.cpp
	src/common.cpp
    src/headers.cpp
	src/delete.cpp
    src/extract.cpp
	src/farutils.cpp
    src/plugin.cpp
	src/msearch.cpp
	src/open.cpp
	src/options.cpp
	src/Patch7zCP.cpp
	src/pathutils.cpp
	src/rsrc.cpp
	src/sfx.cpp
	src/SimpleXML.cpp
	src/strutils.cpp
	src/sysutils.cpp
	src/ui.cpp
	src/update.cpp
	src/test.cpp
    src/MyWindows.cpp
)

#set_property(GLOBAL PROPERTY RULE_LAUNCH_COMPILE "${CMAKE_COMMAND} -E time")
#set_property(GLOBAL PROPERTY RULE_LAUNCH_LINK "${CMAKE_COMMAND} -E time")

add_library (arclite MODULE ${SOURCES})

target_compile_definitions(arclite PRIVATE -DNDEBUG)

target_link_libraries(arclite utils far2m)

target_include_directories(arclite PRIVATE
    ../WinPort
    ../far/far2sdk
    ../far/src/base
)

target_precompile_headers(arclite PRIVATE src/headers.hpp)
set_target_properties(arclite
    PROPERTIES
        LIBRARY_OUTPUT_DIRECTORY "${INSTALL_DIR}/Plugins/arclite/plug"
        PREFIX ""
        SUFFIX ".far-plug-wide")

add_custom_command(TARGET arclite POST_BUILD
    DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/configs
    COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_CURRENT_SOURCE_DIR}/configs "${INSTALL_DIR}/Plugins/arclite")
