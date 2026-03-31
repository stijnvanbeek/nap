cmake_minimum_required(VERSION 3.18.4)

if(UNIX)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-suggest-override -Wno-format-security -Wno-switch -Wno-inconsistent-missing-override -fvisibility=hidden")
    if (APPLE)
        # Add Apple specific flags
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-nullability-completeness")
    endif ()
endif()
if (WIN32)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /wd4244 /wd4305 /wd4996 /wd4267 /wd4018 /wd4251 /wd4099 /MP /bigobj /Zc:preprocessor /wd5105")
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /ZI")
endif ()

# Read vst.json from file
set(vst_json_path ${CMAKE_CURRENT_SOURCE_DIR}/vst.json)
file(READ ${vst_json_path} vst_json)

string(JSON plugin_version GET ${vst_json} Version)
if (plugin_version STREQUAL "")
    set(plugin_version 0.0.0)
endif()

string(JSON company_name GET ${vst_json} Company)
string(JSON company_website GET ${vst_json} CompanyWeb)
string(JSON company_email GET ${vst_json} CompanyEmail)

if (NOT DEFINED bundle_identifier)
    set(bundle_identifier "vst.napframework.${PROJECT_NAME}")
endif ()


string(JSON plugin_name GET ${vst_json} Name)
if (NOT DEFINED plugin_name)
    # Get the plugin name from the directory name
    get_filename_component(plugin_name ${CMAKE_CURRENT_SOURCE_DIR} NAME)
endif ()

string(JSON description GET ${vst_json} Description)
string(JSON app_structure_path GET ${vst_json} Data)
string(JSON plugin_class_name GET ${vst_json} Class)
string(JSON plugin_category GET ${vst_json} Category)
string(JSON plugin_uid GET ${vst_json} UID)

string(JSON audio_input GET ${vst_json} AudioInput)
if (NOT DEFINED audio_input)
    set(audio_input "Empty")
endif ()
string(JSON audio_output GET ${vst_json} AudioOutput)
if (NOT DEFINED audio_output)
    set(audio_output "Empty")
endif ()

# Get the target name from the directory name
get_filename_component(target_name ${CMAKE_CURRENT_SOURCE_DIR} NAME)

project(${target_name} VERSION 1.0.0  DESCRIPTION ${description})
if (APPLE)
    enable_language(OBJCXX)
endif ()

configure_file(${NAP_ROOT}/vst/src/project.h.in ${CMAKE_CURRENT_SOURCE_DIR}/src/project.h @ONLY)

# Set global directories and paths
set(BIN_DIR ${CMAKE_BINARY_DIR}/bin)

include(${NAP_ROOT}/cmake/nap_utilities.cmake)

# Scan for an app module
try_add_module_from_dir(${CMAKE_CURRENT_SOURCE_DIR}/module)

# Initialize VST3 SDK
set(SMTG_RUN_VST_VALIDATOR OFF)
smtg_enable_vst3_sdk()

# Add all cpp files to SOURCES
file(GLOB_RECURSE SOURCES src/*.cpp)
file(GLOB_RECURSE HEADERS src/*.h src/*.hpp)
file(GLOB_RECURSE napvst_sources ${NAP_ROOT}/vst/src/*.cpp)
file(GLOB_RECURSE napvst_headers ${NAP_ROOT}/vst/src/*.h ${NAP_ROOT}/vst/src/*.hpp)

smtg_add_vst3plugin(${PROJECT_NAME}
        ${VST3SDK_DIR}/public.sdk/source/vst/vstsinglecomponenteffect.cpp
        ${VST3SDK_DIR}/public.sdk/source/vst/vstsinglecomponenteffect.h
        ${SOURCES}
        ${HEADERS}
        ${napvst_sources}
        ${napvst_headers}
)
target_include_directories(${PROJECT_NAME} PRIVATE ${NAP_ROOT}/vst/src src)

# Pull in the project module if it exists
if (TARGET nap${PROJECT_NAME})
    target_link_libraries(${PROJECT_NAME} nap${PROJECT_NAME})
endif()

target_link_libraries(${PROJECT_NAME} PRIVATE sdk vstgui_support)

# Read required modules from the vst json file
string(JSON required_modules_json GET ${vst_json} RequiredModules)
string(JSON required_module_count LENGTH ${vst_json} RequiredModules)
set(module_index 0)
while(NOT ${module_index} EQUAL ${required_module_count})
    string(JSON module GET ${required_modules_json} ${module_index})
    list(APPEND required_modules ${module}_static)
    math(EXPR module_index "${module_index}+1")
endwhile()

if (NOT required_modules)
    set(required_modules napparameter_static napimgui_static napaudio_static napmidi_static)
endif()
target_link_libraries(${PROJECT_NAME} PRIVATE ${required_modules})

smtg_target_configure_version_file(${PROJECT_NAME})

set(bundle_name "${PROJECT_NAME}.vst3")
set(source_data_dir ${CMAKE_CURRENT_SOURCE_DIR}/data)
set(output_path "${CMAKE_BINARY_DIR}/VST3/${CMAKE_BUILD_TYPE}/${bundle_name}")
set(lib_dir ${CMAKE_BINARY_DIR}/bin/lib)

if (APPLE)
    set(output_resources_dir ${output_path}/Contents/Resources)
    set(bundle_relative_bin_dir Contents/MacOS)
elseif (UNIX)
    set(output_resources_dir ${output_path}/Contents/Resources)
    set(bundle_relative_bin_dir "Contents/x86_64-linux")
elseif(WIN32)
    set(output_resources_dir ${output_path}/Contents/Resources)
    set(bundle_relative_bin_dir "Contents/x86_64-win")
endif ()
set(output_data_dir ${output_resources_dir}/data)

# Copy data directory to bundle
add_custom_command(
        TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory ${source_data_dir} ${output_data_dir}
)

if (NOT DEFINED CODE_SIGNATURE)
    set(CODE_SIGNATURE "-")
endif ()

install(DIRECTORY "${output_path}" DESTINATION vst COMPONENT vst EXCLUDE_FROM_ALL)
install(DIRECTORY "${lib_dir}"
        DESTINATION "vst/${bundle_name}/${bundle_relative_bin_dir}"
        COMPONENT vst EXCLUDE_FROM_ALL
        FILES_MATCHING
        PATTERN "*.*"
        PATTERN "*.a" EXCLUDE
        PATTERN "*.json" EXCLUDE
)

if(SMTG_MAC)
    set_source_files_properties(${NAP_ROOT}/vst/src/nappluginview.cpp PROPERTIES LANGUAGE OBJCXX)
    smtg_target_set_bundle(${PROJECT_NAME}
            BUNDLE_IDENTIFIER ${bundle_identifier}
            COMPANY_NAME ${company_name}
    )

    # Update executable rpath if it hasn't been set already
    # Using a bash script, it checks if the '@executablepath/%{LIB_RPATH}/' already exists in the list displayed by 'otool -l'. If not, it calls the CMAKE_INSTALL_NAME_TOOL to install the name.
    add_custom_command(TARGET ${PROJECT_NAME}
        POST_BUILD COMMAND
        if ! otool -l $<TARGET_FILE:${PROJECT_NAME}> | grep -q @loader_path/lib\; then ${CMAKE_INSTALL_NAME_TOOL} -add_rpath "@loader_path/lib" $<TARGET_FILE:${PROJECT_NAME}>\; fi)
    add_custom_command(TARGET ${PROJECT_NAME}
        POST_BUILD COMMAND
        if ! otool -l $<TARGET_FILE:${PROJECT_NAME}> | grep -q @loader_path\; then ${CMAKE_INSTALL_NAME_TOOL} -add_rpath "@loader_path" $<TARGET_FILE:${PROJECT_NAME}>\; fi)

elseif(SMTG_WIN)
#        target_sources(${PROJECT_NAME}
#            PRIVATE
#            resource/win32resource.rc
#        )
endif()

codesign_target(${PROJECT_NAME})
