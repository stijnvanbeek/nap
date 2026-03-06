include (${NAP_ROOT}/cmake/nap_utilities.cmake)

# Find other package dependencies
find_package(SPIRVCross REQUIRED)
find_package(glslang REQUIRED)

target_link_libraries(${PROJECT_NAME} debug ${SPIRVCROSS_LIBS_DEBUG} optimized ${SPIRVCROSS_LIBS_RELEASE})
target_link_libraries(${PROJECT_NAME} debug "${GLSLANG_LIBS_DEBUG}" optimized "${GLSLANG_LIBS_RELEASE}")

add_subdirectory(thirdparty/assimp)
add_subdirectory(thirdparty/sdl)
add_subdirectory(thirdparty/FreeImage)
add_subdirectory(thirdparty/vulkansdk)

target_link_import_library(${PROJECT_NAME} assimp)
target_link_import_library(${PROJECT_NAME} SDL3)
target_link_import_library(${PROJECT_NAME} FreeImage)
target_link_import_library(${PROJECT_NAME} vulkan)

# Add includes
set(INCLUDES ${VULKANSDK_INCLUDE_DIRS} ${SPIRVCROSS_INCLUDE_DIR} ${GLSLANG_INCLUDE_DIR})
target_include_directories(${PROJECT_NAME} PUBLIC ${INCLUDES})

# Set compile definitions
target_compile_definitions(${PROJECT_NAME} PRIVATE _USE_MATH_DEFINES)

if(APPLE)
    find_library(METAL_LIB Metal)
    find_library(FOUNDATION_LIB Foundation)
    find_library(QUARTZ_LIB QuartzCore)
    find_library(IOKIT_LIB IOKit)
    find_library(IOSURFACE_LIB IOSurface)
    set(moltenvk_dependencies ${METAL_LIB} ${FOUNDATION_LIB} ${QUARTZ_LIB} ${IOKIT_LIB} ${IOSURFACE_LIB} "-framework CoreGraphics" "-framework AppKit")
    target_link_libraries(${PROJECT_NAME} ${moltenvk_dependencies})
    target_compile_definitions(${PROJECT_NAME} PUBLIC VK_USE_PLATFORM_METAL_EXT=1)
endif()

if(UNIX AND NOT APPLE AND ${ARCH} STREQUAL "armhf")
    list(APPEND LIBRARIES atomic)
endif()

if (BUILD_STATIC)
    # Link naprender dependencies to napstatic
    target_link_import_library(napstatic assimp)
    target_link_import_library(napstatic SDL3)
    target_link_import_library(napstatic FreeImage)
    target_link_import_library(napstatic vulkan)

    target_link_libraries(napstatic debug ${SPIRVCROSS_LIBS_DEBUG} optimized ${SPIRVCROSS_LIBS_RELEASE})
    target_link_libraries(napstatic debug "${GLSLANG_LIBS_DEBUG}" optimized "${GLSLANG_LIBS_RELEASE}")

    if(APPLE)
        target_compile_definitions(napstatic PUBLIC VK_USE_PLATFORM_METAL_EXT=1)
        target_link_libraries(napstatic ${moltenvk_dependencies})
    endif()

    # Add naprender includes to napstatic
    target_include_directories(napstatic PUBLIC ${INCLUDES})
endif ()

if (APPLE)
    get_target_property(MOLTENVK_LIB moltenvk IMPORTED_LOCATION)
    codesign(${MOLTENVK_LIB})
    add_custom_command(
            TARGET ${PROJECT_NAME} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy
            ${MOLTENVK_LIB}
            ${LIB_DIR})
    install(FILES ${MOLTENVK_LIB} TYPE LIB OPTIONAL)

    # Copy MoltenVK_icd.json to bin and install
    add_custom_command(
            TARGET ${PROJECT_NAME} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy
            ${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/vulkansdk/macos/universal/share/vulkan/icd.d/MoltenVK_icd.json
            ${LIB_DIR}/MoltenVK_icd.json)
    install(FILES ${LIB_DIR}/MoltenVK_icd.json DESTINATION ${CMAKE_INSTALL_MODULEINFODIR} OPTIONAL)
    #install(FILES ${LIB_DIR}/MoltenVK_icd.json TYPE DATA OPTIONAL)
endif()

# Copy thirdparty licenses
add_license(assimp ${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/assimp/source/LICENSE)
add_license(FreeImage ${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/FreeImage/${NAP_THIRDPARTY_PLATFORM_DIR}/${ARCH}/license-fi.txt)
add_license(glslang ${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/glslang/source/LICENSE.txt)
add_license(SDL2 ${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/sdl/LICENSE.txt)
add_license(SPIRV-cross ${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/SPIRV-cross/source/LICENSE)
add_license(vulkansdk ${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/vulkansdk/LICENSE.txt)