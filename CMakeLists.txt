cmake_minimum_required (VERSION 2.8.11)
project(SDLMixerX C)

include(CheckIncludeFiles)
include(CheckFunctionExists)
#include(CheckCSourceRuns)
#include(CheckCCompilerFlag)
#include(CheckLibraryExists)

if(NOT CMAKE_VERSION VERSION_LESS 2.8.12)
    set(CMAKE_MACOSX_RPATH 0)
endif()

option(SSEMATH             "Allow GCC to use SSE floating point math" ${OPT_DEF_SSEMATH})
option(MMX                 "Use MMX assembly routines" ${OPT_DEF_ASM})
option(3DNOW               "Use 3Dnow! MMX assembly routines" ${OPT_DEF_ASM})
option(SSE                 "Use SSE assembly routines" ${OPT_DEF_ASM})
option(SSE2                "Use SSE2 assembly routines" ${OPT_DEF_SSEMATH})
option(SSE3                "Use SSE3 assembly routines" ${OPT_DEF_SSEMATH})

# Settings
option(SDL_MIXER_X_STATIC   "Build static library of SDL Mixer X" ON)
option(SDL_MIXER_X_SHARED   "Build shared library of SDL Mixer X" ON)

option(DOWNLOAD_AUDIO_CODECS_DEPENDENCY "Downloads and compiles AudioCodecs dependencies pack directly" OFF)
set(AUDIO_CODECS_REPO_PATH "" CACHE PATH "Path to the AudioCodecs dependencies pack")

if(CMAKE_COMPILER_IS_GNUCC OR CMAKE_COMPILER_IS_GNUCXX)
    # Turn on warnings and legacy C/C++ standards to support more compilers
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -std=c90")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -pedantic -std=c++98")
endif()

if(NOT AUDIO_CODECS_REPO_PATH)
    # Try to resolve sqlite dependency
    if(DOWNLOAD_AUDIO_CODECS_DEPENDENCY)
        # Download and configure AudioCodecs dependency
        set(SDL2_REPO_PATH ${CMAKE_INSTALL_PREFIX})
        include(cmake/DownloadAudioCodecs.cmake)
        set(AUDIO_CODECS_REPO_PATH ${AUDIO_CODECS_INSTALL_DIR})
    endif()
endif()

if(AUDIO_CODECS_REPO_PATH)
    set(AUDIO_CODECS_PATH ${AUDIO_CODECS_REPO_PATH})
    set(AUDIO_CODECS_INSTALL_DIR ${AUDIO_CODECS_REPO_PATH})
    set(SDL2_REPO_PATH ${AUDIO_CODECS_REPO_PATH})
    link_directories(${AUDIO_CODECS_INSTALL_DIR}/lib)
    include_directories(${AUDIO_CODECS_INSTALL_DIR}/include)
else()
    message(FATAL_ERROR "AudioCodecs are not found! ")
endif()

add_definitions(-DHAVE_INTTYPES_H -DHAVE_SETENV -DHAVE_SINF)
add_definitions(-Dmain=SDL_main -DPIC -D_REENTRANT -D_USE_MATH_DEFINES)

set(SDLMixerX_SOURCES)
set(SDLMixerX_LINK_LIBS)

option(USE_WAV             "Build with WAV codec" ON)
if(USE_WAV)
    add_definitions(-DMUSIC_WAV)
    list(APPEND SDLMixerX_SOURCES
        ${SDLMixerX_SOURCE_DIR}/src/codecs/load_aiff.c
        ${SDLMixerX_SOURCE_DIR}/src/codecs/load_voc.c
        ${SDLMixerX_SOURCE_DIR}/src/codecs/music_wav.c)
endif()

option(USE_OGG_VORBIS      "Build with OGG Vorbis codec" ON)
if(USE_OGG_VORBIS)
    add_definitions(-DMUSIC_OGG)
    include_directories(${AUDIO_CODECS_PATH}/libogg/include)
    include_directories(${AUDIO_CODECS_PATH}/libvorbis/include)
    list(APPEND SDLMixerX_LINK_LIBS vorbisfile vorbis)
    set(LIBOGG_NEEDED ON)
    list(APPEND SDLMixerX_SOURCES
        ${SDLMixerX_SOURCE_DIR}/src/codecs/music_ogg.c)
endif()

option(USE_FLAC            "Build with FLAC codec" ON)
if(USE_FLAC)
    add_definitions(-DMUSIC_FLAC -DFLAC__NO_DLL)
    include_directories(${AUDIO_CODECS_PATH}/libogg/include)
    include_directories(${AUDIO_CODECS_PATH}/libFLAC/include)
    set(LIBOGG_NEEDED ON)
    list(APPEND SDLMixerX_LINK_LIBS FLAC)
    list(APPEND SDLMixerX_SOURCES
        ${SDLMixerX_SOURCE_DIR}/src/codecs/music_flac.c)
endif()

if(LIBOGG_NEEDED)
    list(APPEND SDLMixerX_LINK_LIBS ogg)
endif()

option(USE_MP3_ID3TAG   "Build with MP3 Meta tags support provided by libID3Tag library" ON)
if(USE_MP3_ID3TAG)
    add_definitions(-DMUSIC_MP3_ID3TAG)
    include_directories(${AUDIO_CODECS_PATH}/libid3tag/include)
    list(APPEND SDLMixerX_LINK_LIBS id3tag)
endif()

option(USE_MP3_MAD      "Build with MAD MP3 codec" ON)
option(USE_MP3_MAD_GPL_DITHERING "Enable GPL-Licensed dithering functions for MAD library" ON)
if(USE_MP3_MAD)
    add_definitions(-DMUSIC_MP3_MAD)
    if(USE_MP3_MAD_GPL_DITHERING)
        add_definitions(-DMUSIC_MP3_MAD_GPL_DITHERING)
    endif()
    list(APPEND SDLMixerX_LINK_LIBS mad)
    include_directories(${AUDIO_CODECS_PATH}/libmad/include)
    list(APPEND SDLMixerX_SOURCES
        ${SDLMixerX_SOURCE_DIR}/src/codecs/music_mad.c)
endif()

option(USE_MP3_SMPEG       "Build with SMPEG MP3 codec" OFF)
if(USE_MP3_SMPEG)
    add_definitions(-DMUSIC_MP3_SMPEG)
    list(APPEND SDLMixerX_LINK_LIBS smpeg)
    include_directories(${AUDIO_CODECS_PATH}/smpeg/include)
    list(APPEND SDLMixerX_SOURCES
        ${SDLMixerX_SOURCE_DIR}/src/codecs/music_smpeg.c)
endif()

option(USE_MP3_MPG123     "[WIP, DON'T USE IT] Build with MPG123 MP3 codec" OFF)
if(USE_MP3_MPG123)
    add_definitions(-DMUSIC_MP3_MPG123)

    message(WARNING "MPG123 SUPPORT IS WIP, DON'T USE IT IN PRODUCTION!!!")
    # ======= Until AudioCodecs will receive buildable mpg123, detect it externally =======
    include(cmake/FindMpg123.cmake)

    if(NOT MPG123_FOUND)
        message(FATAL_ERROR "MPG123 is not found!")
    else()
        message("MPG123 found in ${MPG123_INCLUDE_DIR} folder")
    endif()

    include_directories(${MPG123_INCLUDE_DIRS})
    set(LIBS ${LIBS} ${MPG123_LIBRARIES})
    list(APPEND SDLMixerX_LINK_LIBS mpg123)
    list(APPEND SDLMixerX_SOURCES
        ${SDLMixerX_SOURCE_DIR}/src/codecs/music_mpg123.c)
endif()

if(USE_MP3_MAD OR USE_MP3_MPG123 OR USE_MP3_SMPEG)
    list(APPEND SDLMixerX_SOURCES
        ${SDLMixerX_SOURCE_DIR}/src/codecs/music_id3tag.c)
endif()

option(USE_MODPLUG         "Build with ModPlug library" ON)
if(USE_MODPLUG)
    add_definitions(-DMUSIC_MOD_MODPLUG -DMODPLUG_STATIC)
    include_directories(${AUDIO_CODECS_PATH}/libmodplug/include)
    list(APPEND SDLMixerX_LINK_LIBS modplug)
    list(APPEND SDLMixerX_SOURCES
        ${SDLMixerX_SOURCE_DIR}/src/codecs/music_modplug.c)
endif()

option(USE_MIKMOD          "Build with MikMod library" OFF)
if(USE_MIKMOD)
    add_definitions(-DMUSIC_MOD_MIKMOD)
    include_directories(${AUDIO_CODECS_PATH}/libmikmod/include)
    list(APPEND SDLMixerX_LINK_LIBS mikmod)
    list(APPEND SDLMixerX_SOURCES
        ${SDLMixerX_SOURCE_DIR}/src/codecs/music_mikmod.c)
endif()

option(USE_GME             "Build with Game Music Emulators library" ON)
if(USE_GME)
    add_definitions(-DMUSIC_GME)
    include_directories(${AUDIO_CODECS_PATH}/libgme/include)
    include_directories(${AUDIO_CODECS_PATH}/zlib/include)
    list(APPEND SDLMixerX_LINK_LIBS gme zlib)
    list(APPEND SDLMixerX_SOURCES
        ${SDLMixerX_SOURCE_DIR}/src/codecs/music_gme.c)
endif()

if(NOT WIN32) # CMD Music is not supported on Windows
    option(USE_CMD             "Build with CMD music player support" ON)
    if(USE_CMD)
        add_definitions(-DMUSIC_CMD -D_POSIX_C_SOURCE=1)
        CHECK_FUNCTION_EXISTS(fork HAVE_FORK)
        if(HAVE_FORK)
            add_definitions(-DHAVE_FORK)
        endif()
        list(APPEND SDLMixerX_SOURCES
            ${SDLMixerX_SOURCE_DIR}/src/codecs/music_cmd.c)
    endif()
endif()

option(USE_MIDI            "Build with MIDI support" ON)
if(USE_MIDI)

    option(USE_MIDI_ADLMIDI    "Build with libADLMIDI OPL3 Emulator based MIDI sequencer support" ON)
    if(USE_MIDI_ADLMIDI)
        add_definitions(-DMUSIC_MID_ADLMIDI)
        include_directories(${AUDIO_CODECS_PATH}/libADLMIDI/include)
        list(APPEND SDLMixerX_LINK_LIBS ADLMIDI)
        list(APPEND SDLMixerX_SOURCES
            ${SDLMixerX_SOURCE_DIR}/src/codecs/music_midi_adl.c)
    endif()

    option(USE_MIDI_OPNMIDI    "Build with libOPNMIDI OPN2 Emulator based MIDI sequencer support" ON)
    if(USE_MIDI_OPNMIDI)
        add_definitions(-DMUSIC_MID_OPNMIDI)
        include_directories(${AUDIO_CODECS_PATH}/libOPNMIDI/include)
        list(APPEND SDLMixerX_LINK_LIBS OPNMIDI)
        list(APPEND SDLMixerX_SOURCES
            ${SDLMixerX_SOURCE_DIR}/src/codecs/music_midi_opn.c)
    endif()

    option(USE_MIDI_TIMIDITY   "Build with Timidity wave table MIDI sequencer support" ON)
    if(USE_MIDI_TIMIDITY)
        add_definitions(-DMUSIC_MID_TIMIDITY)
        include_directories(${AUDIO_CODECS_PATH}/libtimidity/include)
        list(APPEND SDLMixerX_LINK_LIBS timidity)
        list(APPEND SDLMixerX_SOURCES
            ${SDLMixerX_SOURCE_DIR}/src/codecs/music_timidity.c)
    endif()

    option(USE_MIDI_FLUIDSYNTH "Build with FluidSynth wave table MIDI sequencer support" OFF)
    if(USE_MIDI_FLUIDSYNTH)
        add_definitions(-DMUSIC_MID_FLUIDSYNTH)
        include_directories(${AUDIO_CODECS_PATH}/FluidLite/include)
        list(APPEND SDLMixerX_LINK_LIBS fluidlite)
        list(APPEND SDLMixerX_SOURCES
            ${SDLMixerX_SOURCE_DIR}/src/codecs/music_fluidsynth.c)
    endif()

     # Native MIDI correctly works on Windows and macOS only.
    if(WIN32 OR APPLE)
        set(NATIVE_MIDI_SUPPORTED ON)
    else()
        set(NATIVE_MIDI_SUPPORTED OFF)
    endif()

    option(USE_MIDI_NATIVE     "Build with operating system native MIDI output support" ${NATIVE_MIDI_SUPPORTED})
    if(USE_MIDI_NATIVE)
        add_definitions(-DMUSIC_MID_NATIVE)
        list(APPEND SDLMixerX_SOURCES
            ${SDLMixerX_SOURCE_DIR}/src/codecs/music_nativemidi.c
            ${SDLMixerX_SOURCE_DIR}/src/codecs/native_midi/native_midi_common.c)
        if(WIN32)
            list(APPEND SDLMixerX_SOURCES
                ${SDLMixerX_SOURCE_DIR}/src/codecs/native_midi/native_midi_win32.c)
            list(APPEND SDLMixerX_LINK_LIBS winmm)
        endif()
        if(APPLE)
            list(APPEND SDLMixerX_SOURCES
                ${SDLMixerX_SOURCE_DIR}/src/codecs/native_midi/native_midi_macosx.c)
        endif()
    endif()
    if(NOT USE_MIDI_ADLMIDI AND
       NOT USE_MIDI_OPNMIDI AND
       NOT USE_MIDI_TIMIDITY AND
       NOT USE_MIDI_FLUIDSYNTH AND
       NOT USE_MIDI_NATIVE)
        message_error("To have MIDI support you must enable at least one MIDI sequencer library")
    endif()

endif()

list(APPEND SDLMixerX_LINK_LIBS SDL2)

if(WIN32)
    option(BUILD_AS_VB6_BINDING "Compile library into solid module compatible with VisualBasic 6" OFF)
    if(BUILD_AS_VB6_BINDING)
        message("== SDL Mixer X will be built as solid module that is ready for MS-VB6 binding! ==")
        list(APPEND SDLMixerX_SOURCES ${SDLMixerX_SOURCE_DIR}/src/vb6_binding/vb6_sdl_binds.c)
        list(REMOVE_ITEM SDLMixerX_LINK_LIBS SDL2)
        list(APPEND SDLMixerX_LINK_LIBS
                    SDL2-static
                    uuid winmm ole32 imm32 version oleaut32 user32 gdi32)
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
    set_target_properties(SDL2_mixer_ext_Static PROPERTIES OUTPUT_NAME SDL2_mixer_ext)
    target_include_directories(SDL2_mixer_ext_Static PUBLIC ${SDLMixerX_SOURCE_DIR}/include)
    target_include_directories(SDL2_mixer_ext_Static PRIVATE
        ${SDLMixerX_SOURCE_DIR}/include/SDL_mixer_ext
        ${SDLMixerX_SOURCE_DIR}/src
        ${SDLMixerX_SOURCE_DIR}/src/codecs
        ${AUDIO_CODECS_INSTALL_DIR}/include/SDL2)
    if(DOWNLOAD_AUDIO_CODECS_DEPENDENCY AND AUDIO_CODECS_REPO_PATH)
        add_dependencies(SDL2_mixer_ext_Static AudioCodecs)
    endif()
    set_target_properties(SDL2_mixer_ext_Static
        PROPERTIES
        ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"
        LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
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
    target_include_directories(SDL2_mixer_ext PUBLIC ${SDLMixerX_SOURCE_DIR}/include)
    target_include_directories(SDL2_mixer_ext PRIVATE
        ${SDLMixerX_SOURCE_DIR}/include/SDL_mixer_ext
        ${SDLMixerX_SOURCE_DIR}/src
        ${SDLMixerX_SOURCE_DIR}/src/codecs
        ${AUDIO_CODECS_INSTALL_DIR}/include/SDL2)
    if(DOWNLOAD_AUDIO_CODECS_DEPENDENCY AND AUDIO_CODECS_REPO_PATH)
        add_dependencies(SDL2_mixer_ext AudioCodecs)
    endif()
    set_target_properties(SDL2_mixer_ext
        PROPERTIES
        ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"
        LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
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
    target_include_directories(SDL2_mixer_ext_VB6 PUBLIC ${SDLMixerX_SOURCE_DIR}/include)
    target_include_directories(SDL2_mixer_ext_VB6 PUBLIC
        ${SDLMixerX_SOURCE_DIR}/include/SDL_mixer_ext
        ${SDLMixerX_SOURCE_DIR}/src
        ${SDLMixerX_SOURCE_DIR}/src/codecs
        ${AUDIO_CODECS_INSTALL_DIR}/include/SDL2)
    set_target_properties(SDL2_mixer_ext_VB6 PROPERTIES LINK_FLAGS
        "-Wl,--add-stdcall-alias -static-libgcc -static-libstdc++ -static -lpthread")
    if(DOWNLOAD_AUDIO_CODECS_DEPENDENCY AND AUDIO_CODECS_REPO_PATH)
        add_dependencies(SDL2_mixer_ext_VB6 AudioCodecs)
    endif()
    set_target_properties(SDL2_mixer_ext_VB6
        PROPERTIES
        ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/sdl-mixer-vb6"
        LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/sdl-mixer-vb6"
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/sdl-mixer-vb6"
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

