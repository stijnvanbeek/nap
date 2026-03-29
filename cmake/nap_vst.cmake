cmake_minimum_required(VERSION 3.18.4)

macro(add_vst)
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

    if (NOT DEFINED major_version)
        set(major_version 0)
    endif()
    if (NOT DEFINED sub_version)
        set(sub_version 0)
    endif()
    if (NOT DEFINED release_number)
        set(release_number 0)
    endif()
    if (NOT DEFINED build_number)
        set(build_number 1)
    endif()
    if (NOT DEFINED company_name)
        set(company_name "NAP Framework")
    endif ()
    if (NOT DEFINED company_website)
        set(company_website "https://github.com/napframework/nap")
    endif ()
    if (NOT DEFINED company_email)
        set(company_email "")
    endif ()
    if (NOT DEFINED bundle_identifier)
        set(bundle_identifier "vst.napframework.${PROJECT_NAME}")
    endif ()
    if (NOT DEFINED plugin_name)
        # Get the plugin name from the directory name
        get_filename_component(plugin_name ${CMAKE_CURRENT_SOURCE_DIR} NAME)
    endif ()
    if (NOT DEFINED description)
        set(description "This is a VST3 plugin made with NAP framework.")
    endif ()
    if (NOT DEFINED app_structure_filename)
        set(app_structure_filename "objects.json")
    endif ()
    if (NOT DEFINED plugin_type_name)
        set(plugin_type_name "nap::AudioPlugin")
    endif ()

    project(${plugin_name} VERSION 1.0.0  DESCRIPTION ${description})
    if (APPLE)
        enable_language(OBJCXX)
    endif ()

    configure_file(${NAP_ROOT}/vst/src/project.h.in ${CMAKE_CURRENT_LIST_DIR}/src/project.h @ONLY)

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
                BUNDLE_IDENTIFIER ${BUNDLE_IDENTIFIER}
                COMPANY_NAME ${COMPANY_NAME}
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
endmacro()