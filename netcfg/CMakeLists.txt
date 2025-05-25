project(netcfg)

get_directory_property(PROJECT_SOURCE_DIR DIRECTORY ../ DEFINITION PROJECT_SOURCE_DIR)
get_directory_property(INSTALL_DIR DIRECTORY ${PROJECT_SOURCE_DIR} DEFINITION INSTALL_DIR)

message(STATUS "${PROJECT_NAME} PROJECT_SOURCE_DIR ${PROJECT_SOURCE_DIR} ${CMAKE_SYSTEM_NAME}")

set(SOURCES
src/GlobalInfo.cpp
src/netcfgplugin.cpp
src/netif/netif.cpp
src/netif/netifs.cpp
src/netroute/netroute.cpp
src/netroute/netroutes.cpp
src/fardialog.cpp
src/netcfgplugin.cpp
src/netcfginterfaces.cpp
src/netcfgroutes.cpp
src/netcfgiproutes.cpp
src/netcfgarp.cpp
src/farpanel.cpp
src/netfarpanel.cpp
src/plugincfg.cpp
src/farconnect.cpp
src/common/errname.c
src/common/log.c
src/common/netutils.c
src/common/sizestr.c
)

if (${CMAKE_SYSTEM_NAME} MATCHES "Linux")
set(SOURCES
${SOURCES}
src/netcfgiptables.cpp
src/netcfgrules.cpp
src/netcfgarp.cpp
src/common/netlink.c
)
endif()

set(CMAKE_CXX_STANDARD 17)
# set(CMAKE_C_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall")

# GCC 6+ has a warning for an ABI change due to a bug introduced in GCC 5:
# http://gcc.gnu.org/bugzilla/show_bug.cgi?id=77728. As we are building all of
# drcachesim and not linking to other C++ code, we can just ignore it.
if (ARM AND CMAKE_COMPILER_IS_GNUCC)
    include(CheckCXXCompilerFlag)
    CHECK_CXX_COMPILER_FLAG(-Wno-psabi GCC_HAS_NO_PSABI)
    if (GCC_HAS_NO_PSABI)
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-psabi")
    endif (GCC_HAS_NO_PSABI)
endif ()

add_library(${PROJECT_NAME} MODULE ${SOURCES})
target_link_libraries(${PROJECT_NAME} utils far2m)

target_compile_definitions(${PROJECT_NAME} PRIVATE -DUSEUCD=OFF -DWINPORT_DIRECT -DUNICODE -DFAR_DONT_USE_INTERNALS)

target_include_directories(${PROJECT_NAME} PRIVATE src)
target_include_directories(${PROJECT_NAME} PRIVATE ${PROJECT_SOURCE_DIR}/utils/include)
target_include_directories(${PROJECT_NAME} PRIVATE ${PROJECT_SOURCE_DIR}/far/far2sdk)
target_include_directories(${PROJECT_NAME} PRIVATE ${PROJECT_SOURCE_DIR}/WinPort)

set_target_properties(${PROJECT_NAME}
    PROPERTIES
        LIBRARY_OUTPUT_DIRECTORY "${INSTALL_DIR}/Plugins/${PROJECT_NAME}/plug"
        PREFIX ""
        SUFFIX ".far-plug-wide")


# copy resource files
set(CURRENT_TARGET "netcfg")
file(GLOB_RECURSE RESOURCE_FILES
        RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}/configs
        "${CMAKE_CURRENT_SOURCE_DIR}/configs/*"
)
setup_target_auxiliary_files(${CURRENT_TARGET}
        RESOURCE_FILES
        ${CMAKE_CURRENT_SOURCE_DIR}/configs
        "${INSTALL_DIR}/Plugins/${CURRENT_TARGET}/"
)
add_dependencies(${CURRENT_TARGET} copy_aux_files_for_${CURRENT_TARGET})
