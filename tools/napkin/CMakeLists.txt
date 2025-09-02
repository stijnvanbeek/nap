project(napkin)
find_qt()

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

if(WIN32)
    set_property(TARGET ${PROJECT_NAME} PROPERTY WIN32_EXECUTABLE true)
endif()

if(${ARCH} STREQUAL "armhf")
   set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -z muldefs")
endif()

# The NAP modules Napkin needs
set(NAP_MODULES_DEPENDENCIES napcore napscene napcolor napmath naprender napinput napfont)

# Add python if enabled
if(NAP_ENABLE_PYTHON)
    list(APPEND NAP_MODULES_DEPENDENCIES nappython)
endif()

# Set libraries
set(LIBS
    napqt
    ${NAP_MODULES_DEPENDENCIES}
    )

target_link_libraries(${PROJECT_NAME} ${LIBS})

if(APPLE)
    # Update executable rpath if it hasn't been set already
    # Using a bash script, it checks if the '@executablepath/%{LIB_RPATH}/' already exists in the list displayed by 'otool -l'. If not, it calls the CMAKE_INSTALL_NAME_TOOL to install the name.
    add_custom_command(TARGET ${PROJECT_NAME}
            POST_BUILD COMMAND
            if ! otool -l $<TARGET_FILE:${PROJECT_NAME}> | grep -q @executable_path/${LIB_RPATH}/.\; then ${CMAKE_INSTALL_NAME_TOOL} -add_rpath "@executable_path/${LIB_RPATH}/." $<TARGET_FILE:${PROJECT_NAME}>\; fi)

    # Codesign the shared library
    set(signature "-")
    if (DEFINED ${MACOS_CODE_SIGNATURE})
        set(signature ${MACOS_CODE_SIGNATURE})
        # Codesign the executable with signature in environment variable with runtime option
        add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
                COMMAND codesign --force -s ${MACOS_CODE_SIGNATURE} $<TARGET_FILE:${PROJECT_NAME}> --options runtime)
    else ()
        # Codesign the executable with an ad hoc sign
        add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
                COMMAND codesign --force -s - $<TARGET_FILE:${PROJECT_NAME}>)
    endif ()
endif()

# Copy resources to bin and install to packaged app
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
                   COMMAND ${CMAKE_COMMAND} -E copy_directory
                   ${CMAKE_CURRENT_LIST_DIR}/resources
                   ${BIN_DIR}/resources
                   COMMENT "Copy Resources")
install(DIRECTORY ${BIN_DIR}/resources TYPE DATA OPTIONAL)

# Install executable to packaged app
install(FILES $<TARGET_FILE:${PROJECT_NAME}> PERMISSIONS OWNER_EXECUTE OWNER_READ OWNER_WRITE TYPE BIN OPTIONAL)