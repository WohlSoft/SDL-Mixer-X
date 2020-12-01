option(USE_MIDI_ADLMIDI    "Build with libADLMIDI OPL3 Emulator based MIDI sequencer support" ON)
if(USE_MIDI_ADLMIDI AND NOT SDL_MIXER_CLEAR_FOR_ZLIB_LICENSE AND NOT SDL_MIXER_CLEAR_FOR_LGPL_LICENSE)
    option(USE_MIDI_ADLMIDI_DYNAMIC "Use dynamical loading of libADLMIDI library" OFF)

    if(USE_SYSTEM_AUDIO_LIBRARIES)
        find_package(ADLMIDI QUIET)
        message("ADLMIDI: [${ADLMIDI_FOUND}] ${ADLMIDI_INCLUDE_DIRS} ${ADLMIDI_LIBRARIES}")
        if(USE_MIDI_ADLMIDI_DYNAMIC)
            list(APPEND SDL_MIXER_DEFINITIONS -DADLMIDI_DYNAMIC=\"${ADLMIDI_DYNAMIC_LIBRARY}\")
            message("Dynamic libADLMIDI: ${ADLMIDI_DYNAMIC_LIBRARY}")
        endif()

        cpp_needed(${SDLMixerX_SOURCE_DIR}/cmake/tests/cpp_needed/adlmidi.c
            ""
            ${ADLMIDI_INCLUDE_DIRS}
            "${ADLMIDI_LIBRARIES};${M_LIBRARY}"
            STDCPP_NEEDED
        )

    else()
        if(DOWNLOAD_AUDIO_CODECS_DEPENDENCY)
            set(ADLMIDI_LIBRARIES ADLMIDI)
        else()
            find_library(ADLMIDI_LIBRARIES NAMES ADLMIDI HINTS "${AUDIO_CODECS_INSTALL_PATH}/lib")
        endif()
        if(ADLMIDI_LIBRARIES)
            set(ADLMIDI_FOUND 1)
            set(STDCPP_NEEDED 1) # Statically linking ADLMIDI which is C++ library
        endif()
        set(ADLMIDI_INCLUDE_DIRS "${AUDIO_CODECS_PATH}/libADLMIDI/include")
    endif()

    if(ADLMIDI_FOUND)
        message("== using ADLMIDI (GPLv3+) ==")
        setLicense(LICENSE_GPL_3p)
        list(APPEND SDL_MIXER_DEFINITIONS -DMUSIC_MID_ADLMIDI)
        set(LIBMATH_NEEDED 1)
        list(APPEND SDL_MIXER_INCLUDE_PATHS ${ADLMIDI_INCLUDE_DIRS})
        if(NOT USE_SYSTEM_AUDIO_LIBRARIES OR NOT USE_MIDI_ADLMIDI_DYNAMIC)
            list(APPEND SDLMixerX_LINK_LIBS ${ADLMIDI_LIBRARIES})
        endif()
    else()
        message("-- skipping ADLMIDI --")
    endif()
endif()

# Keep this file always built as it contains dummies of some public calls to avoid link errors
list(APPEND SDLMixerX_SOURCES ${CMAKE_CURRENT_LIST_DIR}/music_midi_adl.c)
