include(${NAP_ROOT}/cmake/nap_utilities.cmake)
target_link_import_library(${PROJECT_NAME} SDL3)

if (BUILD_STATIC)
    link_import_library_static(SDL3)
endif ()