include(${NAP_ROOT}/cmake/nap_utilities.cmake)

target_link_import_library(${PROJECT_NAME} SDL3)
target_link_import_library(${static_target} SDL3)
