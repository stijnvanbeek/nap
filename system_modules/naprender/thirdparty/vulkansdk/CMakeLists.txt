include(${NAP_ROOT}/cmake/nap_utilities.cmake)

project(vulkan)

set(platform_dir ${CMAKE_CURRENT_SOURCE_DIR}/${NAP_THIRDPARTY_PLATFORM_DIR}/${ARCH})
set(include_dir ${platform_dir}/include)

if (WIN32)

    set(implib ${platform_dir}/Lib/vulkan-1.lib)
    add_static_import_library(${PROJECT_NAME} ${implib} ${include_dir})

elseif (APPLE)

#    set(dll ${platform_dir}/lib/libvulkan.dylib)
#    add_import_library(${PROJECT_NAME} ${dll} ${dll} ${include_dir})

    set(moltenvk_lib ${platform_dir}/lib/libMoltenVK.dylib)
    add_import_library(${PROJECT_NAME} ${moltenvk_lib} ${moltenvk_lib} ${include_dir})

#    set(moltenvk_lib_static ${platform_dir}/lib/libMoltenVK.a)
#    add_static_import_library(${PROJECT_NAME}${static_suffix} ${moltenvk_lib} ${include_dir})

    # Link macos frameworks
    find_library(METAL_LIB Metal)
    find_library(FOUNDATION_LIB Foundation)
    find_library(QUARTZ_LIB QuartzCore)
    find_library(IOKIT_LIB IOKit)
    find_library(IOSURFACE_LIB IOSurface)
    target_link_libraries(${PROJECT_NAME} INTERFACE ${METAL_LIB} ${FOUNDATION_LIB} ${QUARTZ_LIB} ${IOKIT_LIB} ${IOSURFACE_LIB})
#    target_link_libraries(${PROJECT_NAME}${static_suffix} INTERFACE ${METAL_LIB} ${FOUNDATION_LIB} ${QUARTZ_LIB} ${IOKIT_LIB} ${IOSURFACE_LIB})

else ()

    set(implib ${platform_dir}/lib/libvulkan.so)
    add_import_library(${PROJECT_NAME} ${implib} ${implib} ${include_dir})

endif ()