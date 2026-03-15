########################################################################
# NAPQT Static Library
########################################################################


project(napqt)

function(target_link_qt_module target qt_module)
    get_target_property(include_dirs ${qt_module} INTERFACE_INCLUDE_DIRECTORIES)
    get_target_property(library_path ${qt_module} IMPORTED_LOCATION)

    add_custom_command(
            TARGET ${PROJECT_NAME}
            PRE_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy ${library_path} ${LIB_DIR}
    )

    get_filename_component(library_file_name ${library_path} NAME)
    target_link_libraries(${target} ${library_path})
    target_include_directories(${target} PUBLIC ${include_dirs})

    install(FILES ${LIB_DIR}/${library_file_name} TYPE LIB OPTIONAL)
endfunction()

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

if (APPLE)
    # On macos the qt modules are framework bundles that internally link to each other so we can't copy the DLL's to bin or package
    target_link_libraries(${PROJECT_NAME} Qt6::Core)
    target_link_libraries(${PROJECT_NAME} Qt6::Gui)
    target_link_libraries(${PROJECT_NAME} Qt6::Widgets)
    target_link_libraries(${PROJECT_NAME} Qt6::OpenGL)
    target_link_libraries(${PROJECT_NAME} Qt6::DBus)
else ()
    target_link_qt_module(${PROJECT_NAME} Qt6::Core)
    target_link_qt_module(${PROJECT_NAME} Qt6::Gui)
    target_link_qt_module(${PROJECT_NAME} Qt6::Widgets)
    target_link_qt_module(${PROJECT_NAME} Qt6::OpenGL)
    target_link_qt_module(${PROJECT_NAME} Qt6::DBus)
endif()

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


