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

target_link_libraries(${PROJECT_NAME} debug ${SPIRVCROSS_LIBS_DEBUG} optimized ${SPIRVCROSS_LIBS_RELEASE})
target_link_libraries(${PROJECT_NAME} debug "${GLSLANG_LIBS_DEBUG}" optimized "${GLSLANG_LIBS_RELEASE}")

if(UNIX AND NOT APPLE)
    target_link_libraries(${PROJECT_NAME} atomic)
endif()

target_link_import_library(${static_target} assimp${static_suffix})
target_link_import_library(${static_target} SDL3${static_suffix})
target_link_import_library(${static_target} FreeImage${static_suffix})
target_link_import_library(${static_target} vulkan) # Also link shared vulkan lib for static

target_link_libraries(${static_target} INTERFACE debug ${SPIRVCROSS_LIBS_DEBUG} optimized ${SPIRVCROSS_LIBS_RELEASE})
target_link_libraries(${static_target} INTERFACE debug ${GLSLANG_LIBS_DEBUG} optimized ${GLSLANG_LIBS_RELEASE})
target_include_directories(${static_target} INTERFACE ${SPIRVCROSS_INCLUDE_DIR} ${GLSLANG_INCLUDE_DIR})

if(UNIX AND NOT APPLE)
    target_link_libraries(${static_target} INTERFACE atomic)
endif()

# Add include directories
target_include_directories(${PROJECT_NAME} PUBLIC ${SPIRVCROSS_INCLUDE_DIR} ${GLSLANG_INCLUDE_DIR})

# Set compile definitions
target_compile_definitions(${static_target} INTERFACE _USE_MATH_DEFINES)
if(APPLE)
    target_compile_definitions(${static_target} INTERFACE VK_USE_PLATFORM_METAL_EXT=1)
endif()
target_compile_definitions(${static_target} INTERFACE _USE_MATH_DEFINES)
if(APPLE)
    target_compile_definitions(${static_target} INTERFACE VK_USE_PLATFORM_METAL_EXT=1)
endif()

# Copy thirdparty licenses
add_license(assimp ${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/assimp/source/LICENSE)
add_license(FreeImage ${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/FreeImage/${NAP_THIRDPARTY_PLATFORM_DIR}/${ARCH}/license-fi.txt)
add_license(glslang ${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/glslang/source/LICENSE.txt)
add_license(SDL2 ${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/sdl/LICENSE.txt)
add_license(SPIRV-cross ${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/SPIRV-cross/source/LICENSE)
add_license(vulkansdk ${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/vulkansdk/LICENSE.txt)