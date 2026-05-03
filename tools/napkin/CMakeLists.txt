project(napkin)
find_qt()

#if (APPLE)
#    # Ideally we would put this here on Apple too but in napkin on Apple we need to link to original Qt locations.
#    set(CMAKE_SKIP_BUILD_RPATH TRUE)
#else ()
#    set(CMAKE_BUILD_WITH_INSTALL_RPATH TRUE)
#endif ()


file(GLOB_RECURSE SOURCES src/*.cpp)
file(GLOB_RECURSE HEADERS src/*.h)
file(GLOB_RECURSE RESOURCES resources/*.qrc)

if (MSVC)
    set(RESOURCES ${RESOURCES} resources/resources.rc)
endif()

add_executable(${PROJECT_NAME} ${SOURCES} ${UI_HEADERS} ${HEADERS} ${RESOURCES})
target_include_directories(${PROJECT_NAME} PUBLIC src)

set_target_properties(${PROJECT_NAME} PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY "$(OutDir)")
set_target_properties(${PROJECT_NAME} PROPERTIES FOLDER Tools)
set_target_properties(${PROJECT_NAME} PROPERTIES INSTALL_RPATH "$ORIGIN/lib")

if(WIN32)
    set_property(TARGET ${PROJECT_NAME} PROPERTY WIN32_EXECUTABLE true)
endif()

if(${ARCH} STREQUAL "armhf")
   set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -z muldefs")
endif()

# The NAP modules Napkin needs
set(NAP_MODULES_DEPENDENCIES napcore napscene napcolor napmath naprender napinput napfont napcameracontrol naprenderadvanced napimgui napapi)

# Set libraries
set(LIBS
    napqt
    ${NAP_MODULES_DEPENDENCIES}
    )

target_link_libraries(${PROJECT_NAME} ${LIBS})

# Update executable rpath
if(APPLE)
    add_custom_command(TARGET ${PROJECT_NAME}
            POST_BUILD COMMAND
            ${CMAKE_INSTALL_NAME_TOOL} -add_rpath "@executable_path/${LIB_RPATH}/." $<TARGET_FILE:${PROJECT_NAME}>)
elseif (UNIX)

endif()

# Copy resources to bin and install to packaged app
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
                   COMMAND ${CMAKE_COMMAND} -E copy_directory
                   ${CMAKE_CURRENT_LIST_DIR}/resources
                   ${BIN_DIR}/resources
                   COMMENT "Copy Resources")
install(DIRECTORY ${BIN_DIR}/resources TYPE BIN OPTIONAL)

# Install executable to packaged app
install(FILES $<TARGET_FILE:${PROJECT_NAME}> PERMISSIONS OWNER_EXECUTE OWNER_READ OWNER_WRITE TYPE BIN OPTIONAL)