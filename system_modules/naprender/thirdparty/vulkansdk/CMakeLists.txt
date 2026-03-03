include(${NAP_ROOT}/cmake/nap_utilities.cmake)

project(vulkan)

set(platform_dir ${CMAKE_CURRENT_SOURCE_DIR}/${NAP_THIRDPARTY_PLATFORM_DIR}/${ARCH})
set(include_dir ${platform_dir}/include)

if (WIN32)
    set(implib ${platform_dir}/Lib/vulkan-1.lib)
    add_static_import_library(${PROJECT_NAME} ${implib} ${include_dir})
elseif (APPLE)
    set(implib ${platform_dir}/lib/LibMoltenVK.a)
    add_static_import_library(${PROJECT_NAME} ${implib} ${include_dir})
else ()
    set(implib ${platform_dir/lib/libvulkan.so})
    add_import_library(${PROJECT_NAME} ${implib} ${implib} ${include_dir})
endif ()