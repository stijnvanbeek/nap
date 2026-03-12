include(${NAP_ROOT}/cmake/nap_vst.cmake)

add_vst()
target_link_libraries(${PROJECT_NAME} PRIVATE napparametergui napaudio napmidi napcontrol)

