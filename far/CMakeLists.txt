project(far2m)

add_subdirectory (bootstrap)

set(SOURCES
src/cache.cpp
src/chattr.cpp
src/clipboard.cpp
src/cmdline.cpp
src/copy.cpp
src/ctrlobj.cpp
src/datetime.cpp
src/delete.cpp
src/dialog.cpp
src/dirinfo.cpp
src/dizlist.cpp
src/DialogBuilder.cpp
src/dlgedit.cpp
src/DlgGuid.cpp
src/edit.cpp
src/editor.cpp
src/execute.cpp
src/farwinapi.cpp
src/fileattr.cpp
src/fileedit.cpp
src/filefilter.cpp
src/filefilterparams.cpp
src/FilesSuggestor.cpp
src/fileowner.cpp
src/filepanels.cpp
src/filestr.cpp
src/filetype.cpp
src/fileview.cpp
src/findfile.cpp
src/flink.cpp
src/fnparce.cpp
src/foldtree.cpp
src/frame.cpp
src/global.cpp
src/help.cpp
src/hilight.cpp
src/hmenu.cpp
src/Mounts.cpp
src/keybar.cpp
src/main.cpp
src/manager.cpp
src/menubar.cpp
src/message.cpp
src/mkdir.cpp
src/modal.cpp
src/namelist.cpp
src/options.cpp
src/plist.cpp
src/rdrwdsk.cpp
src/RefreshFrameManager.cpp
src/scantree.cpp
src/setattr.cpp
src/setcolor.cpp
src/pick_color.cpp
src/pick_color256.cpp
src/pick_colorRGB.cpp
src/stddlg.cpp
src/synchro.cpp
src/syslog.cpp
src/TPreRedrawFunc.cpp
src/usermenu.cpp
src/viewer.cpp
src/vmenu.cpp
src/execute_oscmd.cpp
src/ViewerPrinter.cpp
src/fileholder.cpp
src/GrepFile.cpp

src/panels/panel.cpp
src/panels/filelist.cpp
src/panels/flmodes.cpp
src/panels/flplugin.cpp
src/panels/flshow.cpp
src/panels/flupdate.cpp
src/panels/infolist.cpp
src/panels/qview.cpp
src/panels/treelist.cpp

src/console/AnsiEsc.cpp
src/console/keyboard.cpp
src/console/console.cpp
src/console/constitle.cpp
src/console/interf.cpp
src/console/grabber.cpp
src/console/lockscrn.cpp
src/console/palette.cpp
src/console/farcolorexp.cpp
src/console/farcolors.cpp
src/console/savescr.cpp
src/console/scrbuf.cpp
src/console/scrobj.cpp
src/console/scrsaver.cpp

src/macro/chgmmode.cpp
src/macro/macro.cpp
src/macro/macroapi.cpp
src/macro/tvar.cpp

src/filemask/CFileMask.cpp
src/filemask/FileMasksProcessor.cpp

src/locale/codepage.cpp
src/locale/DetectCodepage.cpp
src/locale/xlat.cpp
src/locale/locale.cpp

src/bookmarks/Bookmarks.cpp
src/bookmarks/BookmarksMenu.cpp

src/cfg/AllXLats.cpp
src/cfg/config.cpp
src/cfg/ConfigOpt.cpp
src/cfg/ConfigRW.cpp
src/cfg/HotkeyLetterDialog.cpp
src/cfg/language.cpp
src/cfg/MaskGroups.cpp

src/hist/history.cpp
src/hist/poscache.cpp

src/plug/plugapi.cpp
src/plug/plugins.cpp
src/plug/PluginW.cpp
src/plug/PluginA.cpp
src/plug/plclass.cpp

src/vt/vtansi.cpp
src/vt/vtshell.cpp
src/vt/vtshell_translation.cpp
src/vt/vtshell_translation_kitty.cpp
src/vt/vtshell_compose.cpp
src/vt/vtshell_leader.cpp
src/vt/vtshell_ioreaders.cpp
src/vt/vtshell_mouse.cpp
src/vt/vtlog.cpp
src/vt/vtcompletor.cpp
src/vt/VTFar2lExtensios.cpp

src/base/InterThreadCall.cpp
src/base/SafeMMap.cpp
src/base/farqueue.cpp
src/base/FARString.cpp
src/base/farrtl.cpp

src/mix/format.cpp
src/mix/udlist.cpp
src/mix/cvtname.cpp
src/mix/cddrv.cpp
src/mix/chgprior.cpp
src/mix/MountInfo.cpp
src/mix/dirmix.cpp
src/mix/drivemix.cpp
src/mix/mix.cpp
src/mix/panelmix.cpp
src/mix/pathmix.cpp
src/mix/processname.cpp
src/mix/RegExp.cpp
src/mix/strmix.cpp
src/mix/FSFileFlags.cpp
src/mix/StrCells.cpp
src/mix/CachedCreds.cpp
src/mix/GitTools.cpp
)

# Set target names for the executables
if(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
    set(MACOSX_BUNDLE_LONG_VERSION_STRING ${VERSION})
    set(MACOSX_BUNDLE_SHORT_VERSION_STRING ${VERSION})
    set(MACOSX_BUNDLE_GUI_IDENTIFIER ${APP_IDENTIFIER})
    set(MACOSX_BUNDLE_ICON_FILE "Far.icns")
    set(MACOSX_BUNDLE_BUNDLE_NAME ${APP_NAME})
    add_executable(far2m MACOSX_BUNDLE ${SOURCES} "DE/Far.icns")
    set_target_properties(far2m PROPERTIES
        OUTPUT_NAME ${APP_NAME}
        RUNTIME_OUTPUT_DIRECTORY ${APP_DIR})
    set_source_files_properties("DE/Far.icns" PROPERTIES
        MACOSX_PACKAGE_LOCATION Resources)
else()
    add_executable(far2m ${SOURCES})
    set_target_properties(far2m PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${INSTALL_DIR}")
endif()

if (${USELUA})
    target_compile_definitions(far2m PRIVATE -DUSELUA)
endif()

if (NOT DEFINED USEUCD)
    set(USEUCD "YES")
endif()

if (NOT USEUCD)
    message(STATUS "Building without libuchardet due to USEUCD=${USEUCD}")
else()
    message(STATUS "Building with libuchardet")
    find_package(Uchardet REQUIRED)
    target_compile_definitions(far2m PRIVATE -DUSEUCD)
endif()

target_compile_definitions(far2m PRIVATE -DUNICODE)

target_include_directories(far2m PRIVATE
    .
    far2sdk
    src
    src/base
    src/mix
    src/bookmarks
    src/cfg
    src/console
    src/panels
    src/filemask
    src/hist
    src/locale
    src/macro
    src/plug
    src/vt
    ${CMAKE_CURRENT_BINARY_DIR}
    ${UCHARDET_INCLUDE_DIRS}
    ../WinPort)

add_dependencies(far2m bootstrap WinPort)

if (${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
    SET (WINPORT -force_load WinPort wineguts utils)
else()
    SET (WINPORT -Wl,--whole-archive WinPort -Wl,--no-whole-archive)
endif()

set_target_properties(far2m
    PROPERTIES
    ENABLE_EXPORTS TRUE)

if (DEFINED USELUA AND ${CMAKE_SYSTEM_NAME} MATCHES "Android")
    set (LIBLUAJIT -Wl,--whole-archive ${CMAKE_CURRENT_SOURCE_DIR}/libluajit.a -Wl,--no-whole-archive)
    target_link_libraries(far2m PUBLIC ${LIBLUAJIT})
endif()

if (${CMAKE_SYSTEM_NAME} MATCHES "FreeBSD")
    target_link_libraries(far2m
    PRIVATE ${WINPORT}
    PRIVATE c
    PRIVATE ${UCHARDET_LIBRARIES})
else()
    target_link_libraries(far2m
    PRIVATE ${WINPORT}
    PRIVATE dl
    PRIVATE ${UCHARDET_LIBRARIES})
endif()

add_custom_command(TARGET far2m POST_BUILD
    COMMAND ln -sf ${EXECUTABLE_NAME} ${INSTALL_DIR}/far2m_askpass
    COMMAND ln -sf ${EXECUTABLE_NAME} ${INSTALL_DIR}/far2m_sudoapp
    COMMAND ln -sf ${EXECUTABLE_NAME} ${INSTALL_DIR}/far2medit)

if (${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
#TODO
else()
    configure_file("${CMAKE_CURRENT_SOURCE_DIR}/DE/far2m-tty.desktop" "${CMAKE_CURRENT_BINARY_DIR}/DE/far2m-tty.desktop")
    configure_file("${CMAKE_CURRENT_SOURCE_DIR}/DE/far2medit-tty.desktop" "${CMAKE_CURRENT_BINARY_DIR}/DE/far2medit-tty.desktop")
    configure_file("${CMAKE_CURRENT_SOURCE_DIR}/DE/far2m.desktop" "${CMAKE_CURRENT_BINARY_DIR}/DE/far2m.desktop")
    configure_file("${CMAKE_CURRENT_SOURCE_DIR}/DE/far2medit.desktop" "${CMAKE_CURRENT_BINARY_DIR}/DE/far2medit.desktop")

    install(DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/DE/icons" DESTINATION "share" USE_SOURCE_PERMISSIONS COMPONENT desktop FILES_MATCHING PATTERN "*")

	# Have to make desktop files executable, see:
	# https://wiki.ubuntu.com/SecurityTeam/Policies#Execute-Permission_Bit_Required
    install(FILES "${CMAKE_CURRENT_BINARY_DIR}/DE/far2m-tty.desktop"
                  "${CMAKE_CURRENT_BINARY_DIR}/DE/far2medit-tty.desktop"
                  "${CMAKE_CURRENT_BINARY_DIR}/DE/far2m.desktop"
                  "${CMAKE_CURRENT_BINARY_DIR}/DE/far2medit.desktop"
        DESTINATION "share/applications"
        COMPONENT desktop
        PERMISSIONS OWNER_EXECUTE OWNER_WRITE OWNER_READ GROUP_EXECUTE GROUP_READ WORLD_READ WORLD_EXECUTE)
endif()
