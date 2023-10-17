macro(add_napmodule_static MODULE_DIR)
    set(source_dir ${NAP_ROOT}/${MODULE_DIR}/src)
    file(GLOB_RECURSE SOURCES ${source_dir}/*.cpp ${source_dir}/*.h)
#    filter_platform_specific_files(SOURCES)
#    add_platform_specific_files("${WIN32_SOURCES}" "${MACOS_SOURCES}" "${LINUX_SOURCES}")
    list(FILTER SOURCES EXCLUDE REGEX "mod_.*")
    target_sources(${PROJECT_NAME} PUBLIC ${SOURCES})
    target_include_directories(${PROJECT_NAME} PUBLIC ${source_dir})
    message(STATUS ${source_dir})
#    message(STATUS ${SOURCES})
endmacro()

macro(add_napcore_static)
    set(source_dir ${NAP_ROOT}/core/src)

    file(GLOB SOURCES ${source_dir}/nap/*.cpp src/nap/*.h)

    # Add whether we're a NAP release or not
    configure_file(${source_dir}/nap/packaginginfo.h.in packaginginfo.h @ONLY)
    list(APPEND SOURCES ${CMAKE_CURRENT_BINARY_DIR}/packaginginfo.h)

    # Populate version info
    include(${NAP_ROOT}/cmake/version.cmake)
    configure_file(${source_dir}/nap/version.h.in version.h @ONLY)

    list(APPEND SOURCES ${CMAKE_CURRENT_BINARY_DIR}/version.h)

    if(APPLE)
        list(APPEND
                SOURCES
                ${source_dir}/nap/osx/directorywatcher.cpp
                ${source_dir}/nap/osx/module.cpp
                ${source_dir}/nap/osx/core_env.cpp
                ${source_dir}/nap/native/modulemanager_ext.cpp
                ${source_dir}/nap/native/logger_ext.cpp
                ${source_dir}/nap/native/core_ext.cpp
                ${source_dir}/nap/native/resourcemanager_ext.cpp
                )
    elseif(MSVC)
        list(APPEND
                SOURCES
                ${source_dir}/nap/win32/directorywatcher.cpp
                ${source_dir}/nap/win32/module.cpp
                ${source_dir}/nap/win32/core_env.cpp
                ${source_dir}/nap/native/modulemanager_ext.cpp
                ${source_dir}/nap/native/logger_ext.cpp
                ${source_dir}/nap/native/core_ext.cpp
                ${source_dir}/nap/native/resourcemanager_ext.cpp
                )
    elseif(UNIX)
        file(GLOB FILEWATCHER_SOURCES
                ${source_dir}/nap/linux/FileWatcher/*.cpp
                ${source_dir}/nap/linux/FileWatcher/*.h)
        list(APPEND
                SOURCES
                ${FILEWATCHER_SOURCES}
                ${source_dir}/nap/linux/directorywatcher.cpp
                ${source_dir}/nap/linux/module.cpp
                ${source_dir}/nap/linux/core_env.cpp
                ${source_dir}/nap/native/modulemanager_ext.cpp
                ${source_dir}/nap/native/logger_ext.cpp
                ${source_dir}/nap/native/core_ext.cpp
                ${source_dir}/nap/native/resourcemanager_ext.cpp
                )
    endif()
    target_sources(${PROJECT_NAME} PUBLIC ${SOURCES})
    target_include_directories(${PROJECT_NAME} PUBLIC ${source_dir})
endmacro()