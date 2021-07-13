cmake_minimum_required (VERSION 2.8.12)

if(Vita)
    message("VITASDK Defined!")
    set(CMAKE_TOOLCHAIN_FILE "$ENV{VITASDK}/share/vita.toolchain.cmake" CACHE PATH "toolchain file")
    include("$ENV{VITASDK}/share/vita.cmake" REQUIRED)

    set(VITA 1)
    # set(VITA_APP_NAME "TheXTech Vita Edition")
    # set(VITA_TITLEID  "THEXTECH00001")
    # set(VITA_VERSION  "01.00")

    # set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=gnu11")
    # set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

    set(AUDIO_CODECS_REPO_PATH "")
    set(AUDIO_CODECS_INSTALL_PATH "")
    cmake_policy(SET CMP0077 OLD)
endif()

project(SDLMixerX C CXX)

set(SDLMixerX_VERSION_MAJOR 2)
set(SDLMixerX_VERSION_MINOR 5)
set(SDLMixerX_VERSION_PATCH 0)
set(SDLMixerX_VERSION_TWEAK 0)
set(SDLMixerX_VERSION      "${SDLMixerX_VERSION_MAJOR}.${SDLMixerX_VERSION_MINOR}.${SDLMixerX_VERSION_PATCH}.${SDLMixerX_VERSION_TWEAK}")

include(GNUInstallDirs)
include(FindPkgConfig)
include(CheckIncludeFiles)
include(CheckFunctionExists)
include(CheckLibraryExists)
include(CheckCCompilerFlag)
#include(CheckCSourceRuns)

set(CMAKE_MODULE_PATH "${SDLMixerX_SOURCE_DIR}/cmake;${SDLMixerX_SOURCE_DIR}/cmake/find;${SDLMixerX_SOURCE_DIR}/src/codecs;${CMAKE_MODULE_PATH}")

include(CppNeedCheck)
include(WhichLicense)

if(NOT CMAKE_VERSION VERSION_LESS 2.8.12)
    set(CMAKE_MACOSX_RPATH 0)
endif()

if(${CMAKE_SYSTEM_NAME} STREQUAL "Emscripten")
    set(EMSCRIPTEN 1 BOOLEAN)
endif()

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Release" CACHE STRING "Choose the type of build, options are: Debug Release RelWithDebInfo MinSizeRel." FORCE)
    message("== Using default build configuration which is a Release!")
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

option(SSEMATH             "Allow GCC to use SSE floating point math" ${OPT_DEF_SSEMATH})
option(MMX                 "Use MMX assembly routines" ${OPT_DEF_ASM})
option(3DNOW               "Use 3Dnow! MMX assembly routines" ${OPT_DEF_ASM})
option(SSE                 "Use SSE assembly routines" ${OPT_DEF_ASM})
option(SSE2                "Use SSE2 assembly routines" ${OPT_DEF_SSEMATH})
option(SSE3                "Use SSE3 assembly routines" ${OPT_DEF_SSEMATH})

if(EMSCRIPTEN OR VITA)
    set(SDL_MIXER_X_SHARED_DEFAULT_ON OFF)
else()
    set(SDL_MIXER_X_SHARED_DEFAULT_ON ON)
endif()

# Settings
option(SDL_MIXER_X_STATIC   "Build static library of SDL Mixer X" ON)
option(SDL_MIXER_X_SHARED   "Build shared library of SDL Mixer X" ${SDL_MIXER_X_SHARED_DEFAULT_ON})

option(DOWNLOAD_AUDIO_CODECS_DEPENDENCY "Downloads and compiles AudioCodecs dependencies pack directly" OFF)
option(AUDIO_CODECS_BUILD_LOCAL_SDL2 "Build SDL2 from local AudioCodecs' copy" OFF)
set(AUDIO_CODECS_REPO_PATH "" CACHE PATH "Path to the AudioCodecs dependencies pack")
set(AUDIO_CODECS_INSTALL_PATH "" CACHE PATH "Path to installed AudioCodecs prefix")

option(SDL_MIXER_CLEAR_FOR_ZLIB_LICENSE   "Disable all GPL/LGPL dependencies to make binary being licensed with ZLib license" OFF)
option(SDL_MIXER_CLEAR_FOR_LGPL_LICENSE   "Disable all GPL dependencies to make binary being licensed with LGPL license" OFF)

if(SDL_MIXER_CLEAR_FOR_ZLIB_LICENSE AND SDL_MIXER_CLEAR_FOR_LGPL_LICENSE)
    message(FATAL_ERROR "You shouldn't use SDL_MIXER_CLEAR_FOR_ZLIB_LICENSE and SDL_MIXER_CLEAR_FOR_LGPL_LICENSE flags together!")
endif()

option(USE_SYSTEM_SDL2   "Use SDL2 from system" OFF)
if(VITA)
    message("Vita Build, setting SDL_MIXER_X_SHARED OFF")
    set(SDL_MIXER_X_SHARED OFF)

    message("Vita Build, setting USE_SYSTEM_SDL2 ON")
    set(USE_SYSTEM_SDL2 ON)

    message("Vita Build, setting GME & MIDI support off (for now)")
    set(USE_GME OFF)
    set(USE_MIDI OFF)
endif()

if(CMAKE_COMPILER_IS_GNUCC OR CMAKE_COMPILER_IS_GNUCXX OR CMAKE_C_COMPILER_ID STREQUAL "Clang")
    # Turn on warnings and legacy C/C++ standards to support more compilers
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra -Wno-long-long -Wno-missing-field-initializers -std=c89")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -pedantic -std=c++98")
    # Deny undefined symbols
    if(NOT APPLE)
        set(OLD_REQUIRED_FLAGS "${CMAKE_REQUIRED_FLAGS}")
        set(CMAKE_REQUIRED_FLAGS "-Wl,--no-undefined")
        check_c_compiler_flag("" HAVE_NO_UNDEFINED)
        set(CMAKE_REQUIRED_FLAGS "${OLD_REQUIRED_FLAGS}")
        if(HAVE_NO_UNDEFINED)
            set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -Wl,--no-undefined" )
            set(CMAKE_MODULE_LINKER_FLAGS "${CMAKE_MODULE_LINKER_FLAGS} -Wl,--no-undefined" )
        endif()
    else()
        set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -Wl,-undefined,error" )
        set(CMAKE_MODULE_LINKER_FLAGS "${CMAKE_MODULE_LINKER_FLAGS} -Wl,-undefined,error" )
    endif()
endif()

set(SDL_MIXER_DEFINITIONS)
set(SDL_MIXER_INCLUDE_PATHS)

if(NOT AUDIO_CODECS_REPO_PATH AND NOT AUDIO_CODECS_INSTALL_PATH)
    # Try to resolve sqlite dependency
    if(DOWNLOAD_AUDIO_CODECS_DEPENDENCY)
        # Download and configure AudioCodecs dependency
        set(SDL2_REPO_PATH ${CMAKE_INSTALL_PREFIX})
        set(SDL2_INCLUDE_PATH ${CMAKE_INSTALL_PREFIX}/include/SDL2)
        set(SDL2_LIBS_PATH ${CMAKE_INSTALL_PREFIX}/lib)
        set(BUILD_AUDIO_CODECS_NEEDED TRUE)
        message("== AudioCodecs will be built locally ==")
        include(cmake/DownloadAudioCodecs.cmake)
        set(AUDIO_CODECS_REPO_PATH ${AUDIO_CODECS_REPOSITORY_PATH})
        set(AUDIO_CODECS_INSTALL_PATH ${AUDIO_CODECS_INSTALL_DIR})
    endif()
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

if(AUDIO_CODECS_REPO_PATH OR AUDIO_CODECS_INSTALL_PATH AND NOT VITA)
    set(AUDIO_CODECS_PATH ${AUDIO_CODECS_REPO_PATH})
    set(AUDIO_CODECS_INSTALL_DIR ${AUDIO_CODECS_INSTALL_PATH})
    set(SDL2_REPO_PATH ${AUDIO_CODECS_INSTALL_DIR})
    set(SDL2_INCLUDE_PATH ${AUDIO_CODECS_INSTALL_DIR}/include/SDL2)
    link_directories(${AUDIO_CODECS_INSTALL_DIR}/lib)
    list(APPEND SDL_MIXER_INCLUDE_PATHS ${AUDIO_CODECS_INSTALL_DIR}/include ${SDL2_INCLUDE_PATH})
    message("AudioCodecs will be used. No needed extra libraries installed in the system.")
    set(USE_SYSTEM_AUDIO_LIBRARIES 0)
    set(CMAKE_FIND_ROOT_PATH ${AUDIO_CODECS_INSTALL_DIR})
    set(CMAKE_FIND_LIBRARY_SUFFIXES ${FIND_PREFER_STATIC})
else()
    message("AudioCodecs will NOT be used. Libraries will be recognized in the system.")
    set(USE_SYSTEM_AUDIO_LIBRARIES 1)
endif()

list(APPEND SDL_MIXER_DEFINITIONS
    -DHAVE_INTTYPES_H -DHAVE_SETENV -DHAVE_SINF
    -DPIC -D_REENTRANT -D_USE_MATH_DEFINES
)

if(UNIX AND NOT APPLE AND NOT HAIKU AND NOT EMSCRIPTEN)
    find_library(M_LIBRARY m)
    if(M_LIBRARY) # No need to link it by an absolute path
        set(M_LIBRARY m)
    endif()
    mark_as_advanced(M_LIBRARY)
else()
    set(M_LIBRARY "")
endif()

if(NOT MSVC)
    try_compile(STDCPP_FOUND
        ${CMAKE_BINARY_DIR}/compile_tests
        ${SDLMixerX_SOURCE_DIR}/cmake/tests/dummy.c
        LINK_LIBRARIES stdc++
    )
    try_compile(CPPABI_FOUND
        ${CMAKE_BINARY_DIR}/compile_tests
        ${SDLMixerX_SOURCE_DIR}/cmake/tests/dummy.c
        LINK_LIBRARIES c++abi
    )
    message("C++ Library: stdc++ [${STDCPP_FOUND}], c++abi [${CPPABI_FOUND}]")
    if(STDCPP_FOUND)
        set(STDCPP_LIBRARY stdc++)
    elseif(CPPABI_FOUND)
        set(STDCPP_LIBRARY c++abi)
    endif()
endif()

set(SDLMixerX_SOURCES)
set(SDLMixerX_LINK_LIBS)

include(music_wav)

include(music_ogg_vorbis)
include(music_opus)
include(music_flac)
include(music_ogg)

include(music_mad)
include(music_mpg123)

if(USE_MP3_MAD OR USE_MP3_MPG123 OR USE_WAV)
    list(APPEND SDLMixerX_SOURCES
        ${SDLMixerX_SOURCE_DIR}/src/codecs/mp3utils.c)
endif()

include(music_modplug)
include(music_mikmod)
include(music_xmp)
include(music_gme)

include(music_cmd)

option(USE_MIDI            "Build with MIDI support" ON)

if(VITA)
    set(USE_MIDI OFF)
    set(USE_GME OFF)
endif()

if(USE_MIDI)
    list(APPEND SDL_MIXER_DEFINITIONS -DMUSIC_USE_MIDI)

    include(music_midi_adl)
    include(music_midi_opn)
    include(music_timidity)
    include(music_fluidlite)
    include(music_fluidsynth)
    include(music_nativemidi_alt_win32)
    include(music_nativemidi)

    include(music_midi_seq)

    if(NOT USE_MIDI_ADLMIDI AND
       NOT USE_MIDI_OPNMIDI AND
       NOT USE_MIDI_TIMIDITY AND
       NOT USE_MIDI_FLUIDSYNTH AND
       NOT USE_MIDI_NATIVE)
        message_error("To have MIDI support you must enable at least one MIDI sequencer library")
    endif()
else()
    # Fallbacks to prevent undefined reference errors with giving of dummy calls
    list(APPEND SDLMixerX_SOURCES
        ${SDLMixerX_SOURCE_DIR}/src/codecs/music_midi_adl.c
        ${SDLMixerX_SOURCE_DIR}/src/codecs/music_midi_opn.c
    )
    message("== !!! MIDI SUPORT HAS BEEN DISABLED !!! ==")
endif()

include(libSDL2)

if(LIBMATH_NEEDED AND NOT MSVC AND M_LIBRARY)
    list(APPEND SDLMixerX_LINK_LIBS ${M_LIBRARY})
endif()

# It's the solid-built DLL of MixerX which is a mix of statically linked SDL2 and codec libraries
if(WIN32 AND NOT EMSCRIPTEN)
    option(BUILD_AS_VB6_BINDING "Compile library into solid module compatible with VisualBasic 6" OFF)
    if(BUILD_AS_VB6_BINDING)
        message("== SDL Mixer X will be built as solid module that is ready for MS-VB6 binding! ==")
        list(APPEND SDLMixerX_SOURCES ${SDLMixerX_SOURCE_DIR}/VB6_Wrapper/vb6_sdl_binds.c)
        list(REMOVE_ITEM SDLMixerX_LINK_LIBS "${LIBSDL2CUSTOM_LIB}")
        list(REMOVE_ITEM SDLMixerX_LINK_LIBS SDL2)
        list(REMOVE_ITEM SDLMixerX_LINK_LIBS SDL2d)
        list(APPEND SDLMixerX_LINK_LIBS
                    SDL2-static
                    uuid winmm ole32 imm32 version oleaut32 user32 gdi32 setupapi)
        if(NOT MSVC)
            list(APPEND SDLMixerX_LINK_LIBS stdc++ gcc pthread)
            unset(STDCPP_NEEDED) # stdc++ has been handled, no need to link it twice
        endif()
    endif()
endif()

if(VITA)
    message("VITA, APPENDING LIBS")
    find_library(SDL2_LIBRARIES SDL2)

    list(APPEND SDLMixerX_LINK_LIBS
        ${SDL2_LIBRARIES}
        debugnet
        SceRtc_stub
        SceNetCtl_stub
        SceNet_stub
        SceLibKernel_stub
        ScePvf_stub
        SceAppMgr_stub
        SceAppUtil_stub
        ScePgf_stub
        SceCommonDialog_stub
        m
        z
        SceGxm_stub
        SceDisplay_stub
        SceSysmodule_stub
        SceTouch_stub
        vitashark
    )

    message("\n\n${SDLMixerX_LINK_LIBS}\n\n")
endif()

if(APPLE)
    find_library(APPLE_CORE_AUDIO CoreAudio)
    find_library(APPLE_IOKIT IOKit)
    find_library(APPLE_COCOA Cocoa)
    find_library(APPLE_AUDIOTOOLBOX AudioToolbox)
    find_library(APPLE_CORE_VIDEO CoreVideo)
    find_library(APPLE_CORE_FOUNDATION CoreFoundation)
    find_library(APPLE_CARBON Carbon)
    find_library(APPLE_FORCE_FEEDBACK ForceFeedback)
    check_library_exists(iconv iconv_open "" HAVE_LIBICONV)
    if(HAVE_LIBICONV)
        set(ICONV_LIB iconv)
    endif()

    list(APPEND SDLMixerX_LINK_LIBS
            ${APPLE_CORE_AUDIO}
            ${APPLE_COCOA}
            ${APPLE_CORE_VIDEO}
            ${APPLE_CORE_FOUNDATION}
            ${APPLE_FORCE_FEEDBACK}
            ${APPLE_IOKIT}
            ${APPLE_CARBON}
            ${APPLE_AUDIOTOOLBOX}
            ${ICONV_LIB})
    list(APPEND SDLMixerX_LINK_LIBS objc)
endif()

if(ANDROID AND USE_XMP AND NOT USE_XMP_DYNAMIC)
    list(APPEND SDLMixerX_LINK_LIBS log)
endif()

# When statically linking some libraries, it's need to link  also
if(STDCPP_NEEDED AND NOT MSVC)
    message("Note: ${STDCPP_LIBRARY} will be linked!")
    list(APPEND SDLMixerX_LINK_LIBS ${STDCPP_LIBRARY})
endif()

list(APPEND SDLMixerX_SOURCES
    ${SDLMixerX_SOURCE_DIR}/src/effect_position.c
    ${SDLMixerX_SOURCE_DIR}/src/effects_internal.c
    ${SDLMixerX_SOURCE_DIR}/src/effect_stereoreverse.c
    ${SDLMixerX_SOURCE_DIR}/src/mixer.c
    ${SDLMixerX_SOURCE_DIR}/src/music.c
    ${SDLMixerX_SOURCE_DIR}/src/mixer_x_deprecated.c
    ${SDLMixerX_SOURCE_DIR}/src/utils.c
)

#file(GLOB SDLMixerX_SOURCES ${SDLMixerX_SOURCES})

set(SDLMixerX_INSTALLS)



if(SDL_MIXER_X_STATIC AND NOT BUILD_AS_VB6_BINDING)
    add_library(SDL2_mixer_ext_Static STATIC ${SDLMixerX_SOURCES})
    if(WIN32)
        set_target_properties(SDL2_mixer_ext_Static PROPERTIES OUTPUT_NAME SDL2_mixer_ext-static)
    else()
        set_target_properties(SDL2_mixer_ext_Static PROPERTIES OUTPUT_NAME SDL2_mixer_ext)
    endif()

    message("LINK LIBRARIES SDLMIXERX")
    if(VITA)
        # target_compile_options(SDL2_mixer_ext_Static PRIVATE --no-pie)
    endif()

    target_link_libraries(SDL2_mixer_ext_Static PUBLIC ${SDLMixerX_LINK_LIBS})
    target_compile_definitions(SDL2_mixer_ext_Static PRIVATE ${SDL_MIXER_DEFINITIONS})
    target_include_directories(SDL2_mixer_ext_Static PRIVATE
        ${SDLMixerX_SOURCE_DIR}/include
        ${SDLMixerX_SOURCE_DIR}/src
        ${SDLMixerX_SOURCE_DIR}/src/codecs
        ${AUDIO_CODECS_INSTALL_DIR}/include/SDL2
        ${SDL_MIXER_INCLUDE_PATHS}
    )
    target_include_directories(SDL2_mixer_ext_Static PUBLIC
        ${SDLMixerX_SOURCE_DIR}/include
    )
    if(BUILD_AUDIO_CODECS_NEEDED)
        add_dependencies(SDL2_mixer_ext_Static AudioCodecs)
    endif()
    if(NOT CMAKE_VERSION VERSION_LESS 3.0.2)
        set_target_properties(SDL2_mixer_ext_Static
            PROPERTIES
            ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib/$<$<CONFIG:Debug>:>"
            LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib/$<$<CONFIG:Debug>:>"
            RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/$<$<CONFIG:Debug>:>"
        )
    endif()
    if(DOWNLOAD_AUDIO_CODECS_DEPENDENCY)
        add_custom_command(
            TARGET SDL2_mixer_ext_Static POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy
                    ${CMAKE_SOURCE_DIR}/include/SDL_mixer.h
                    ${AUDIO_CODECS_INSTALL_DIR}/include/SDL2/SDL_mixer_ext.h)
    endif()
    list(APPEND SDLMixerX_INSTALLS SDL2_mixer_ext_Static)
endif()

if(WIN32)
    list(APPEND SDLMixerX_SOURCES ${SDLMixerX_SOURCE_DIR}/version.rc)
endif()

if(SDL_MIXER_X_SHARED AND NOT BUILD_AS_VB6_BINDING)
    add_library(SDL2_mixer_ext SHARED ${SDLMixerX_SOURCES})
    set_target_properties(SDL2_mixer_ext PROPERTIES
        VERSION ${SDLMixerX_VERSION}
        SOVERSION ${SDLMixerX_VERSION_MAJOR}
    )
    target_link_libraries(SDL2_mixer_ext PRIVATE ${SDLMixerX_LINK_LIBS})
    target_compile_definitions(SDL2_mixer_ext PRIVATE ${SDL_MIXER_DEFINITIONS} -DDLL_EXPORT)
    target_include_directories(SDL2_mixer_ext PRIVATE
        ${SDLMixerX_SOURCE_DIR}/include
        ${SDLMixerX_SOURCE_DIR}/src
        ${SDLMixerX_SOURCE_DIR}/src/codecs
        ${AUDIO_CODECS_INSTALL_DIR}/include/SDL2
        ${SDL_MIXER_INCLUDE_PATHS}
    )
    target_include_directories(SDL2_mixer_ext PUBLIC
        ${SDLMixerX_SOURCE_DIR}/include
    )
    if(BUILD_AUDIO_CODECS_NEEDED)
        add_dependencies(SDL2_mixer_ext AudioCodecs)
    endif()
    if(NOT CMAKE_VERSION VERSION_LESS 3.0.2)
        set_target_properties(SDL2_mixer_ext
            PROPERTIES
            ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib/$<$<CONFIG:Debug>:>"
            LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib/$<$<CONFIG:Debug>:>"
            RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/$<$<CONFIG:Debug>:>"
        )
    endif()
    if(DOWNLOAD_AUDIO_CODECS_DEPENDENCY)
        add_custom_command(
            TARGET SDL2_mixer_ext POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy
                    ${CMAKE_SOURCE_DIR}/include/SDL_mixer.h
                    ${AUDIO_CODECS_INSTALL_DIR}/include/SDL2/SDL_mixer_ext.h)
    endif()
    list(APPEND SDLMixerX_INSTALLS SDL2_mixer_ext)
endif()

if(BUILD_AS_VB6_BINDING)
    add_library(SDL2_mixer_ext_VB6 MODULE ${SDLMixerX_SOURCES})
    set_target_properties(SDL2_mixer_ext_VB6 PROPERTIES OUTPUT_NAME SDL2MixerVB)
    set_target_properties(SDL2_mixer_ext_VB6 PROPERTIES PREFIX "")
    target_compile_definitions(SDL2_mixer_ext_VB6 PRIVATE ${SDL_MIXER_DEFINITIONS} -DFORCE_STDCALLS -DDLL_EXPORT)
    target_link_libraries(SDL2_mixer_ext_VB6 ${SDLMixerX_LINK_LIBS})
    target_include_directories(SDL2_mixer_ext_VB6 PUBLIC
        ${SDLMixerX_SOURCE_DIR}/include
        ${SDLMixerX_SOURCE_DIR}/src
        ${SDLMixerX_SOURCE_DIR}/src/codecs
        ${AUDIO_CODECS_INSTALL_DIR}/include/SDL2
    )
    target_include_directories(SDL2_mixer_ext_VB6 PUBLIC
        ${SDLMixerX_SOURCE_DIR}/include
        ${SDL_MIXER_INCLUDE_PATHS}
    )
    set_target_properties(SDL2_mixer_ext_VB6 PROPERTIES LINK_FLAGS
        "-Wl,--add-stdcall-alias -static-libgcc -static-libstdc++ -static -lpthread")
    if(BUILD_AUDIO_CODECS_NEEDED)
        add_dependencies(SDL2_mixer_ext_VB6 AudioCodecs)
    endif()
    if(NOT CMAKE_VERSION VERSION_LESS 3.0.2)
        set_target_properties(SDL2_mixer_ext_VB6
            PROPERTIES
            ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/sdl-mixer-vb6/$<$<CONFIG:Debug>:>"
            LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/sdl-mixer-vb6/$<$<CONFIG:Debug>:>"
            RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/sdl-mixer-vb6/$<$<CONFIG:Debug>:>"
        )
    endif()
    add_custom_command(
        TARGET SDL2_mixer_ext_VB6 POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
                ${CMAKE_SOURCE_DIR}/VB6_Wrapper/modSDL2_mixer_ext_vb6.bas
                ${CMAKE_BINARY_DIR}/sdl-mixer-vb6)
    list(APPEND SDLMixerX_INSTALLS SDL2_mixer_ext_VB6)
endif()

option(BUILD_EXAMPLES "Build examples" OFF)
if(BUILD_EXAMPLES AND NOT BUILD_AS_VB6_BINDING)
    add_subdirectory(examples)
endif()

install(TARGETS ${SDLMixerX_INSTALLS}
    RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}"
    LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}"
    ARCHIVE DESTINATION "${CMAKE_INSTALL_LIBDIR}"
    INCLUDES DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}"
    FRAMEWORK DESTINATION "${CMAKE_INSTALL_LIBDIR}"
)

if(DOWNLOAD_AUDIO_CODECS_DEPENDENCY)
    install(DIRECTORY ${AUDIO_CODECS_INSTALL_DIR}/bin
            DESTINATION .)
    install(DIRECTORY ${AUDIO_CODECS_INSTALL_DIR}/lib
            DESTINATION .)
    install(DIRECTORY ${AUDIO_CODECS_INSTALL_DIR}/include
            DESTINATION .)
endif()

install(FILES
    include/SDL_mixer.h
    DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/SDL2"
    RENAME SDL_mixer_ext.h
)

if(NOT ANDROID)
    set(prefix ${CMAKE_INSTALL_PREFIX})
    set(exec_prefix "\${prefix}")
    set(libdir "\${exec_prefix}/lib${LIB_SUFFIX}")
    set(bindir "\${exec_prefix}/bin")
    set(includedir "\${prefix}/include")
    set(VERSION ${SDLMixerX_VERSION})

    configure_file("${SDLMixerX_SOURCE_DIR}/SDL2_mixer_ext.pc.in"
        "${SDLMixerX_BINARY_DIR}/SDL2_mixer_ext.pc" @ONLY)
    install(FILES "${SDLMixerX_BINARY_DIR}/SDL2_mixer_ext.pc"
            DESTINATION "${CMAKE_INSTALL_LIBDIR}/pkgconfig")
endif()

# === Unit tests ====
option(WITH_UNIT_TESTS "Enable unit testing" OFF)
if(WITH_UNIT_TESTS)
    enable_testing()
    add_subdirectory(test)
endif()

function(print_sumary _libName _isEnabled _wasFound _whatFound)
    if(${_isEnabled})
        if(${_wasFound})
            message("  ${_libName} found:")
            message("  -- ${${_whatFound}}")
        else()
            message("  ${_libName} is missing, will be disabled")
            message("  -- <missing>")
        endif()
    else()
        message("  ${_libName} is disabled")
        message("  -- <disabled>")
    endif()
endfunction()

message("==== SDL Mixer X modules ====")
if(LIBOGG_NEEDED)
    print_sumary("OGG" LIBOGG_NEEDED OGG_FOUND OGG_LIBRARIES)
endif()
if(USE_OGG_VORBIS_TREMOR)
    print_sumary("Tremor" USE_OGG_VORBIS_TREMOR Tremor_FOUND Tremor_LIBRARIES)
else()
    print_sumary("Vorbis" USE_OGG_VORBIS Vorbis_FOUND Vorbis_LIBRARIES)
endif()
print_sumary("OPUS" USE_OPUS Opus_FOUND Opus_LIBRARIES)
print_sumary("FLAC" USE_FLAC FLAC_FOUND FLAC_LIBRARIES)
print_sumary("libMAD" USE_MP3_MAD MAD_FOUND MAD_LIBRARIES)
print_sumary("MPG123" USE_MP3_MPG123 MPG123_FOUND MPG123_LIBRARIES)
print_sumary("ModPlug" USE_MODPLUG ModPlug_FOUND ModPlug_LIBRARIES)
print_sumary("MikMod" USE_MIKMOD MikMod_FOUND MikMod_LIBRARIES)
print_sumary("libXMP" USE_XMP XMP_FOUND XMP_LIBRARIES)
print_sumary("GME" USE_GME GME_FOUND GME_LIBRARIES)
print_sumary("libADLMIDI" USE_MIDI_ADLMIDI ADLMIDI_FOUND ADLMIDI_LIBRARIES)
print_sumary("libOPNMIDI" USE_MIDI_OPNMIDI OPNMIDI_FOUND OPNMIDI_LIBRARIES)
print_sumary("FluidSynth" USE_MIDI_FLUIDSYNTH FluidSynth_FOUND FluidSynth_LIBRARIES)
print_sumary("FluidLite" USE_MIDI_FLUIDLITE FluidLite_FOUND FluidLite_LIBRARIES)
print_sumary("Timidity-SDL" USE_MIDI_TIMIDITY TIMIDITYSDL_FOUND TIMIDITYSDL_LIBRARIES)
printLicense()
