option(USE_MIDI_TIMIDITY   "Build with Timidity wave table MIDI sequencer support" ON)
if(USE_MIDI_TIMIDITY AND NOT USE_SYSTEM_AUDIO_LIBRARIES)
    message("== using Timidity-SDL ==")

    if(DOWNLOAD_AUDIO_CODECS_DEPENDENCY)
        set(TIMIDITYSDL_LIBRARIES timidity_sdl2)
        set(TIMIDITYSDL_FOUND True)
    else()
        find_library(TIMIDITYSDL_LIBRARIES NAMES timidity_sdl2
                     HINTS "${AUDIO_CODECS_INSTALL_PATH}/lib")
        if(TIMIDITYSDL_LIBRARIES)
            set(TIMIDITYSDL_FOUND True)
        endif()
    endif()

    if(TIMIDITYSDL_FOUND)
        list(APPEND SDL_MIXER_DEFINITIONS -DMUSIC_MID_TIMIDITY)
        if(AUDIO_CODECS_REPO_PATH)
            list(APPEND SDL_MIXER_INCLUDE_PATHS ${AUDIO_CODECS_PATH}/libtimidity/include)
        endif()
        list(APPEND SDLMixerX_LINK_LIBS ${TIMIDITYSDL_LIBRARIES})
        list(APPEND SDLMixerX_SOURCES ${CMAKE_CURRENT_LIST_DIR}/music_timidity.c)
    endif()
endif()
