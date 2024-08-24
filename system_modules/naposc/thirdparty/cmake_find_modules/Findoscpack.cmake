# default oscpack directory
find_path(OSCPACK_DIR
          NO_CMAKE_FIND_ROOT_PATH
          NAMES
          msvc/x86_64/include/oscpack/osc/OscTypes.h
          macos/${ARCH}/include/oscpack/osc/OscTypes.h
          linux/${ARCH}/include/oscpack/osc/OscTypes.h
          HINTS
          ${NAP_ROOT}/modules/naposc/thirdparty/oscpack
          )

if(WIN32)
    set(OSCPACK_LIBS_DIR ${OSCPACK_DIR}/msvc/x86_64/lib)
    set(OSCPACK_LIBS_DEBUG ${OSCPACK_LIBS_DIR}/Debug/oscpack.lib Ws2_32 winmm)
    set(OSCPACK_LIBS_RELEASE ${OSCPACK_LIBS_DIR}/Release/oscpack.lib Ws2_32 winmm)
    set(OSCPACK_INCLUDE_DIRS ${OSCPACK_DIR}/msvc/x86_64/include/oscpack)
elseif(APPLE)
    set(OSCPACK_LIBS_DIR ${OSCPACK_DIR}/macos/${ARCH}/lib)
    set(OSCPACK_LIBS_RELEASE ${OSCPACK_LIBS_DIR}/Release/liboscpack.dylib)
    set(OSCPACK_LIBS_DEBUG ${OSCPACK_LIBS_RELEASE})
    set(OSCPACK_INCLUDE_DIRS ${OSCPACK_DIR}/macos/${ARCH}/include/oscpack)
else()
    set(OSCPACK_LIBS_DIR ${OSCPACK_DIR}/linux/${ARCH}/lib)
    set(OSCPACK_LIBS_DEBUG ${OSCPACK_LIBS_DIR}/liboscpack.so)
    set(OSCPACK_LIBS_RELEASE ${OSCPACK_LIBS_DIR}/liboscpack.so)
    set(OSCPACK_INCLUDE_DIRS ${OSCPACK_DIR}/linux/${ARCH}/include/oscpack)
endif()

set(OSCPACK_LICENSE_FILES ${OSCPACK_DIR}/source/LICENSE)

mark_as_advanced(OSCPACK_INCLUDE_DIRS)

# promote package for find
include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(oscpack REQUIRED_VARS OSCPACK_DIR OSCPACK_LIBS_DIR OSCPACK_INCLUDE_DIRS)

add_library(oscpack SHARED IMPORTED)
set_target_properties(oscpack PROPERTIES
        IMPORTED_CONFIGURATIONS "Debug;Release;MinSizeRel;RelWithDebInfo"
        IMPORTED_LOCATION_RELEASE ${OSCPACK_LIBS_RELEASE}
        IMPORTED_LOCATION_DEBUG ${OSCPACK_LIBS_RELEASE}
        IMPORTED_LOCATION_MINSIZEREL ${OSCPACK_LIBS_RELEASE}
        IMPORTED_LOCATION_RELWITHDEBINFO ${OSCPACK_LIBS_RELEASE}
)

