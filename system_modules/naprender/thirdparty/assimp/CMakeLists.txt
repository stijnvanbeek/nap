include(${NAP_ROOT}/cmake/nap_utilities.cmake)

project(assimp)

set(platform_dir ${CMAKE_CURRENT_SOURCE_DIR}/${NAP_THIRDPARTY_PLATFORM_DIR}/${ARCH})
set(include_dir ${platform_dir}/include)
if (WIN32)
    set(implib ${platform_dir}/lib/assimp-vc140-mt.lib)
    set(dll ${platform_dir}/bin/assimp-vc140-mt.dll)
else ()
    set(lib_dir ${platform_dir}/lib)
    set(implib ${lib_dir}/${implib_prefix}${PROJECT_NAME}${implib_suffix})
    set(dll ${lib_dir}/${dll_prefix}${PROJECT_NAME}${dll_suffix})
endif()

add_import_library(${PROJECT_NAME} ${implib} ${dll} ${include_dir})

if (APPLE)
    set(bin_dir ${platform_dir}/bin)
    set(static_lib ${bin_dir}/${static_lib_prefix}${PROJECT_NAME}${static_lib_suffix})
    add_static_import_library(${PROJECT_NAME}${static_suffix} ${static_lib} ${include_dir})
elseif(WIN32)
    set(bin_dir ${platform_dir}/static)
    set(static_lib ${bin_dir}/assimp-vc143-mt.lib)
    add_static_import_library(${PROJECT_NAME}${static_suffix} ${static_lib} ${include_dir})
elseif ()
    add_import_library(${PROJECT_NAME}${static_suffix} ${implib} ${dll} ${include_dir}) # Use shared library for static builds for now
endif ()

