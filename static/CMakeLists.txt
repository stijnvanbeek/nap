project(napstatic)

set(rapidjson_FIND_QUIETLY TRUE)
find_package(rapidjson REQUIRED)
add_definitions(-DRAPIDJSON_HAS_STDSTRING=1)

# Find RTTR::Core through its provided cmake files using the package finder
set(RTTR_DIR "${NAP_ROOT}/thirdparty/rttr/${NAP_THIRDPARTY_PLATFORM_DIR}/${ARCH}/cmake")
find_package(RTTR CONFIG REQUIRED Core_Lib)

add_library(${PROJECT_NAME} STATIC src/napstatic.cpp)

target_link_libraries(${PROJECT_NAME} naputility RTTR::Core_Lib)
target_include_directories(${PROJECT_NAME} PUBLIC ${RAPIDJSON_INCLUDE_DIRS})

if(APPLE)
    target_link_libraries(${PROJECT_NAME} "-framework CoreServices")
elseif(UNIX)
    target_link_libraries(${PROJECT_NAME} dl pthread)
endif ()

if(MSVC)
    target_compile_definitions(${PROJECT_NAME} PUBLIC NOMINMAX)
endif()

