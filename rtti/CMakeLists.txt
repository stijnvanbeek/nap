project(naprtti)

file(GLOB_RECURSE SOURCES src/*.cpp src/*.h)

# Find RTTR::Core through its provided cmake files using the package finder
set(RTTR_DIR "${NAP_ROOT}/thirdparty/rttr/${NAP_THIRDPARTY_PLATFORM_DIR}/${ARCH}/cmake")
find_package(RTTR CONFIG REQUIRED Core)

set(rapidjson_FIND_QUIETLY TRUE)
find_package(rapidjson REQUIRED)

add_library(${PROJECT_NAME} SHARED ${SOURCES})
set(static_target ${PROJECT_NAME}${static_suffix})
add_library(${static_target} INTERFACE)

set(STATIC_SOURCES ${SOURCES})
list(FILTER STATIC_SOURCES EXCLUDE REGEX ".*${PROJECT_NAME}.cpp")
target_sources(${static_target} INTERFACE ${STATIC_SOURCES})

target_compile_definitions(${PROJECT_NAME} PUBLIC -DRAPIDJSON_HAS_STDSTRING=1)
target_compile_definitions(${static_target} INTERFACE -DRAPIDJSON_HAS_STDSTRING=1)

set_target_properties(${PROJECT_NAME} PROPERTIES FOLDER Core)
set_target_properties(${PROJECT_NAME} PROPERTIES INSTALL_RPATH "$ORIGIN")

# Remove lib prefix on Unix libraries
set_target_properties(${PROJECT_NAME} PROPERTIES PREFIX "")

if (MSVC)
    include(GenerateExportheader)
    GENERATE_EXPORT_HEADER(${PROJECT_NAME})
endif()

target_compile_definitions(${PROJECT_NAME} PRIVATE MODULE_NAME=${PROJECT_NAME} NAP_SHARED_LIBRARY)
target_include_directories(${PROJECT_NAME} PUBLIC src)
target_include_directories(${PROJECT_NAME} PUBLIC ${RAPIDJSON_INCLUDE_DIRS})
target_include_directories(${static_target} INTERFACE src)
target_include_directories(${static_target} INTERFACE ${RAPIDJSON_INCLUDE_DIRS})

target_link_libraries(${PROJECT_NAME} naputility RTTR::Core)
if(APPLE)
    find_package(RTTR CONFIG REQUIRED Core_Lib)
    target_link_libraries(${PROJECT_NAME} "-framework CoreServices")
    target_link_libraries(${static_target} INTERFACE naputility RTTR::Core_Lib)
    target_link_libraries(${static_target} INTERFACE "-framework CoreServices")
elseif(UNIX)
    target_link_libraries(${PROJECT_NAME} dl pthread)
    target_link_libraries(${static_target} INTERFACE naputility RTTR::Core)
    target_link_libraries(${static_target} INTERFACE dl pthread)
elseif (WIN32)
    set(static_lib_path ${THIRDPARTY_DIR}/rttr/msvc/x86_64/static/librttr_core.a)
    set(rttr_include_dir ${THIRDPARTY_DIR}/rttr/msvc/x86_64/include)
    target_link_libraries(${static_target} INTERFACE naputility ${static_lib_path})
    target_include_directories(${static_target} INTERFACE ${rttr_include_dir})
endif ()

# Copy rttr library to bin
if (UNIX)
    if (APPLE)
        get_target_property(rttr_library_path RTTR::Core IMPORTED_LOCATION_RELEASE)
    else ()
        # On linux we have to get the harcoded path because IMPORTED_LOCATION_RELEASE does not work
        set(rttr_library_path ${THIRDPARTY_DIR}/rttr/linux/${ARCH}/bin/librttr_core.so.0.9.6)
    endif ()
    get_filename_component(rttr_filename ${rttr_library_path} NAME)
    set(rttr_bin_path ${LIB_DIR}/${rttr_filename})

    # Copy to bin, update install name and codesign before building the target
    copy_import_library_to_bin(${PROJECT_NAME} ${rttr_library_path})

    # Package RTTR::Core libraries
    install(FILES ${rttr_bin_path} TYPE LIB OPTIONAL)
else ()
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:RTTR::Core> ${LIB_DIR}
    )
    install(FILES $<TARGET_FILE:RTTR::Core> TYPE LIB OPTIONAL)
endif ()

codesign_target(${PROJECT_NAME})

# Package naprtti library
install(FILES $<TARGET_FILE:${PROJECT_NAME}> TYPE LIB OPTIONAL)
