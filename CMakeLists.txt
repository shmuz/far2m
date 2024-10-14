set(CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake/modules)

cmake_minimum_required (VERSION 3.5)

project (far2m)

set(APP_NAME "far2m")
set(APP_IDENTIFIER "com.far2m")
set(CPACK_PACKAGE_NAME "far2m")

string(ASCII 27 Esc)
set(ColorNormal "${Esc}[m")
set(ColorRed "${Esc}[1;31m")

include(GNUInstallDirs)

# just to force reconfigure if packaging/version changed
configure_file(${CMAKE_SOURCE_DIR}/packaging/version ${CMAKE_BINARY_DIR}/packaging/version @ONLY)

# reconfigure in case .git directory changed to update version correctly
IF(EXISTS ${CMAKE_SOURCE_DIR}/.git)
    set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS ${CMAKE_SOURCE_DIR}/.git)
endif()

file(READ "${CMAKE_SOURCE_DIR}/packaging/version" VERSION)
string(REGEX REPLACE "[\r\n\t ]$" "" VERSION "${VERSION}")
string(REGEX REPLACE "^([0-9]+)\\..*" "\\1" VERSION_MAJOR "${VERSION}")
string(REGEX REPLACE "^[0-9]+\\.([0-9]+).*" "\\1" VERSION_MINOR "${VERSION}")
string(REGEX REPLACE "^[0-9]+\\.[0-9]+\\.(.*)" "\\1" VERSION_PATCH "${VERSION}")

if("${VERSION_MAJOR}" STREQUAL "" OR "${VERSION_MAJOR}" STREQUAL "${VERSION}"
    OR "${VERSION_MINOR}" STREQUAL "" OR "${VERSION_MINOR}" STREQUAL "${VERSION}"
    OR "${VERSION_PATCH}" STREQUAL "" OR "${VERSION_PATCH}" STREQUAL "${VERSION}")
        message(FATAL_ERROR "${ColorRed}packaging/version is corrupted, please write actual X.Y.Z version into it.${ColorNormal}")
endif()

find_package(Git)

if(GIT_FOUND)
    execute_process(COMMAND "${GIT_EXECUTABLE}" describe --tag
        WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
        OUTPUT_VARIABLE GIT_TAG
        OUTPUT_STRIP_TRAILING_WHITESPACE)
endif()

if(NOT "${GIT_TAG}" STREQUAL "v_${VERSION}")
    if(GIT_FOUND)
        if(IS_DIRECTORY ${CMAKE_SOURCE_DIR}/.git)
            execute_process(COMMAND "${GIT_EXECUTABLE}" show --no-patch --format=%ci HEAD
                WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
                OUTPUT_VARIABLE GIT_DATE
                OUTPUT_STRIP_TRAILING_WHITESPACE)
            if(NOT "${GIT_DATE}" STREQUAL "")
                string(SUBSTRING ${GIT_DATE} 0 10 GIT_DATE)
                message(STATUS "git date: ${GIT_DATE}")
                set(VERSION_PATCH "${VERSION_PATCH}-${GIT_DATE}")
            endif()
            execute_process(COMMAND "${GIT_EXECUTABLE}" rev-parse --short HEAD
                WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
                OUTPUT_VARIABLE GIT_HASH
                OUTPUT_STRIP_TRAILING_WHITESPACE)
        endif()
    endif()
    if(NOT "${GIT_HASH}" STREQUAL "")
        message(STATUS "git hash: ${GIT_HASH}")
        set(VERSION_PATCH "${VERSION_PATCH}-${GIT_HASH}")
    else()
        message(WARNING "git information unavailable, assuming its a build from v_${VERSION}")
    endif()
endif()

set(VERSION_PATCH "${VERSION_PATCH}-beta")

set(VERSION "${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_PATCH}")

set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT CMAKE_BUILD_TYPE)
   set(CMAKE_BUILD_TYPE "Release")
endif()

message(STATUS "Build Type: ${CMAKE_BUILD_TYPE} Version: ${VERSION}")

if(NOT DEFINED CMAKE_INSTALL_PREFIX)
   if (${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
      set(CMAKE_INSTALL_PREFIX "/usr/local")
   elseif (${CMAKE_SYSTEM_NAME} MATCHES "FreeBSD")
      set(CMAKE_INSTALL_PREFIX "/usr/local")
   else()
      set(CMAKE_INSTALL_PREFIX "/usr")
   endif()
endif()

##############################

if (NOT DEFINED TESTING)
    set(TESTING "NO")
endif()

if (NOT DEFINED USEWX)
    set(USEWX "YES")
endif()

if (NOT DEFINED USELUA)
    set(USELUA "YES")
endif()

if (NOT DEFINED UNRAR)
    set(UNRAR "bundled")
endif()

# EACP disabled by default for now as FAR's GetCodePageInfo rejects all multibyte charsets due to BUGBUG.
# Uncomment EACP-stuff here when that will be fixed. See https://github.com/elfmz/far2l/issues/1179
#if (NOT DEFINED EACP)
#   set(EACP "YES")
#endif()

if (NOT DEFINED LEGACY)
    # TODO: default it to NO  somewhere in 2022, after all obsolete settings
    # registry-upgrade-to-ini stuff will be safe to remove
    set(LEGACY "YES")
endif()

#if (NOT EACP)
#   message(STATUS "${ColorRed}Building without East Asian codepages support due to EACP=${EACP}${ColorNormal}")
#endif()

# set(RM rm) actually not needed: use $(CMAKE_COMMAND) -E remove

if (${CMAKE_SYSTEM_NAME} MATCHES "FreeBSD")
    set(M4_NAME gm4)
else()
    set(M4_NAME m4)
endif()

find_program(M4_PROG NAMES ${M4_NAME})

if(NOT M4_PROG)
    message(FATAL_ERROR "${ColorRed}${M4_NAME} not found - please install it${ColorNormal}")
endif ()
set(M4 ${M4_PROG} -P "-DARCH=${CMAKE_SYSTEM_NAME} ${CMAKE_SYSTEM_PROCESSOR}" "-DMAJOR=${VERSION_MAJOR}" "-DMINOR=${VERSION_MINOR}" "-DPATCH=${VERSION_PATCH}")

# set(RM rm) actually not needed: use $(CMAKE_COMMAND) -E remove
find_program (GAWK NAMES gawk)
if(NOT GAWK)
  message(FATAL_ERROR "${ColorRed}gawk not found - please install it${ColorNormal}")
endif ()

if(${CMAKE_VERSION} VERSION_LESS "3.1.0")
    message("Please consider to switch to newer CMake")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
endif()

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -fPIC -Wno-unused-function -D_FILE_OFFSET_BITS=64") #  -fsanitize=address
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -std=c99 -fPIC -Wno-unused-function -D_FILE_OFFSET_BITS=64") #  -fsanitize=address
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS} -O2")
set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS} -O2")
set(TOOLS_DIR ${CMAKE_BINARY_DIR}/tools)

if(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
    set(APP_DIR ${CMAKE_BINARY_DIR}/install)
    set(INSTALL_DIR ${APP_DIR}/${APP_NAME}.app/Contents/MacOS)
    set(EXECUTABLE_NAME ${APP_NAME})

else()
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -ffunction-sections -fdata-sections")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -ffunction-sections -fdata-sections")
    set(INSTALL_DIR ${CMAKE_BINARY_DIR}/install)
    set(EXECUTABLE_NAME far2m)

    if (NOT CMAKE_CXX_COMPILER_ID MATCHES "Clang")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wl,--gc-sections")
        set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wl,--gc-sections")
    endif()
endif()

if (CMAKE_SYSTEM_NAME MATCHES "CYGWIN")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -D_GNU_SOURCE -D_BSD_SOURCE -D_XOPEN_SOURCE=600")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -D_GNU_SOURCE -D_BSD_SOURCE -D_XOPEN_SOURCE=600")
    message(STATUS "Running under cygwin, wow!")
endif()

if (CMAKE_SYSTEM_NAME MATCHES "Haiku")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -D_GNU_SOURCE -D_DEFAULT_SOURCE -Wno-attributes")
endif()

set(CMAKE_CXX_VISIBILITY_PRESET hidden)
set(CMAKE_C_VISIBILITY_PRESET hidden)

if (LEGACY)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DWINPORT_REGISTRY")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DWINPORT_REGISTRY")
    message(STATUS "Enabling registry support due to enabled LEGACY")
else()
    message(STATUS "LEGACY is not enabled, your old registry settings will be forgotten, if any")
endif()

if(CMAKE_VERSION VERSION_GREATER "3.3")
    cmake_policy(SET CMP0063 NEW)
    cmake_policy(SET CMP0057 NEW)
endif()

include_directories(utils/include)

add_subdirectory (farlng)
add_subdirectory (WinPort)
add_subdirectory (utils)
add_subdirectory (far)
add_dependencies (far2m farlng)

set_target_properties(farlng
  PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${TOOLS_DIR}"
)

if (NOT ${USEWX})
    message(STATUS "Building without GUI/wxWidgets backend due to USEWX=${USEWX}")
else()
    message(STATUS "Building with GUI/wxWidgets backend")
    if (${CMAKE_SYSTEM_NAME} MATCHES "FreeBSD")
        if (DEFINED wxWidgets_CONFIG_EXECUTABLE)
            message(STATUS "Using pre-defined wx config: ${wxWidgets_CONFIG_EXECUTABLE}")

        elseif(EXISTS "/usr/local/bin/wxgtk3u-3.2-config")
            set(wxWidgets_CONFIG_EXECUTABLE "/usr/local/bin/wxgtk3u-3.2-config")
            set(wxWidgets_wxrc_EXECUTABLE "/usr/local/bin/wxrc-gtk3u-3.2")
            message(STATUS "Using known location of wx-config 3.2")

        elseif(EXISTS "/usr/local/bin/wxgtk3u-3.1-config")
            set(wxWidgets_CONFIG_EXECUTABLE "/usr/local/bin/wxgtk3u-3.1-config")
            set(wxWidgets_wxrc_EXECUTABLE "/usr/local/bin/wxrc-gtk3u-3.1")
            message(STATUS "Using known location of wx-config 3.1")

        elseif (EXISTS "/usr/local/bin/wxgtk3u-3.0-config")
            set(wxWidgets_CONFIG_EXECUTABLE "/usr/local/bin/wxgtk3u-3.0-config")
            set(wxWidgets_wxrc_EXECUTABLE "/usr/local/bin/wxrc-gtk3u-3.0")
            message(STATUS "Using known location of wx-config 3.0")

        else()
            message(STATUS "No wxgtk3u-3.*-config in expected locations")
        endif()

    elseif (${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
        if (EXISTS "/opt/local/Library/Frameworks/wxWidgets.framework/Versions/wxWidgets/3.2/bin/wx-config")
            # MacPorts
            set(wxWidgets_CONFIG_EXECUTABLE "/opt/local/Library/Frameworks/wxWidgets.framework/Versions/wxWidgets/3.2/bin/wx-config")
            set(wxWidgets_wxrc_EXECUTABLE "/opt/local/Library/Frameworks/wxWidgets.framework/Versions/wxWidgets/3.2/bin/wxrc")
            message(STATUS "Using known location of wx-config 3.2")

        elseif (EXISTS "/opt/local/Library/Frameworks/wxWidgets.framework/Versions/wxWidgets/3.1/bin/wx-config")
            # MacPorts
            set(wxWidgets_CONFIG_EXECUTABLE "/opt/local/Library/Frameworks/wxWidgets.framework/Versions/wxWidgets/3.1/bin/wx-config")
            set(wxWidgets_wxrc_EXECUTABLE "/opt/local/Library/Frameworks/wxWidgets.framework/Versions/wxWidgets/3.1/bin/wxrc")
            message(STATUS "Using known location of wx-config 3.1")

        else()
            message(STATUS "No wx-config in expected locations")
        endif()
    endif()

    add_subdirectory(WinPort/src/Backend/WX)
endif()

if (NOT DEFINED TTYX)
    find_package(X11)
    if(X11_FOUND)
        message(STATUS "Building with TTY X11 extensions due to X11 found")
        add_subdirectory(WinPort/src/Backend/TTY/TTYX)
    else()
        message(STATUS "Building without TTY X11 extensions due to X11 not found")
    endif()
elseif (TTYX)
    message(STATUS "Building with TTY X11 extensions due to TTYX=${TTYX}")
    add_subdirectory(WinPort/src/Backend/TTY/TTYX)
else()
    message(STATUS "Building without TTY X11 extensions due to TTYX=${TTYX}")
endif()


##############################
# plugins directives

function(AddPlugin Name Suffix Active)
  string(TOUPPER ${Name} Var)
  if ((DEFINED ${Var} AND ${Var}) OR (NOT DEFINED ${Var} AND ${Active}))
      message(STATUS "${Var} plugin enabled")
      add_subdirectory (${Name})
  else()
      if (${Active})
          message(STATUS "${ColorRed}${Var} plugin disabled due to ${Var}=${${Var}}${ColorNormal}")
      else()
          message(STATUS "${ColorRed}${Var} plugin disabled, use -D${Var}=yes if you need it${ColorNormal}")
      endif()
      install(CODE "
          execute_process(COMMAND ${CMAKE_COMMAND} -E rm -f ${CMAKE_INSTALL_PREFIX}/lib/far2m/Plugins/${Name}/plug/${Name}.far-plug-${Suffix})
          " COMPONENT system)
  endif()
endfunction()

if (${USELUA})
  find_package(LibLua REQUIRED)
  find_package(LibUuid)
  if (NOT LibUuid_FOUND)
    message(STATUS "${ColorRed}Building without LibUuid due to it was not found${ColorNormal}")
  endif()
  message(STATUS "LUAFAR library enabled")
  add_subdirectory (luafar)
  AddPlugin (luamacro wide 1)
  AddPlugin (hlfviewer wide 1)
else()
  message(STATUS "${ColorRed}LuaFAR plugins disabled due to -DUSELUA=${USELUA}${ColorNormal}")
endif()

AddPlugin( align         wide  1 )
AddPlugin( autowrap      wide  1 )
AddPlugin( calc          wide  1 )
AddPlugin( colorer       wide  1 )
AddPlugin( compare       wide  1 )
AddPlugin( drawline      wide  1 )
AddPlugin( editcase      wide  1 )
AddPlugin( editorcomp    wide  1 )
AddPlugin( filecase      wide  1 )
AddPlugin( incsrch       wide  1 )
AddPlugin( inside        mb    1 )
AddPlugin( netcfg        wide  1 )
AddPlugin( SimpleIndent  wide  1 )
AddPlugin( tmppanel      wide  1 )

if (NOT DEFINED MULTIARC OR MULTIARC)
    message(STATUS "MULTIARC plugin enabled")

    find_package(PkgConfig REQUIRED)
    pkg_search_module(LibArchive QUIET libarchive)
    if(NOT LibArchive_FOUND)
        # workaround for MacOS brew's/macports' libarchive
        if(IS_DIRECTORY "/usr/local/opt/libarchive/include")
            set(LibArchive_INCLUDE_DIR "/usr/local/opt/libarchive/include")
            set(LibArchive_LIBRARY "/usr/local/opt/libarchive/lib/libarchive.dylib")
        elseif(EXISTS "/opt/local/lib/libarchive.dylib")
            set(LibArchive_LIBRARY "/opt/local/lib/libarchive.dylib")
        endif()
        find_package(LibArchive)
        if(NOT LibArchive_FOUND)
            message(WARNING "${ColorRed}libarchive not found, multiarc will have weaker archives support. Its recommended to install libarchive-dev and reconfigure far2m.${ColorNormal}")
        endif()
    endif()

    pkg_search_module(PCRE QUIET libpcre)
    if(NOT PCRE_FOUND)
        message(WARNING "${ColorRed}libpcre not found, multiarc will have no regexp support in custom archive format descriptions. Install libpcre and reconfigure far2m if you need that functionality.${ColorNormal}")
    endif()

    add_subdirectory (multiarc)
else()
    AddPlugin(multiarc mb 0)
endif()

if (NOT DEFINED NETROCKS OR NETROCKS)
    message(STATUS "NETROCKS plugin enabled")
    find_package(OpenSSL)
    if(OPENSSL_FOUND)
        message(STATUS "OpenSSL found -> enjoy FTPS support in NetRocks")
    else()
        message(WARNING "${ColorRed}OpenSSL not found, NetRocks will not have FTPS protocol support. Install libssl-dev if you want FTPS protocol available in NetRocks.${ColorNormal}")
    endif(OPENSSL_FOUND)

    find_package(LibSSH 0.5.0)
    if(LIBSSH_FOUND)
        message(STATUS "libssh found -> enjoy SFTP support in NetRocks")
    else()
        message(WARNING "${ColorRed}libssh not found, NetRocks will not have SFTP protocol support. Install libssh-dev if you want SFTP protocol available in NetRocks.${ColorNormal}")
    endif(LIBSSH_FOUND)

    find_package(Libsmbclient)
    if(LIBSMBCLIENT_FOUND)
        message(STATUS "libsmbclient found -> enjoy SMB support in NetRocks")
    else()
        message(WARNING "${ColorRed}libsmbclient not found, NetRocks will not have SMB protocol support. Install libsmbclient-dev if you want SMB protocol available in NetRocks.${ColorNormal}")
    endif(LIBSMBCLIENT_FOUND)

    find_package(LibNfs)
    if(LIBNFS_FOUND)
        message(STATUS "libnfs found -> enjoy NFS support in NetRocks")
    else()
        message(WARNING "${ColorRed}libnfs not found, NetRocks will not have NFS protocol support. Install libnfs-dev if you want NFS protocol available in NetRocks.${ColorNormal}")
    endif(LIBNFS_FOUND)

    find_package(LibNEON)
    if(LIBNEON_FOUND)
        message(STATUS "libneon found -> enjoy WebDav support in NetRocks")
    else()
        message(WARNING "${ColorRed}libneon not found, NetRocks will not have WebDav protocol support. Install libneon*-dev if you want WebDav protocol available in NetRocks.${ColorNormal}")
    endif(LIBNEON_FOUND)

    add_subdirectory (NetRocks)
else()
    AddPlugin(NetRocks wide 0)
endif()

if (DEFINED PYTHON AND PYTHON)
    message(STATUS "PYTHON plugin with interpreter from virtualenv enabled")
    add_subdirectory (python)
    # Copy this explicitly cuz generic copier command excludes 'far2l'
    install(DIRECTORY "${INSTALL_DIR}/Plugins/python/plug/far2l"
        DESTINATION "lib/far2m/Plugins/python/plug/" USE_SOURCE_PERMISSIONS
        COMPONENT base FILES_MATCHING
        PATTERN "*")
    install(DIRECTORY "${INSTALL_DIR}/Plugins/python/plug/plugins"
        DESTINATION "lib/far2m/Plugins/python/plug/" USE_SOURCE_PERMISSIONS
        COMPONENT base FILES_MATCHING
        PATTERN "*")
else()
    AddPlugin(python wide 0)
endif()

##############################
# common install directives

add_subdirectory (packaging)

if (${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
    set(FIXUP_SCRIPT packaging/osx/FixupBundle.cmake)
    configure_file(${FIXUP_SCRIPT} ${PROJECT_BINARY_DIR}/${FIXUP_SCRIPT} @ONLY)
    install(DIRECTORY ${APP_DIR}/${APP_NAME}.app DESTINATION . USE_SOURCE_PERMISSIONS COMPONENT app EXCLUDE_FROM_ALL)
    install(SCRIPT ${PROJECT_BINARY_DIR}/${FIXUP_SCRIPT} COMPONENT app EXCLUDE_FROM_ALL)
endif()

install(PROGRAMS "${INSTALL_DIR}/${EXECUTABLE_NAME}" DESTINATION "bin" RENAME far2m COMPONENT base)

install(DIRECTORY "${INSTALL_DIR}/" DESTINATION "lib/far2m" USE_SOURCE_PERMISSIONS COMPONENT base FILES_MATCHING
    PATTERN "colorer/base" EXCLUDE
    PATTERN "far2m_gui.so"
    PATTERN "far2m_ttyx.broker"
    PATTERN "plug/*.far-plug-*"
    PATTERN "plug/*.broker"
    PATTERN "luafar/luafar.so"
    PATTERN "Manuals" EXCLUDE
    PATTERN "lua_share" EXCLUDE)

install(DIRECTORY "${INSTALL_DIR}/" DESTINATION "share/far2m" USE_SOURCE_PERMISSIONS COMPONENT base FILES_MATCHING
    PATTERN "${EXECUTABLE_NAME}" EXCLUDE
    PATTERN "far2medit" EXCLUDE
    PATTERN "far2m_*" EXCLUDE
    PATTERN "*.far-plug-*" EXCLUDE
    PATTERN "*.broker" EXCLUDE
    PATTERN "python/plug/python" EXCLUDE
    PATTERN "luafar/luafar.so" EXCLUDE
    PATTERN "lua_share/far2/test" EXCLUDE
    PATTERN "*")

# setup some symlinks
execute_process(COMMAND ${CMAKE_COMMAND} -E create_symlink ../../bin/far2m ${PROJECT_BINARY_DIR}/far/far2m_askpass)
execute_process(COMMAND ${CMAKE_COMMAND} -E create_symlink ../../bin/far2m ${PROJECT_BINARY_DIR}/far/far2m_sudoapp)
execute_process(COMMAND ${CMAKE_COMMAND} -E create_symlink far2m ${PROJECT_BINARY_DIR}/far/far2medit)
install(FILES "${PROJECT_BINARY_DIR}/far/far2m_askpass" DESTINATION "lib/far2m" COMPONENT system)
install(FILES "${PROJECT_BINARY_DIR}/far/far2m_sudoapp" DESTINATION "lib/far2m" COMPONENT system)
install(FILES "${PROJECT_BINARY_DIR}/far/far2medit" DESTINATION "bin" COMPONENT base)
