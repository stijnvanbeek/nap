include(${NAP_ROOT}/cmake/nap_vst.cmake)

add_vst()
target_link_libraries(${PROJECT_NAME} PRIVATE napimgui_static napparametergui_static napfmsynth_static napcontrol_static)