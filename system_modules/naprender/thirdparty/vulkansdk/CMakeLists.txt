include(${NAP_ROOT}/cmake/nap_utilities.cmake)

project(vulkan)

set(platform_dir ${CMAKE_CURRENT_SOURCE_DIR}/${NAP_THIRDPARTY_PLATFORM_DIR}/${ARCH})
set(include_dir ${platform_dir}/include)

if (WIN32)

    set(implib ${platform_dir}/Lib/vulkan-1.lib)
    add_static_import_library(${PROJECT_NAME} ${implib} ${include_dir})

elseif (APPLE)

    set(dll ${platform_dir}/lib/libvulkan.dylib)
    add_import_library(${PROJECT_NAME} ${dll} ${dll} ${include_dir})

    # Link macos frameworks
    find_library(METAL_LIB Metal)
    find_library(FOUNDATION_LIB Foundation)
    find_library(QUARTZ_LIB QuartzCore)
    find_library(IOKIT_LIB IOKit)
    find_library(IOSURFACE_LIB IOSurface)
    target_link_libraries(${PROJECT_NAME} INTERFACE ${METAL_LIB} ${FOUNDATION_LIB} ${QUARTZ_LIB} ${IOKIT_LIB} ${IOSURFACE_LIB})

    # Copy MoltenVk to bin and codesign
    set(moltenvk_lib ${platform_dir}/lib/libMoltenVK.dylib)
    file(COPY ${moltenvk_lib} DESTINATION ${LIB_DIR})
    get_filename_component(MoltenVK_filename ${moltenvk_lib} NAME)
    set(MoltenVK_Dest ${LIB_DIR}/${MoltenVK_filename})
    codesign(${MoltenVK_Dest})
    install(FILES ${MoltenVK_Dest} TYPE LIB OPTIONAL)

    # Copy ICD json used to find MoltenVK library at runtime by the loader.
    file(COPY ${platform_dir}/share/vulkan/icd.d/MoltenVK_icd.json DESTINATION ${LIB_DIR})
    install(FILES ${LIB_DIR}/MoltenVK_icd.json DESTINATION ${CMAKE_INSTALL_MODULEINFODIR} OPTIONAL)

else ()

    set(implib ${platform_dir/lib/libvulkan.so})
    add_import_library(${PROJECT_NAME} ${implib} ${implib} ${include_dir})

endif ()