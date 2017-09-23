cmake_minimum_required (VERSION 2.8.11)
project(SDLMixerX C)

#include(CheckCSourceRuns)
#include(CheckCCompilerFlag)
#include(CheckFunctionExists)
#include(CheckLibraryExists)

option(SSEMATH             "Allow GCC to use SSE floating point math" ${OPT_DEF_SSEMATH})
option(MMX                 "Use MMX assembly routines" ${OPT_DEF_ASM})
option(3DNOW               "Use 3Dnow! MMX assembly routines" ${OPT_DEF_ASM})
option(SSE                 "Use SSE assembly routines" ${OPT_DEF_ASM})
option(SSE2                "Use SSE2 assembly routines" ${OPT_DEF_SSEMATH})
option(SSE3                "Use SSE3 assembly routines" ${OPT_DEF_SSEMATH})

set(AUDIO_CODECS_PATH ${SDLMixerX_SOURCE_DIR}/../AudioCodecs)

add_definitions(-DHAVE_INTTYPES_H -DHAVE_SETENV -DHAVE_SINF)
add_definitions(-Dmain=SDL_main -DPIC -D_REENTRANT -D_USE_MATH_DEFINES)

set(SDLMixerX_SOURCES)
set(SDLMixerX_LINK_LIBS)

link_directories(${AUDIO_CODECS_PATH}/build/lib)

# libSDL2
if(EXISTS "${AUDIO_CODECS_PATH}/libSDL2/include/")
    # TODO: Implement automatical downloading and unpacking of hg.libsdl.org/SDL
    # or using of separated repository for libSDL's backup
    message("SDL2 found in ${AUDIO_CODECS_PATH}/libSDL2/include folder")
    include_directories(${AUDIO_CODECS_PATH}/libSDL2/include)
    list(APPEND SDLMixerX_LINK_LIBS SDL2)
else()
    find_library(SDL2_LIBRARY SDL2)
    message("Found ${SDL2_LIBRARY}")
endif()




option(USE_WAV             "Build with WAV codec" ON)
if(USE_WAV)
    add_definitions(-DWAV_MUSIC)
    list(APPEND SDLMixerX_SOURCES
        ${SDLMixerX_SOURCE_DIR}/src/codecs/load_aiff.c
        ${SDLMixerX_SOURCE_DIR}/src/codecs/load_voc.c
        ${SDLMixerX_SOURCE_DIR}/src/codecs/music_wave.c)
endif()

option(USE_OGG_VORBIS      "Build with OGG Vorbis codec" ON)
if(USE_OGG_VORBIS)
    add_definitions(-DOGG_MUSIC)
    include_directories(${AUDIO_CODECS_PATH}/libogg/include)
    include_directories(${AUDIO_CODECS_PATH}/libvorbis/include)
    list(APPEND SDLMixerX_LINK_LIBS vorbisfile vorbis)
    set(LIBOGG_NEEDED ON)
    list(APPEND SDLMixerX_SOURCES
        ${SDLMixerX_SOURCE_DIR}/src/codecs/dynamic_ogg.c
        ${SDLMixerX_SOURCE_DIR}/src/codecs/load_ogg.c
        ${SDLMixerX_SOURCE_DIR}/src/codecs/music_ogg.c)
endif()

option(USE_FLAC            "Build with FLAC codec" ON)
if(USE_FLAC)
    add_definitions(-DFLAC_MUSIC -DFLAC__NO_DLL)
    include_directories(${AUDIO_CODECS_PATH}/libogg/include)
    include_directories(${AUDIO_CODECS_PATH}/libFLAC/include)
    set(LIBOGG_NEEDED ON)
    list(APPEND SDLMixerX_LINK_LIBS FLAC)
    list(APPEND SDLMixerX_SOURCES
        ${SDLMixerX_SOURCE_DIR}/src/codecs/dynamic_flac.c
        ${SDLMixerX_SOURCE_DIR}/src/codecs/load_flac.c
        ${SDLMixerX_SOURCE_DIR}/src/codecs/music_flac.c)
endif()

if(LIBOGG_NEEDED)
    list(APPEND SDLMixerX_LINK_LIBS ogg)
endif()

option(USE_MP3_MAD         "Build with MAD MP3 codec" ON)
if(USE_MP3_MAD)
    add_definitions(-DMP3_MAD_MUSIC)
    list(APPEND SDLMixerX_LINK_LIBS mad id3tag)
    include_directories(${AUDIO_CODECS_PATH}/libmad/include)
    include_directories(${AUDIO_CODECS_PATH}/libid3tag/include)
    list(APPEND SDLMixerX_SOURCES
        ${SDLMixerX_SOURCE_DIR}/src/codecs/dynamic_mp3.c
        ${SDLMixerX_SOURCE_DIR}/src/codecs/load_mp3.c
        ${SDLMixerX_SOURCE_DIR}/src/codecs/music_mad.c)
endif()

option(USE_MP3_SMPEG       "Build with SMPEG MP3 codec" OFF)
if(USE_MP3_SMPEG)
    add_definitions(-DMP3_MUSIC)
    list(APPEND SDLMixerX_LINK_LIBS smpeg id3tag)
    include_directories(${AUDIO_CODECS_PATH}/smpeg/include)
    include_directories(${AUDIO_CODECS_PATH}/libid3tag/include)
    list(APPEND SDLMixerX_SOURCES
        ${SDLMixerX_SOURCE_DIR}/src/codecs/dynamic_mp3.c
        ${SDLMixerX_SOURCE_DIR}/src/codecs/load_mp3.c)
endif()

option(USE_MODPLUG         "Build with ModPlug library" ON)
if(USE_MODPLUG)
    add_definitions(-DMODPLUG_MUSIC -DMODPLUG_STATIC)
    include_directories(${AUDIO_CODECS_PATH}/libmodplug/include)
    list(APPEND SDLMixerX_LINK_LIBS modplug)
    list(APPEND SDLMixerX_SOURCES
        ${SDLMixerX_SOURCE_DIR}/src/codecs/dynamic_modplug.c
        ${SDLMixerX_SOURCE_DIR}/src/codecs/music_modplug.c)
endif()

option(USE_MIKMOD          "Build with MikMod library" OFF)
if(USE_MIKMOD)
    add_definitions(-DMOD_MUSIC)
    include_directories(${AUDIO_CODECS_PATH}/libmikmod/include)
    list(APPEND SDLMixerX_LINK_LIBS mikmod)
    list(APPEND SDLMixerX_SOURCES
        ${SDLMixerX_SOURCE_DIR}/src/codecs/dynamic_mod.c
        ${SDLMixerX_SOURCE_DIR}/src/codecs/music_mod.c)
endif()

option(USE_GME             "Build with Game Music Emulators library" ON)
if(USE_GME)
    add_definitions(-DGME_MUSIC)
    include_directories(${AUDIO_CODECS_PATH}/libgme/include)
    include_directories(${AUDIO_CODECS_PATH}/zlib/include)
    list(APPEND SDLMixerX_LINK_LIBS gme zlib)
    list(APPEND SDLMixerX_SOURCES
        ${SDLMixerX_SOURCE_DIR}/src/codecs/music_gme.c)
endif()

option(USE_CMD             "Build with CMD music player support" ON)
if(USE_CMD)
    add_definitions(-DCMD_MUSIC)
    list(APPEND SDLMixerX_SOURCES
        ${SDLMixerX_SOURCE_DIR}/src/codecs/music_cmd.c)
endif()

option(USE_MIDI            "Build with MIDI support" ON)
if(USE_MIDI)
    add_definitions(-DMID_MUSIC)

    option(USE_MIDI_ADLMIDI    "Build with libADLMIDI OPL3 Emulator based MIDI sequencer support" ON)
    if(USE_MIDI_ADLMIDI)
        add_definitions(-DUSE_ADL_MIDI)
        include_directories(${AUDIO_CODECS_PATH}/libADLMIDI/include)
        list(APPEND SDLMixerX_LINK_LIBS ADLMIDI)
        list(APPEND SDLMixerX_SOURCES
            ${SDLMixerX_SOURCE_DIR}/src/codecs/music_midi_adl.c)
    endif()

    option(USE_MIDI_OPNMIDI    "Build with libOPNMIDI OPN2 Emulator based MIDI sequencer support" ON)
    if(USE_MIDI_OPNMIDI)
        add_definitions(-DUSE_OPN2_MIDI)
        include_directories(${AUDIO_CODECS_PATH}/libOPNMIDI/include)
        list(APPEND SDLMixerX_LINK_LIBS OPNMIDI)
        list(APPEND SDLMixerX_SOURCES
            ${SDLMixerX_SOURCE_DIR}/src/codecs/music_midi_opn.c)
    endif()

    option(USE_MIDI_TIMIDITY   "Build with Timidity wave table MIDI sequencer support" ON)
    if(USE_MIDI_TIMIDITY)
        add_definitions(-DUSE_TIMIDITY_MIDI)
        include_directories(${AUDIO_CODECS_PATH}/libtimidity/include)
        list(APPEND SDLMixerX_LINK_LIBS timidity)
        list(APPEND SDLMixerX_SOURCES
            ${SDLMixerX_SOURCE_DIR}/src/codecs/music_midi_timidity.c)
    endif()

    option(USE_MIDI_FLUIDSYNTH "Build with FluidSynth wave table MIDI sequencer support" OFF)
    if(USE_MIDI_FLUIDSYNTH)
        add_definitions(-DUSE_FLUIDSYNTH_MIDI)
        include_directories(${AUDIO_CODECS_PATH}/FluidLite/include)
        list(APPEND SDLMixerX_LINK_LIBS fluidlite)
        list(APPEND SDLMixerX_SOURCES
            ${SDLMixerX_SOURCE_DIR}/src/codecs/dynamic_fluidsynth.c
            ${SDLMixerX_SOURCE_DIR}/src/codecs/music_fluidsynth.c)
    endif()

     # Native MIDI correctly works on Windows only. Mac works,
     # but it is useless without external synthesizer (which is not provided
     # like MS GS on Windows)
    if(WIN32)
        set(NATIVE_MIDI_SUPPORTED ON)
    else()
        set(NATIVE_MIDI_SUPPORTED OFF)
    endif()

    option(USE_MIDI_NATIVE     "Build with operating system native MIDI output support" ${NATIVE_MIDI_SUPPORTED})
    if(USE_MIDI_NATIVE)
        add_definitions(-DUSE_NATIVE_MIDI)
        list(APPEND SDLMixerX_SOURCES
            ${SDLMixerX_SOURCE_DIR}/src/codecs/native_midi/native_midi_common.c)
        if(WIN32)
            list(APPEND SDLMixerX_SOURCES
                ${SDLMixerX_SOURCE_DIR}/src/codecs/native_midi/native_midi_win32.c)
        endif()
        if(APPLE)
            list(APPEND SDLMixerX_SOURCES
                ${SDLMixerX_SOURCE_DIR}/src/codecs/native_midi/native_midi_mac.c
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

if(WIN32)
    option(BUILD_AS_VB6_BINDING "Compile library into solid module compatible with VisualBasic 6" OFF)
    if(BUILD_AS_VB6_BINDING)
        add_definitions(-DFORCE_STDCALLS)
        list(APPEND SDLMixerX_SOURCES ${SDLMixerX_SOURCE_DIR}/src/vb6_binding/vb6_sdl_binds.c)
    endif()
endif()

list(APPEND SDLMixerX_SOURCES
        ${SDLMixerX_SOURCE_DIR}/src/resample/my_resample.c
        ${SDLMixerX_SOURCE_DIR}/src/audio_codec.c
        ${SDLMixerX_SOURCE_DIR}/src/effect_position.c
        ${SDLMixerX_SOURCE_DIR}/src/effects_internal.c
        ${SDLMixerX_SOURCE_DIR}/src/effect_stereoreverse.c
        ${SDLMixerX_SOURCE_DIR}/src/mixer.c
        ${SDLMixerX_SOURCE_DIR}/src/music.c
    )

link_directories(${AUDIO_CODECS_PATH}/build/lib)

file(GLOB SDLMixerX_SOURCES ${SDLMixerX_SOURCES})

add_library(SDL_mixer_ext SHARED ${SDLMixerX_SOURCES})
target_link_libraries(SDL_mixer_ext ${SDLMixerX_LINK_LIBS})

target_include_directories(SDL_mixer_ext PUBLIC ${SDLMixerX_SOURCE_DIR}/include)
