cmake_minimum_required(VERSION 3.18.4)

# Get the plugin name from the directory name
get_filename_component(plugin_name ${CMAKE_CURRENT_SOURCE_DIR} NAME)

project(${plugin_name} VERSION 1.0.0  DESCRIPTION "Test Effect")
enable_language(OBJCXX)

# Set global directories and paths
set(VST3SDK_SOURCE_DIR ${NAP_ROOT}/../VST_SDK/vst3sdk)
set(BIN_DIR ${CMAKE_BINARY_DIR}/bin)

# Include NAP, VST3 SDK and NAP cmake utilites
add_subdirectory(${VST3SDK_SOURCE_DIR} ${CMAKE_BINARY_DIR}/vst3)
include(${NAP_ROOT}/cmake/nap_utilities.cmake)

# Scan for an app module
try_add_module_from_dir(${CMAKE_CURRENT_SOURCE_DIR}/module)

# Initialize VST3 SDK
set(SMTG_RUN_VST_VALIDATOR OFF)
smtg_enable_vst3_sdk()

# Add all cpp files to SOURCES
file(GLOB_RECURSE SOURCES src/*.cpp)
file(GLOB_RECURSE HEADERS src/*.h src/*.hpp)

smtg_add_vst3plugin(${PROJECT_NAME}
        ${VST3SDK_SOURCE_DIR}/public.sdk/source/vst/vstsinglecomponenteffect.cpp
        ${VST3SDK_SOURCE_DIR}/public.sdk/source/vst/vstsinglecomponenteffect.h
        ${SOURCES}
        ${HEADERS}
)

# Pull in the project module if it exists
if (TARGET nap${PROJECT_NAME})
    target_link_libraries(${PROJECT_NAME} nap${PROJECT_NAME})
endif()

target_link_libraries(${PROJECT_NAME} PRIVATE sdk vstgui_support napimgui_static napparametergui_static napfmsynth_static napcontrol_static)

smtg_target_configure_version_file(${PROJECT_NAME})

set(bundle_name "${PROJECT_NAME}.vst3")
set(source_data_dir ${CMAKE_CURRENT_SOURCE_DIR}/data)
set(output_path "${CMAKE_BINARY_DIR}/VST3/${CMAKE_BUILD_TYPE}/${bundle_name}")
set(lib_dir ${CMAKE_BINARY_DIR}/bin/lib)

if (APPLE)
    set(output_resources_dir ${output_path}/Contents/Resources)
    set(bundle_relative_bin_dir Contents/MacOS)
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

install(DIRECTORY ${output_path} DESTINATION vst COMPONENT vst EXCLUDE_FROM_ALL)
install(DIRECTORY ${lib_dir}
        DESTINATION vst/${bundle_name}/${bundle_relative_bin_dir}
        COMPONENT vst EXCLUDE_FROM_ALL
        FILES_MATCHING PATTERN "*${CMAKE_SHARED_LIBRARY_SUFFIX}"
)

if(SMTG_MAC)
    set_source_files_properties(src/nappluginview.cpp PROPERTIES LANGUAGE OBJCXX)
    smtg_target_set_bundle(${PROJECT_NAME}
            BUNDLE_IDENTIFIER org.stijnvanbeek.VstTest
            COMPANY_NAME StijnVanBeek
    )
    smtg_target_set_debug_executable(${PROJECT_NAME}
            "/Applications/VST3PluginTestHost.app"
            "--pluginfolder;$(BUILT_PRODUCTS_DIR)"
    )

#    # Copy libMoltenVK.dylib to bundle
#    add_custom_command(
#        TARGET ${PROJECT_NAME} POST_BUILD
#        COMMAND ${CMAKE_COMMAND} -E copy ${lib_dir}/libMoltenVK.dylib ${output_lib_dir}/libMoltenVK.dylib
#    )

    # Update executable rpath if it hasn't been set already
    # Using a bash script, it checks if the '@executablepath/%{LIB_RPATH}/' already exists in the list displayed by 'otool -l'. If not, it calls the CMAKE_INSTALL_NAME_TOOL to install the name.
    add_custom_command(TARGET ${PROJECT_NAME}
            POST_BUILD COMMAND
            if ! otool -l $<TARGET_FILE:${PROJECT_NAME}> | grep -q @loader_path/lib\; then ${CMAKE_INSTALL_NAME_TOOL} -add_rpath "@loader_path/lib" $<TARGET_FILE:${PROJECT_NAME}>\; fi)
    add_custom_command(TARGET ${PROJECT_NAME}
            POST_BUILD COMMAND
            if ! otool -l $<TARGET_FILE:${PROJECT_NAME}> | grep -q @loader_path\; then ${CMAKE_INSTALL_NAME_TOOL} -add_rpath "@loader_path" $<TARGET_FILE:${PROJECT_NAME}>\; fi)

elseif(SMTG_WIN)
#    target_sources(MyPlugin
#            PRIVATE
#            resource/win32resource.rc
#    )
#    if(MSVC)
#        set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT MyPlugin)
#
#        smtg_target_set_debug_executable(MyPlugin
#                "$(ProgramW6432)/Steinberg/VST3PluginTestHost/VST3PluginTestHost.exe"
#                "--pluginfolder \"$(OutDir)/\""
#        )
#    endif()
endif()

codesign_target(${PROJECT_NAME})
