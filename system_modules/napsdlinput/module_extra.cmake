include(${NAP_ROOT}/cmake/nap_utilities.cmake)
target_link_import_library(${PROJECT_NAME} SDL3)

if (BUILD_STATIC)
    add_static_linked_target(SDL3)
endif ()