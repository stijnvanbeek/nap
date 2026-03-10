project(napcore)

file(GLOB SOURCES src/nap/*.cpp src/nap/*.h)

if(APPLE)
    list(APPEND SOURCES
        ${CMAKE_CURRENT_SOURCE_DIR}/src/nap/osx/directorywatcher.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/nap/osx/module.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/nap/osx/core_env.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/nap/native/modulecache_ext.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/nap/native/logger_ext.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/nap/native/core_ext.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/nap/native/resourcemanager_ext.cpp
    )
elseif(MSVC)
    list(APPEND SOURCES
        src/nap/win32/directorywatcher.cpp
        src/nap/win32/module.cpp
        src/nap/win32/core_env.cpp
        src/nap/native/modulecache_ext.cpp
        src/nap/native/logger_ext.cpp
        src/nap/native/core_ext.cpp
        src/nap/native/resourcemanager_ext.cpp
    )
elseif(UNIX)
    file(GLOB FILEWATCHER_SOURCES
                src/nap/linux/FileWatcher/*.cpp
                src/nap/linux/FileWatcher/*.h)
    list(APPEND SOURCES
        ${FILEWATCHER_SOURCES}
        src/nap/linux/directorywatcher.cpp
        src/nap/linux/module.cpp
        src/nap/linux/core_env.cpp
        src/nap/native/modulecache_ext.cpp
        src/nap/native/logger_ext.cpp
        src/nap/native/core_ext.cpp
        src/nap/native/resourcemanager_ext.cpp
    )
endif()
message("napcore project ${PROJECT_NAME}")
add_library(${PROJECT_NAME} SHARED ${SOURCES})

set_target_properties(${PROJECT_NAME} PROPERTIES INSTALL_RPATH "$ORIGIN")

set_target_properties(${PROJECT_NAME} PROPERTIES FOLDER Core)
target_compile_definitions(${PROJECT_NAME} PRIVATE NAP_SHARED_LIBRARY)

# Remove lib prefix on Unix libraries
set_target_properties(${PROJECT_NAME} PROPERTIES PREFIX "")

if (MSVC)
    include(GenerateExportheader)
    GENERATE_EXPORT_HEADER(${PROJECT_NAME})
endif()

target_compile_definitions(${PROJECT_NAME} PRIVATE MODULE_NAME=${PROJECT_NAME})
if(MSVC)
    target_compile_definitions(${PROJECT_NAME} PUBLIC NOMINMAX)
endif()

target_include_directories(${PROJECT_NAME} PUBLIC src ${CMAKE_CURRENT_BINARY_DIR})
target_link_libraries(${PROJECT_NAME} naprtti)

if (UNIX AND NOT APPLE)
    include_directories(${PROJECT_NAME} src/nap/linux/FileWatcher)
endif()

if(APPLE)
    target_link_libraries(${PROJECT_NAME} "-framework CoreServices")
elseif(UNIX)
    target_link_libraries(${PROJECT_NAME} dl pthread)
endif ()

if (BUILD_STATIC)
    set(STATIC_SOURCES ${SOURCES})
    list(FILTER STATIC_SOURCES EXCLUDE REGEX ".*${PROJECT_NAME}.cpp")
    target_sources(napstatic INTERFACE ${STATIC_SOURCES})
    target_include_directories(napstatic INTERFACE src)
    if (UNIX AND NOT APPLE)
        target_include_directories(napstatic INTERFACE src/nap/linux/FileWatcher)
    endif()
endif ()

codesign_target(${PROJECT_NAME})
install(FILES $<TARGET_FILE:${PROJECT_NAME}> TYPE LIB OPTIONAL)
