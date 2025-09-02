########################################################################
# NAPQT Static Library
########################################################################

project(napqt)

include(${NAP_ROOT}/cmake/qtconfig.cmake)
include(${NAP_ROOT}/cmake/nap_utilities.cmake)
find_qt()

file(GLOB_RECURSE SOURCES src/*.cpp)
file(GLOB_RECURSE HEADERS src/*.h)
file(GLOB_RECURSE UI_HEADERS src/*.ui)
file(GLOB_RECURSE RESOURCES resources/*.qrc)

add_library(${PROJECT_NAME} STATIC ${SOURCES} ${UI_HEADERS} ${HEADERS})
target_include_directories(${PROJECT_NAME} PUBLIC src)

set_target_properties(${PROJECT_NAME} PROPERTIES FOLDER Core)

codesign(${QTDIR}/plugins/platforms/libqminimal.dylib)
codesign(${QTDIR}/plugins/platforms/libqoffscreen.dylib)
codesign(${QTDIR}/plugins/platforms/libqcocoa.dylib)
get_target_property(qt_core_path Qt6::Core IMPORTED_LOCATION)
get_target_property(qt_gui_path Qt6::Gui IMPORTED_LOCATION)
get_target_property(qt_widgets_path Qt6::Widgets IMPORTED_LOCATION)
get_target_property(qt_opengl_path Qt6::OpenGL IMPORTED_LOCATION)
codesign(${qt_core_path})
codesign(${qt_gui_path})
codesign(${qt_widgets_path})
codesign(${qt_opengl_path})

target_link_import_library(${PROJECT_NAME} Qt6::Core)
target_link_import_library(${PROJECT_NAME} Qt6::Gui)
target_link_import_library(${PROJECT_NAME} Qt6::Widgets)
target_link_import_library(${PROJECT_NAME} Qt6::OpenGL)

# Remove lib prefix on Unix libraries
set_target_properties(${PROJECT_NAME} PROPERTIES PREFIX "")

# Deploy Qt plugins from thirdparty on Win64. Unlike macOS, Windows won't find
# the plugins under a plugins folder, the categories need to sit beside the binary.
add_custom_command(
        TARGET ${PROJECT_NAME}
        POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory ${QTDIR}/plugins/platforms ${LIB_DIR}/platforms
)
install(DIRECTORY ${LIB_DIR}/platforms TYPE LIB OPTIONAL)


