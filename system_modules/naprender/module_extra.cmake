include (${NAP_ROOT}/cmake/nap_utilities.cmake)

add_subdirectory(thirdparty/assimp)
add_subdirectory(thirdparty/sdl)
add_subdirectory(thirdparty/FreeImage)
add_subdirectory(thirdparty/vulkansdk)

find_package(SPIRVCross REQUIRED)
find_package(glslang REQUIRED)

macro(link_dependencies target)
    # Link dependencies
    target_link_import_library(${target} assimp)
    target_link_import_library(${target} SDL3)
    target_link_import_library(${target} FreeImage)
    target_link_import_library(${target} vulkan)

    if(UNIX AND NOT APPLE AND ${ARCH} STREQUAL "armhf")
        target_link_libraries(${target} atomic)
    endif()

    target_link_libraries(${target} debug ${SPIRVCROSS_LIBS_DEBUG} optimized ${SPIRVCROSS_LIBS_RELEASE})
    target_link_libraries(${target} debug "${GLSLANG_LIBS_DEBUG}" optimized "${GLSLANG_LIBS_RELEASE}")

    # Add includes
    target_include_directories(${target} PUBLIC ${SPIRVCROSS_INCLUDE_DIR} ${GLSLANG_INCLUDE_DIR})
endmacro()

link_dependencies(${PROJECT_NAME})
if(BUILD_STATIC)
    link_dependencies(napstatic)
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