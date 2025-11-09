
project(ImageViewer)

set(SOURCES
GlobalInfo.cpp
ImageViewer.cpp
)

add_library (ImageViewer MODULE ${SOURCES})

target_link_libraries(ImageViewer utils far2m)

target_compile_definitions(ImageViewer PRIVATE -DWINPORT_DIRECT -DUNICODE -DFAR_DONT_USE_INTERNALS)

target_include_directories(ImageViewer PRIVATE .)
target_include_directories(ImageViewer PRIVATE ../WinPort)
target_include_directories(ImageViewer PRIVATE ../far/far2sdk)

set_target_properties(ImageViewer
    PROPERTIES
        LIBRARY_OUTPUT_DIRECTORY "${INSTALL_DIR}/Plugins/ImageViewer/plug"
        PREFIX ""
        SUFFIX ".far-plug-wide")

