include(${NAP_ROOT}/cmake/nap_utilities.cmake)

project(SDL3)

set(platform_dir ${CMAKE_CURRENT_SOURCE_DIR}/${NAP_THIRDPARTY_PLATFORM_DIR}/${ARCH})
if (WIN32)
    set(include_dir ${platform_dir}/include)
    set(lib_dir ${platform_dir}/bin)
    set(implib_dir ${platform_dir}/lib)
else ()
    set(include_dir ${platform_dir}/include ${platform_dir}/include)
    set(lib_dir ${platform_dir}/lib)
    set(implib_dir ${platform_dir}/lib)
endif()
set(implib ${implib_dir}/${implib_prefix}${PROJECT_NAME}${implib_suffix})
set(dll ${lib_dir}/${dll_prefix}${PROJECT_NAME}${dll_suffix})

add_import_library(${PROJECT_NAME} ${implib} ${dll} ${include_dir})
target_include_directories(${PROJECT_NAME} INTERFACE ${include_dir}/SDL3) # Makes sure sdl headers can be included within the SDL3 subfolder as well