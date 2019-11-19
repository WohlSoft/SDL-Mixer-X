cmake_minimum_required (VERSION 2.8.11)
project(SDLMixerX C)

include(FindPkgConfig)
include(CheckIncludeFiles)
include(CheckFunctionExists)
include(CheckLibraryExists)
#include(CheckCSourceRuns)
#include(CheckCCompilerFlag)

set(CMAKE_MODULE_PATH "${SDLMixerX_SOURCE_DIR}/cmake;${SDLMixerX_SOURCE_DIR}/cmake/find;${SDLMixerX_SOURCE_DIR}/src/codecs;${CMAKE_MODULE_PATH}")

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

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    set(MIX_DEBUG_SUFFIX "d")
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

if(EMSCRIPTEN)
    set(SDL_MIXER_X_SHARED_DEFAULT_ON OFF)
else()
    set(SDL_MIXER_X_SHARED_DEFAULT_ON ON)
endif()

# Settings
option(SDL_MIXER_X_STATIC   "Build static library of SDL Mixer X" ON)
option(SDL_MIXER_X_SHARED   "Build shared library of SDL Mixer X" ${SDL_MIXER_X_SHARED_DEFAULT_ON})

option(DOWNLOAD_AUDIO_CODECS_DEPENDENCY "Downloads and compiles AudioCodecs dependencies pack directly" OFF)
set(AUDIO_CODECS_REPO_PATH "" CACHE PATH "Path to the AudioCodecs dependencies pack")
set(AUDIO_CODECS_INSTALL_PATH "" CACHE PATH "Path to installed AudioCodecs prefix")

option(USE_SYSTEM_SDL2   "Use SDL2 from system" OFF)

if(CMAKE_COMPILER_IS_GNUCC OR CMAKE_COMPILER_IS_GNUCXX)
    # Turn on warnings and legacy C/C++ standards to support more compilers
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -std=c90")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -pedantic -std=c++98")
    # Deny undefined symbols
    if(NOT APPLE)
        set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -Wl,--no-undefined" )
        set(CMAKE_MODULE_LINKER_FLAGS "${CMAKE_MODULE_LINKER_FLAGS} -Wl,--no-undefined" )
    endif()
endif()

set(SDL_MIXER_INCLUDE_PATHS)

if(NOT AUDIO_CODECS_REPO_PATH AND NOT AUDIO_CODECS_INSTALL_PATH)
    # Try to resolve sqlite dependency
    if(DOWNLOAD_AUDIO_CODECS_DEPENDENCY)
        # Download and configure AudioCodecs dependency
        set(SDL2_REPO_PATH ${CMAKE_INSTALL_PREFIX})
        set(SDL2_INCLUDE_PATH ${CMAKE_INSTALL_PREFIX}/include/SDL2)
        set(SDL2_LIBS_PATH ${CMAKE_INSTALL_PREFIX}/lib)
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

if(AUDIO_CODECS_REPO_PATH OR AUDIO_CODECS_INSTALL_PATH)
    set(AUDIO_CODECS_PATH ${AUDIO_CODECS_REPO_PATH})
    set(AUDIO_CODECS_INSTALL_DIR ${AUDIO_CODECS_INSTALL_PATH})
    set(SDL2_REPO_PATH ${AUDIO_CODECS_INSTALL_DIR})
    set(SDL2_INCLUDE_PATH ${AUDIO_CODECS_INSTALL_DIR}/include/SDL2)
    link_directories(${AUDIO_CODECS_INSTALL_DIR}/lib)
    list(APPEND SDL_MIXER_INCLUDE_PATHS ${AUDIO_CODECS_INSTALL_DIR}/include ${SDL2_INCLUDE_PATH})
    message("AudioCodecs will be used. No needed extra libraries installed in the system.")
    set(USE_SYSTEM_AUDIO_LIBRARIES 0)
    set(CMAKE_FIND_LIBRARY_SUFFIXES ${FIND_PREFER_STATIC})
else()
    message("AudioCodecs will NOT be used. Libraries will be recognized in the system.")
    set(USE_SYSTEM_AUDIO_LIBRARIES 1)
endif()

add_definitions(-DHAVE_INTTYPES_H -DHAVE_SETENV -DHAVE_SINF)
add_definitions(-Dmain=SDL_main -DPIC -D_REENTRANT -D_USE_MATH_DEFINES)

set(SDLMixerX_SOURCES)
set(SDLMixerX_LINK_LIBS)

include(music_wav)

include(music_ogg_vorbis)
include(music_opus)
include(music_flac)
include(music_ogg)

include(music_mad)
include(music_mpg123)
include(music_id3tag)

if(USE_MP3_MAD OR USE_MP3_MPG123 OR USE_WAV)
    list(APPEND SDLMixerX_SOURCES
        ${SDLMixerX_SOURCE_DIR}/src/codecs/music_id3tag.c)
endif()

include(music_modplug)
include(music_mikmod)
include(music_xmp)
include(music_gme)

include(music_cmd)

option(USE_MIDI            "Build with MIDI support" ON)
if(USE_MIDI)
    add_definitions(-DMUSIC_USE_MIDI)

    include(music_midi_adl)
    include(music_midi_opn)
    include(music_timidity)
    include(music_fluidsynth)
    include(music_nativemidi)

    if(NOT USE_MIDI_ADLMIDI AND
       NOT USE_MIDI_OPNMIDI AND
       NOT USE_MIDI_TIMIDITY AND
       NOT USE_MIDI_FLUIDSYNTH AND
       NOT USE_MIDI_NATIVE)
        message_error("To have MIDI support you must enable at least one MIDI sequencer library")
    endif()
endif()

# Keep those files in build list even those libraries are not used
# (they are contains public calls are dummied when referred libraries are disabled from the use)
list(APPEND SDLMixerX_SOURCES
    ${SDLMixerX_SOURCE_DIR}/src/codecs/music_midi_adl.c
    ${SDLMixerX_SOURCE_DIR}/src/codecs/music_midi_opn.c
    ${SDLMixerX_SOURCE_DIR}/src/codecs/music_timidity.c
)

include(libSDL2)

if(LIBMATH_NEEDED AND NOT MSVC)
    list(APPEND SDLMixerX_LINK_LIBS m)
endif()

if(WIN32 AND NOT EMSCRIPTEN)
    option(BUILD_AS_VB6_BINDING "Compile library into solid module compatible with VisualBasic 6" OFF)
    if(BUILD_AS_VB6_BINDING)
        message("== SDL Mixer X will be built as solid module that is ready for MS-VB6 binding! ==")
        list(APPEND SDLMixerX_SOURCES ${SDLMixerX_SOURCE_DIR}/src/vb6_binding/vb6_sdl_binds.c)
        list(REMOVE_ITEM SDLMixerX_LINK_LIBS SDL2)
        list(REMOVE_ITEM SDLMixerX_LINK_LIBS SDL2$<$<CONFIG:Debug>:d>)
        list(APPEND SDLMixerX_LINK_LIBS
                    SDL2-static
                    uuid winmm ole32 imm32 version oleaut32 user32 gdi32 setupapi)
        if(NOT MSVC)
            list(APPEND SDLMixerX_LINK_LIBS stdc++ gcc pthread)
        endif()
    endif()
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
    find_library(ICONV_LIB iconv)
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

if(NOT MSVC)
    list(APPEND SDLMixerX_LINK_LIBS stdc++)
endif()

list(APPEND SDLMixerX_SOURCES
        ${SDLMixerX_SOURCE_DIR}/src/effect_position.c
        ${SDLMixerX_SOURCE_DIR}/src/effects_internal.c
        ${SDLMixerX_SOURCE_DIR}/src/effect_stereoreverse.c
        ${SDLMixerX_SOURCE_DIR}/src/mixer.c
        ${SDLMixerX_SOURCE_DIR}/src/music.c
        ${SDLMixerX_SOURCE_DIR}/src/mixer_x_deprecated.c
    )

file(GLOB SDLMixerX_SOURCES ${SDLMixerX_SOURCES})

set(SDLMixerX_INSTALLS)

if(SDL_MIXER_X_STATIC AND NOT BUILD_AS_VB6_BINDING)
    add_library(SDL2_mixer_ext_Static STATIC ${SDLMixerX_SOURCES})
    if(WIN32)
        set_target_properties(SDL2_mixer_ext_Static PROPERTIES OUTPUT_NAME SDL2_mixer_ext-static)
    else()
        set_target_properties(SDL2_mixer_ext_Static PROPERTIES OUTPUT_NAME SDL2_mixer_ext)
    endif()
    target_include_directories(SDL2_mixer_ext_Static PRIVATE
        ${SDLMixerX_SOURCE_DIR}/include/SDL_mixer_ext
        ${SDLMixerX_SOURCE_DIR}/src
        ${SDLMixerX_SOURCE_DIR}/src/codecs
        ${LIBSDL2CUSTOM_INCLUDES}
    )
    target_include_directories(SDL2_mixer_ext_Static PUBLIC
        ${SDLMixerX_SOURCE_DIR}/include
        ${SDL_MIXER_INCLUDE_PATHS}
    )
    if(DOWNLOAD_AUDIO_CODECS_DEPENDENCY AND AUDIO_CODECS_INSTALL_PATH)
        add_dependencies(SDL2_mixer_ext_Static AudioCodecs)
    endif()
    set_target_properties(SDL2_mixer_ext_Static
        PROPERTIES
        ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib/$<$<CONFIG:Debug>:>"
        LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib/$<$<CONFIG:Debug>:>"
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/$<$<CONFIG:Debug>:>"
    )
    if(DOWNLOAD_AUDIO_CODECS_DEPENDENCY)
        add_custom_command(
            TARGET SDL2_mixer_ext_Static POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy
                    ${CMAKE_SOURCE_DIR}/include/SDL_mixer_ext/SDL_mixer_ext.h
                    ${AUDIO_CODECS_INSTALL_DIR}/include/SDL2)
    endif()
    list(APPEND SDLMixerX_INSTALLS SDL2_mixer_ext_Static)
endif()

if(WIN32)
    list(APPEND SDLMixerX_SOURCES ${SDLMixerX_SOURCE_DIR}/version.rc)
endif()

if(SDL_MIXER_X_SHARED AND NOT BUILD_AS_VB6_BINDING)
    add_library(SDL2_mixer_ext SHARED ${SDLMixerX_SOURCES})
    target_link_libraries(SDL2_mixer_ext ${SDLMixerX_LINK_LIBS})
    target_include_directories(SDL2_mixer_ext PRIVATE
        ${SDLMixerX_SOURCE_DIR}/include/SDL_mixer_ext
        ${SDLMixerX_SOURCE_DIR}/src
        ${SDLMixerX_SOURCE_DIR}/src/codecs
        ${AUDIO_CODECS_INSTALL_DIR}/include/SDL2
    )
    target_include_directories(SDL2_mixer_ext PUBLIC
        ${SDLMixerX_SOURCE_DIR}/include
        ${SDL_MIXER_INCLUDE_PATHS}
    )
    if(DOWNLOAD_AUDIO_CODECS_DEPENDENCY AND AUDIO_CODECS_INSTALL_PATH)
        add_dependencies(SDL2_mixer_ext AudioCodecs)
    endif()
    set_target_properties(SDL2_mixer_ext
        PROPERTIES
        ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib/$<$<CONFIG:Debug>:>"
        LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib/$<$<CONFIG:Debug>:>"
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/$<$<CONFIG:Debug>:>"
    )
    if(DOWNLOAD_AUDIO_CODECS_DEPENDENCY)
        add_custom_command(
            TARGET SDL2_mixer_ext POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy
                    ${CMAKE_SOURCE_DIR}/include/SDL_mixer_ext/SDL_mixer_ext.h
                    ${AUDIO_CODECS_INSTALL_DIR}/include/SDL2)
    endif()
    list(APPEND SDLMixerX_INSTALLS SDL2_mixer_ext)
endif()

if(BUILD_AS_VB6_BINDING)
    add_library(SDL2_mixer_ext_VB6 MODULE ${SDLMixerX_SOURCES})
    set_target_properties(SDL2_mixer_ext_VB6 PROPERTIES OUTPUT_NAME SDL2MixerVB)
    set_target_properties(SDL2_mixer_ext_VB6 PROPERTIES PREFIX "")
    target_compile_definitions(SDL2_mixer_ext_VB6 PRIVATE -DFORCE_STDCALLS)
    target_link_libraries(SDL2_mixer_ext_VB6 ${SDLMixerX_LINK_LIBS})
    target_include_directories(SDL2_mixer_ext_VB6 PUBLIC
        ${SDLMixerX_SOURCE_DIR}/include/SDL_mixer_ext
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
    if(DOWNLOAD_AUDIO_CODECS_DEPENDENCY AND AUDIO_CODECS_INSTALL_PATH)
        add_dependencies(SDL2_mixer_ext_VB6 AudioCodecs)
    endif()
    set_target_properties(SDL2_mixer_ext_VB6
        PROPERTIES
        ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/sdl-mixer-vb6/$<$<CONFIG:Debug>:>"
        LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/sdl-mixer-vb6/$<$<CONFIG:Debug>:>"
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/sdl-mixer-vb6/$<$<CONFIG:Debug>:>"
    )
    add_custom_command(
        TARGET SDL2_mixer_ext_VB6 POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
                ${CMAKE_SOURCE_DIR}/VB6_Wrapper/modSDL2_mixer_ext_vb6.bas
                ${CMAKE_BINARY_DIR}/sdl-mixer-vb6)
    list(APPEND SDLMixerX_INSTALLS SDL2_mixer_ext_VB6)
endif()

install(TARGETS ${SDLMixerX_INSTALLS}
        RUNTIME DESTINATION "bin"
        LIBRARY DESTINATION "lib"
        ARCHIVE DESTINATION "lib"
        FRAMEWORK DESTINATION "lib"
        INCLUDES DESTINATION "include")

if(DOWNLOAD_AUDIO_CODECS_DEPENDENCY)
    install(DIRECTORY ${AUDIO_CODECS_INSTALL_DIR}/bin
            DESTINATION .)
    install(DIRECTORY ${AUDIO_CODECS_INSTALL_DIR}/lib
            DESTINATION .)
    install(DIRECTORY ${AUDIO_CODECS_INSTALL_DIR}/include
            DESTINATION .)
endif()

install(FILES
        include/SDL_mixer_ext/begin_code.h
        include/SDL_mixer_ext/close_code.h
        include/SDL_mixer_ext/SDL_mixer_ext.h
        DESTINATION include/SDL2)

if(USE_SYSTEM_AUDIO_LIBRARIES)
    message("==== SDL Mixer X system-wide libraries ====")
    message("  OGG found: ${OGG_FOUND}")
    message("  Vorbis found: ${Vorbis_FOUND}")
    message("  OPUS found: ${Opus_FOUND}")
    message("  FLAC found: ${FOUND_FLAC}")
    message("  libMAD found: ${FOUND_MAD}")
    message("  MPG123 found: ${MPG123_FOUND}")
    message("  ModPlug found: ${ModPlug_FOUND}")
    message("  MikMod found: ${FOUND_MIKMOD}")
    message("  GME found: ${FOUND_GME}")
    message("  libADLMIDI found: ${ADLMIDI_FOUND}")
    message("  libOPNMIDI found: ${OPNMIDI_FOUND}")
    message("  FluidSynth found: ${FOUND_FLUIDSYNTH}")
endif()
