include (${NAP_ROOT}/cmake/nap_utilities.cmake)

add_subdirectory(thirdparty/assimp)
add_subdirectory(thirdparty/sdl)
add_subdirectory(thirdparty/FreeImage)
add_subdirectory(thirdparty/vulkansdk)

find_package(SPIRVCross REQUIRED)
find_package(glslang REQUIRED)

# Link dependencies
target_link_import_library(${PROJECT_NAME} assimp)
target_link_import_library(${PROJECT_NAME} SDL3)
target_link_import_library(${PROJECT_NAME} FreeImage)
target_link_import_library(${PROJECT_NAME} vulkan)

if(UNIX AND NOT APPLE AND ${ARCH} STREQUAL "armhf")
    target_link_libraries(${PROJECT_NAME} atomic)
endif()

target_link_libraries(${PROJECT_NAME} debug ${SPIRVCROSS_LIBS_DEBUG} optimized ${SPIRVCROSS_LIBS_RELEASE})
target_link_libraries(${PROJECT_NAME} debug "${GLSLANG_LIBS_DEBUG}" optimized "${GLSLANG_LIBS_RELEASE}")

# Add includes
target_include_directories(${PROJECT_NAME} PUBLIC ${SPIRVCROSS_INCLUDE_DIR} ${GLSLANG_INCLUDE_DIR})

if(BUILD_STATIC)
    link_import_library_static(assimp)
    link_import_library_static(SDL3)
    link_import_library_static(FreeImage)
    link_import_library_static(vulkan)
    target_link_libraries(napstatic INTERFACE debug ${SPIRVCROSS_LIBS_DEBUG} optimized ${SPIRVCROSS_LIBS_RELEASE})
    target_link_libraries(napstatic INTERFACE debug ${GLSLANG_LIBS_DEBUG} optimized ${GLSLANG_LIBS_RELEASE})
    target_include_directories(napstatic INTERFACE ${SPIRVCROSS_INCLUDE_DIR} ${GLSLANG_INCLUDE_DIR})
    if(UNIX AND NOT APPLE AND ${ARCH} STREQUAL "armhf")
        target_link_libraries(napstatic INTERFACE atomic)
    endif()
endif()

# Set compile definitions
target_compile_definitions(${PROJECT_NAME} PRIVATE _USE_MATH_DEFINES)
if(APPLE)
    target_compile_definitions(${PROJECT_NAME} PUBLIC VK_USE_PLATFORM_METAL_EXT=1)
endif()

# Copy thirdparty licenses
add_license(assimp ${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/assimp/source/LICENSE)
add_license(FreeImage ${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/FreeImage/${NAP_THIRDPARTY_PLATFORM_DIR}/${ARCH}/license-fi.txt)
add_license(glslang ${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/glslang/source/LICENSE.txt)
add_license(SDL2 ${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/sdl/LICENSE.txt)
add_license(SPIRV-cross ${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/SPIRV-cross/source/LICENSE)
add_license(vulkansdk ${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/vulkansdk/LICENSE.txt)