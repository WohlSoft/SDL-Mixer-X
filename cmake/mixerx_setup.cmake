if(NOT CMAKE_VERSION VERSION_LESS 2.8.12)
    set(CMAKE_MACOSX_RPATH 0)
endif()

if(${CMAKE_SYSTEM_NAME} STREQUAL "Emscripten")
    set(EMSCRIPTEN 1 BOOLEAN)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -s USE_SDL=0 -s USE_SDL_MIXER=0")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -s USE_SDL=0 -s USE_SDL_MIXER=0")
endif()

# Define iOS/tvOS/visionOS/watchOS platform markers if they are missing (at some old CMake toolchains)
if(${CMAKE_SYSTEM_NAME} STREQUAL "iOS")
    if(NOT DEFINED IOS)
        set(IOS TRUE)
    endif()

    if(NOT DEFINED PLATFORM_IOS)
        set(PLATFORM_IOS TRUE)
    endif()
elseif(${CMAKE_SYSTEM_NAME} STREQUAL "tvOS")
    if(NOT DEFINED TVOS)
        set(TVOS TRUE)
    endif()

    if(NOT DEFINED PLATFORM_TVOS)
        set(PLATFORM_TVOS TRUE)
    endif()
elseif(${CMAKE_SYSTEM_NAME} STREQUAL "watchOS")
    if(NOT DEFINED WATCHOS)
        set(WATCHOS TRUE)
    endif()
elseif(${CMAKE_SYSTEM_NAME} STREQUAL "visionOS")
    if(NOT DEFINED VISIONOS)
        set(VISIONOS TRUE)
    endif()
elseif(APPLE AND NOT IOS AND NOT TVOS AND NOT VISIONOS AND NOT WATCHOS)
    if(NOT DEFINED MACOS)
        set(MACOS 1)
    endif()
endif()

if(NOT CMAKE_BUILD_TYPE)
    message("== Using default build configuration ==")
endif()

string(TOLOWER "${CMAKE_BUILD_TYPE}" CMAKE_BUILD_TYPE_LOWER)

if(CMAKE_BUILD_TYPE_LOWER STREQUAL "debug")
    set(MIX_DEBUG_SUFFIX d)
else()
    set(MIX_DEBUG_SUFFIX "")
endif()

if(WIN32 AND NOT EMSCRIPTEN)
    set(CMAKE_SHARED_LIBRARY_PREFIX "")
endif()

if(POLICY CMP0058)
    cmake_policy(SET CMP0058 NEW)
endif()


set(FIND_PREFER_STATIC
    "-static${MIX_DEBUG_SUFFIX}.a"
    "-static${MIX_DEBUG_SUFFIX}.lib"
    "${MIX_DEBUG_SUFFIX}.a"
    "${MIX_DEBUG_SUFFIX}.lib"
    "-static.a"
    "-static.lib"
    ".a"
    ".lib"
    "${MIX_DEBUG_SUFFIX}.dll.a"
    "${MIX_DEBUG_SUFFIX}.lib"
    ".dll.a"
    ".lib"
    "${MIX_DEBUG_SUFFIX}.so"
    "${MIX_DEBUG_SUFFIX}.dylib"
    ".so"
    ".dylib"
)

set(FIND_PREFER_SHARED
    "${MIX_DEBUG_SUFFIX}.dll.a"
    "${MIX_DEBUG_SUFFIX}.lib"
    ".dll.a"
    ".lib"
    "${MIX_DEBUG_SUFFIX}.so"
    "${MIX_DEBUG_SUFFIX}.dylib"
    ".so"
    ".dylib"
    "-static${MIX_DEBUG_SUFFIX}.a"
    "-static${MIX_DEBUG_SUFFIX}.lib"
    "${MIX_DEBUG_SUFFIX}.a"
    "${MIX_DEBUG_SUFFIX}.lib"
    "-static.a"
    "-static.lib"
    ".a"
    ".lib"
)
