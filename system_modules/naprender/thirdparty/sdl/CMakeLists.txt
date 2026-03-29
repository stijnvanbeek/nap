include(${NAP_ROOT}/cmake/nap_utilities.cmake)

project(SDL3)

set(platform_dir ${CMAKE_CURRENT_SOURCE_DIR}/${NAP_THIRDPARTY_PLATFORM_DIR}/${ARCH})
if (WIN32)
    set(include_dir ${platform_dir}/include)
    set(lib_dir ${platform_dir}/bin)
    set(implib_dir ${platform_dir}/lib)
else ()
    set(include_dir ${platform_dir}/include)
    set(lib_dir ${platform_dir}/lib)
    set(implib_dir ${platform_dir}/lib)
endif()
set(implib ${implib_dir}/${implib_prefix}${PROJECT_NAME}${implib_suffix})
set(dll ${lib_dir}/${dll_prefix}${PROJECT_NAME}${dll_suffix})

add_import_library(${PROJECT_NAME} ${implib} ${dll} ${include_dir})

if (APPLE)
    set(bin_dir ${platform_dir}/bin)
    set(static_lib ${bin_dir}/${static_lib_prefix}${PROJECT_NAME}${static_lib_suffix})
    add_static_import_library(${PROJECT_NAME}${static_suffix} ${static_lib} ${include_dir})
    target_link_libraries(${PROJECT_NAME}${static_suffix} INTERFACE
            "-framework Cocoa"                # Common SDL deps
            "-framework IOKit"
            "-framework AVFoundation"         # Camera
            "-framework CoreMedia"
            "-framework CoreVideo"
            "-framework CoreAudio"            # Audio object APIs
            "-framework AudioToolbox"         # AudioQueue APIs
            "-framework GameController"       # Controllers
            "-framework CoreHaptics"          # Rumble
            "-framework ForceFeedback"        # Legacy FF APIs
            "-framework Carbon"               # HIToolbox (LMGetKbdType, TIS*)
            "-framework UniformTypeIdentifiers" # UTType
    )
elseif (WIN32)
    set(static_lib ${platform_dir}/static/SDL3-static.lib)
    add_static_import_library(${PROJECT_NAME}${static_suffix} ${static_lib} ${include_dir})
else()
    add_import_library(${PROJECT_NAME}${static_suffix} ${implib} ${dll} ${include_dir})
endif ()


#target_include_directories(${PROJECT_NAME} INTERFACE ${include_dir}/SDL3) # Makes sure sdl headers can be included within the SDL3 subfolder as well

