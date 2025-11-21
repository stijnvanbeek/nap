# Set this flag in order to link libsndfile and mpg123 and build support for reading audio files
set(NAP_AUDIOFILE_SUPPORT ON)

# Add sources to target
if (NAP_AUDIOFILE_SUPPORT)
    # Add compile definition to enable audio file support
    target_compile_definitions(${PROJECT_NAME} PRIVATE NAP_AUDIOFILE_SUPPORT)
else()
    # Filter out sources for audio file functionality
    set(AUDIO_FILE_SUPPORT_FILTER ".*audiofileutils.*" ".*audiofileresource.*")
endif()

add_source_dir("core" "src/audio/core")
add_source_dir("node" "src/audio/node")
add_source_dir("component" "src/audio/component")
add_source_dir("service" "src/audio/service")
add_source_dir("resource" "src/audio/resource" ${AUDIO_FILE_SUPPORT_FILTER})
add_source_dir("utility" "src/audio/utility" ${AUDIO_FILE_SUPPORT_FILTER})

add_subdirectory(thirdparty/libsamplerate)
target_link_import_library(${PROJECT_NAME} libsamplerate)

# Add thirdparty libraries for audio file support
if (NAP_AUDIOFILE_SUPPORT)
    add_subdirectory(thirdparty/libsndfile)
    target_link_import_library(${PROJECT_NAME} libsndfile)

    add_subdirectory(thirdparty/libflac)
    target_link_import_library(${PROJECT_NAME} libflac)

    add_subdirectory(thirdparty/libmp3lame)
    target_link_import_library(${PROJECT_NAME} libmp3lame)

    add_subdirectory(thirdparty/libmpg123)
    target_link_import_library(${PROJECT_NAME} libmpg123)

    add_subdirectory(thirdparty/libogg)
    target_link_import_library(${PROJECT_NAME} libogg)

    add_subdirectory(thirdparty/libopus)
    target_link_import_library(${PROJECT_NAME} libopus)

    add_subdirectory(thirdparty/libvorbis)
    target_link_import_library(${PROJECT_NAME} libvorbis)
    target_link_import_library(${PROJECT_NAME} libvorbisenc)
    target_link_import_library(${PROJECT_NAME} libvorbisfile)

    if (APPLE)
        list(APPEND LIBRARIES "-framework CoreFoundation")
    elseif(UNIX)
        list(APPEND LIBRARIES atomic)
    endif()
    target_link_libraries(${PROJECT_NAME} ${LIBRARIES})

    target_compile_definitions(${PROJECT_NAME} PRIVATE _USE_MATH_DEFINES)

    add_license(libsndfile ${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/libsndfile/source/COPYING)
endif()