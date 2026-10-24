include(${NAP_ROOT}/cmake/nap_utilities.cmake)

add_subdirectory(thirdparty/ffmpeg)

target_link_import_library(${PROJECT_NAME} avcodec)
target_link_import_library(${PROJECT_NAME} avdevice)
target_link_import_library(${PROJECT_NAME} avfilter)
target_link_import_library(${PROJECT_NAME} avformat)
target_link_import_library(${PROJECT_NAME} avresample)
target_link_import_library(${PROJECT_NAME} avutil)
target_link_import_library(${PROJECT_NAME} swresample)
target_link_import_library(${PROJECT_NAME} swscale)

add_license(freetype ${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/ffmpeg/licenses/LICENSE.md)
