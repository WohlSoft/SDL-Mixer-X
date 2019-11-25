option(USE_MIDI_TIMIDITY   "Build with Timidity wave table MIDI sequencer support" ON)
if(USE_MIDI_TIMIDITY AND NOT USE_SYSTEM_AUDIO_LIBRARIES)
    message("== using Timidity-SDL ==")
    list(APPEND SDL_MIXER_DEFINITIONS -DMUSIC_MID_TIMIDITY)

    if(AUDIO_CODECS_REPO_PATH)
        list(APPEND SDL_MIXER_INCLUDE_PATHS ${AUDIO_CODECS_PATH}/libtimidity/include)
    endif()

    if(DOWNLOAD_AUDIO_CODECS_DEPENDENCY)
        set(LIBTIMIDITY_LIB timidity)
    else()
        find_library(LIBTIMIDITY_LIB NAMES timidity
                     HINTS "${AUDIO_CODECS_INSTALL_PATH}/lib")
    endif()

    list(APPEND SDLMixerX_LINK_LIBS ${LIBTIMIDITY_LIB})
endif()

list(APPEND SDLMixerX_SOURCES ${CMAKE_CURRENT_LIST_DIR}/music_timidity.c)
