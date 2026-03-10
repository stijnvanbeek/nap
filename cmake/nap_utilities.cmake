cmake_minimum_required(VERSION 3.19)

# Use this function to link import libraries from module_extra.cmake and other included cmake files
# target: The target to link the library to
# library: An import target that links to a DLL
function(target_link_import_library target library)
    if (NOT TARGET ${library})
        message(FATAL_ERROR "Library dependency ${library} not found for building ${target}")
    endif()
    get_target_property(library_path ${library} IMPORTED_LOCATION)
    get_target_property(library_path_debug ${library} IMPORTED_LOCATION_DEBUG)
    if (DEFINED CMAKE_BUILD_TYPE)
        if (${CMAKE_BUILD_TYPE} STREQUAL "Debug" AND EXISTS ${library_path_debug})
            set(library_path ${library_path_debug})
        endif()
    endif()
    get_filename_component(library_file_name ${library_path} NAME)

    get_target_property(include_dir ${library} INCLUDE_DIRECTORIES)
    get_target_property(library_type ${library} TYPE)
    get_target_property(target_type ${target} TYPE)

    if (${target_type} STREQUAL INTERFACE_LIBRARY)
        if (EXISTS ${include_dir})
            target_include_directories(${target} INTERFACE ${include_dir})
        endif ()
        target_link_libraries(${target} INTERFACE ${library})
    else ()
        if (EXISTS ${include_dir})
            target_include_directories(${target} PUBLIC ${include_dir})
        endif ()
        target_link_libraries(${target} ${library})
    endif ()
    if (${library_type} STREQUAL SHARED_LIBRARY)
        install(FILES ${LIB_DIR}/${library_file_name} TYPE LIB OPTIONAL)
    endif()
endfunction()


# Creates a target importing a single shared library
# target_name: Name of the new target
# implib: Path to the implib of the shared library: on Windows this is the .lib file, on linux and MacOS it's the same file as the dll.
# dll: Path to the DLL part of the shared library: on Windows the .dll file, on linux the .so file, on MacOS the .dylib.
# include_dir: Path to the include directories for the library.
function(add_import_library target_name implib dll include_dir)
    # Resolve symbolic links
    get_filename_component(implib ${implib} REALPATH)
    get_filename_component(dll ${dll} REALPATH)
    get_filename_component(include_dir ${include_dir} REALPATH)

    if (NOT EXISTS ${dll})
        message(WARNING "Dynamic library for ${target_name} not found for this platform. Tried: ${dll}")
        return()
    endif()

    if (NOT EXISTS ${implib})
        message(WARNING "Import library for ${target_name} not found for this platform. Tried: ${implib}")
        return()
    endif()

    if (NOT EXISTS ${include_dir})
        message(WARNING "Include directory for ${target_name} not found for this platform. Tried: ${include_dir}")
        return()
    endif()

    get_filename_component(dll_filename ${dll} NAME)
    set(dest_dll ${LIB_DIR}/${dll_filename})
    if (NOT EXISTS ${dest_dll})
        # Copy DLL file to LIB_DIR
        file(COPY ${dll} DESTINATION ${LIB_DIR})
        # Set install name for library
        if (UNIX)
            if (APPLE)
                execute_process(COMMAND install_name_tool -id
                        @rpath/${dll_filename}
                        ${dest_dll}
                        RESULT_VARIABLE EXIT_CODE)
                if(NOT ${EXIT_CODE} EQUAL 0)
                    message(FATAL_ERROR "Failed to set RPATH on ${dll_filename} using install_name_tool -id.")
                endif()
            else ()
                # Set so name or rpath for linux
                execute_process(COMMAND patchelf --set-soname
                        ${dll_filename}
                        ${dest_dll}
                        RESULT_VARIABLE EXIT_CODE)
                if(NOT ${EXIT_CODE} EQUAL 0)
                    message(FATAL_ERROR "Failed to set RPATH on ${dll_filename} using patchelf. Is patchelf installed?")
                endif()
            endif()
        endif ()
        # Codesign
        codesign(${dest_dll})
    endif()

    add_library(${target_name} SHARED IMPORTED GLOBAL)
    set_property(TARGET ${target_name} PROPERTY IMPORTED_LOCATION ${dest_dll})
    set_property(TARGET ${target_name} PROPERTY IMPORTED_IMPLIB ${implib})
    set_property(TARGET ${target_name} PROPERTY INCLUDE_DIRECTORIES ${include_dir})
endfunction()


# Creates a target importing one static library
# target_name: Name of the new target
# implib: Path to the to the shared library: on Windows this is a .lib file, on linux and MacOS it's a .a file
# include_dir: Path to the include directories for the library.
function(add_static_import_library target_name implib include_dir)
    # Resolve symbolic links
    get_filename_component(implib ${implib} REALPATH)
    get_filename_component(include_dir ${include_dir} REALPATH)

    if (NOT EXISTS ${implib})
        message(WARNING "Import library for ${target_name} not found for this platform. Tried: ${implib}")
        return()
    endif()

    if (NOT EXISTS ${include_dir})
        message(WARNING "Include directory for ${target_name} not found for this platform. Tried: ${include_dir}")
        return()
    endif()

    add_library(${target_name} STATIC IMPORTED GLOBAL)
    target_include_directories(${target_name} INTERFACE ${include_dir})
    set_property(TARGET ${target_name} PROPERTY IMPORTED_LOCATION ${implib})
    set_property(TARGET ${target_name} PROPERTY INCLUDE_DIRECTORIES ${include_dir})
endfunction()


# Add a source directory to an already defined target
# NAME of the source group in the IDE
# DIR directory of the source files relative to the project directory
# ARGN additional optional arguments are regex expressions to filter from the file list
function(add_source_dir NAME DIR)
    # Collect source files in directory
    file(GLOB SOURCES ${DIR}/*.cpp ${DIR}/*.h ${DIR}/*.hpp)

    # Loop through optional arguments and exclude them from the sources list
    foreach(element ${ARGN})
        list(FILTER SOURCES EXCLUDE REGEX ${element})
    endforeach()

    # Create source group for IDE
    source_group(${NAME} FILES ${SOURCES})

    # Add sources to target
    target_sources(${PROJECT_NAME} PRIVATE ${SOURCES})
    target_sources(${static_target} INTERFACE ${SOURCES})
endfunction()


function(add_subdirectory_apps subdirectory)
    set(directory ${CMAKE_CURRENT_SOURCE_DIR}/${subdirectory})
    file(GLOB children ${directory}/*)
    foreach(child ${children})
        if (IS_DIRECTORY ${child})
            try_add_app_from_dir(${child})
        endif ()
    endforeach ()
endfunction()


function(add_subdirectory_modules subdirectory)
    set(directory ${CMAKE_CURRENT_SOURCE_DIR}/${subdirectory})
    file(GLOB children ${directory}/*)
    foreach(child ${children})
        if (IS_DIRECTORY ${child})
            try_add_module_from_dir(${child})
        endif ()
    endforeach ()
endfunction()


function(try_add_module_from_dir module_dir)
    if (EXISTS ${module_dir}/module.json)
        # We have a module!
        # Create CMakeLists.txt
        set(cmake_list ${module_dir}/CMakeLists.txt)
        if (NOT EXISTS ${cmake_list})
            file(WRITE ${cmake_list}
                    [=[include(${NAP_ROOT}/cmake/nap_module.cmake)]=]
            )
        endif()
        file(RELATIVE_PATH module_subdirectory ${CMAKE_CURRENT_SOURCE_DIR} ${module_dir})
        add_subdirectory(${module_subdirectory})
    endif()
endfunction()


function(try_add_app_from_dir app_dir)
    if (EXISTS ${app_dir}/app.json)
        # We have an app!
        # Create CMakeLists.txt
        set(cmake_list ${app_dir}/CMakeLists.txt)
        if (NOT EXISTS ${cmake_list})
            file(WRITE ${cmake_list}
                    [=[include(${NAP_ROOT}/cmake/nap_app.cmake)]=]
            )
        endif()
        file(RELATIVE_PATH app_subdirectory ${CMAKE_CURRENT_SOURCE_DIR} ${app_dir})
        add_subdirectory(${app_subdirectory})
    endif()
endfunction()


function(add_license name license_path)
    # For dynamic builds only copy the license after the module is included and built.
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD COMMAND ${CMAKE_COMMAND} -E make_directory ${BIN_DIR}/license/${name})
    get_filename_component(license_filename ${license_path} NAME)
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy_if_different
            ${license_path}
            ${BIN_DIR}/license/${name}/${license_filename})
endfunction()


# Codesign the target's output file post build on MacOS
function(codesign_target target)
    if (APPLE)
        set(signature "-") # Use add hoc sign as default
        if (DEFINED ENV{MACOS_CODE_SIGNATURE})
            set(signature $ENV{MACOS_CODE_SIGNATURE}) # If defined, use environment variable as signature
        endif ()
        # Codesign the target post build
        add_custom_command(TARGET ${target} POST_BUILD
                COMMAND codesign --force -s ${signature} $<TARGET_FILE:${target}>)
    endif ()
endfunction()


# Codesign imported dynamic library
function(codesign path)
    if (APPLE)
        set(signature "-") # Use add hoc sign as default
        if (DEFINED ENV{MACOS_CODE_SIGNATURE})
            set(signature $ENV{MACOS_CODE_SIGNATURE}) # If defined, use environment variable as signature
        endif ()
        # Codesign the file
        execute_process(COMMAND codesign --force -s ${signature} ${path})
    endif ()
endfunction()


# Sets the rpath for a target's output executable or loading library
function(set_target_rpath target rpath)
    if (APPLE)
        # Update executable rpath if it hasn't been set already
        # Using a bash script, it checks if the '@executablepath/%{LIB_RPATH}/' already exists in the list displayed by 'otool -l'. If not, it calls the CMAKE_INSTALL_NAME_TOOL to install the name.
        add_custom_command(TARGET ${target}
                POST_BUILD COMMAND
                if ! otool -l $<TARGET_FILE:${target} | grep -q ${rpath}/.\; then ${CMAKE_INSTALL_NAME_TOOL} -add_rpath "${rpath}/." $<TARGET_FILE:${target}>\; fi)
    endif ()
endfunction()

