option(USE_MIDI_FLUIDSYNTH "Build with FluidSynth wave table MIDI sequencer support" OFF)
if(USE_MIDI_FLUIDSYNTH)
    option(USE_MIDI_FLUIDSYNTH_DYNAMIC "Use dynamical loading of FluidSynth" OFF)

    if(USE_SYSTEM_AUDIO_LIBRARIES)
        find_package(FluidSynth QUIET)
        message("FluidSynth: [${FluidSynth_FOUND}] ${FluidSynth_INCLUDE_DIRS} ${FluidSynth_LIBRARIES}")
        if(USE_MIDI_FLUIDSYNTH_DYNAMIC)
            list(APPEND SDL_MIXER_DEFINITIONS -DFLUIDSYNTH_DYNAMIC=\"${FluidSynth_DYNAMIC_LIBRARY}\")
            message("Dynamic FluidSynth: ${FluidSynth_DYNAMIC_LIBRARY}")
        endif()
    else()
        if(DOWNLOAD_AUDIO_CODECS_DEPENDENCY)
            set(LIBFLUIDSYNTH_LIB fluidsynth)
        else()
            find_library(FluidSynth_LIBRARIES NAMES fluidsynth
                         HINTS "${AUDIO_CODECS_INSTALL_PATH}/lib")
        endif()
        set(FluidSynth_FOUND 1)
        set(FluidSynth_INCLUDE_DIRS ${AUDIO_CODECS_PATH}/FluidLite/include)
    endif()

    if(FluidSynth_FOUND)
        message("== using FluidSynth ==")
        list(APPEND SDL_MIXER_DEFINITIONS -DMUSIC_MID_FLUIDSYNTH)
        list(APPEND SDL_MIXER_INCLUDE_PATHS ${FluidSynth_INCLUDE_DIRS})
        if(NOT USE_SYSTEM_AUDIO_LIBRARIES OR NOT USE_MIDI_FLUIDSYNTH_DYNAMIC)
            list(APPEND SDLMixerX_LINK_LIBS ${FluidSynth_LIBRARIES})
        endif()
        list(APPEND SDLMixerX_SOURCES
            ${CMAKE_CURRENT_LIST_DIR}/music_fluidsynth.c)
    else()
        message("-- skipping FluidSynth --")
    endif()
endif()
