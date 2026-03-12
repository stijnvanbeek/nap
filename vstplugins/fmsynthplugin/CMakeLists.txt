include(${NAP_ROOT}/cmake/nap_vst.cmake)

set(plugin_type_name "nap::FMSynthPlugin")
add_vst()
target_link_libraries(${PROJECT_NAME} PRIVATE napimgui_static napparametergui_static napfmsynth_static napcontrol_static)