project(naprtti)

file(GLOB_RECURSE SOURCES src/*.cpp src/*.h)

# Find RTTR::Core through its provided cmake files using the package finder
set(RTTR_DIR "${NAP_ROOT}/thirdparty/rttr/${NAP_THIRDPARTY_PLATFORM_DIR}/${ARCH}/cmake")
find_package(RTTR CONFIG REQUIRED Core)

set(rapidjson_FIND_QUIETLY TRUE)
find_package(rapidjson REQUIRED)
add_definitions(-DRAPIDJSON_HAS_STDSTRING=1)

add_library(${PROJECT_NAME} SHARED ${SOURCES})
if (BUILD_STATIC)
    set(STATIC_SOURCES ${SOURCES})
    list(FILTER STATIC_SOURCES EXCLUDE REGEX ".*${PROJECT_NAME}.cpp")
    target_sources(napstatic PUBLIC ${STATIC_SOURCES})
    target_include_directories(napstatic PUBLIC src)
endif ()
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

target_link_libraries(${PROJECT_NAME} naputility RTTR::Core)
if(APPLE)
    target_link_libraries(${PROJECT_NAME} "-framework CoreServices")
elseif(UNIX)
    target_link_libraries(${PROJECT_NAME} dl pthread)
endif ()

# Codesign the shared libraries
get_target_property(rttr_library_path RTTR::Core IMPORTED_LOCATION_RELEASE)
file(COPY ${rttr_library_path} DESTINATION ${LIB_DIR})
get_filename_component(rttr_filename ${rttr_library_path} NAME)
set(rttr_bin_path ${LIB_DIR}/${rttr_filename})

codesign(${rttr_bin_path})
codesign_target(${PROJECT_NAME})

# Package RTTR::Core libraries
install(FILES ${rttr_bin_path} TYPE LIB OPTIONAL)

# Package naprtti library
install(FILES $<TARGET_FILE:${PROJECT_NAME}> TYPE LIB OPTIONAL)
