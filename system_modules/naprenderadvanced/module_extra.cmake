# Skip certain features when building against Raspberry Pi 4
if (RPI_MODEL)
    target_compile_definitions(${PROJECT_NAME} PRIVATE RASPBERRY_PI)
endif()
