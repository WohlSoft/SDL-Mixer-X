
option(USE_MP3_MPG123     "[WIP, DON'T USE IT] Build with MPG123 MP3 codec" OFF)
if(USE_MP3_MPG123)
    option(USE_MP3_MPG123_DYNAMIC "Use dynamical loading of MPG123" OFF)

    if(NOT USE_SYSTEM_AUDIO_LIBRARIES)
        # ======= Until AudioCodecs will receive buildable mpg123, detect it externally =======
        message(WARNING "AudioCodecs doesn't ship MPG123 YET, it will be recognized from a system!!!")
    endif()

    find_package(Mpg123 REQUIRED)
    message("MPG123 found in ${MPG123_INCLUDE_DIR} folder")
    if(USE_MP3_MPG123_DYNAMIC)
        add_definitions(-DMPG123_DYNAMIC=\"${MPG123_DYNAMIC_LIBRARY}\")
        message("Dynamic MPG123: ${MPG123_DYNAMIC_LIBRARY}")
    endif()

    if(MPG123_FOUND)
        add_definitions(-DMUSIC_MP3_MPG123)
        list(APPEND SDL_MIXER_INCLUDE_PATHS ${MPG123_INCLUDE_DIRS})
        if(NOT USE_SYSTEM_AUDIO_LIBRARIES OR NOT USE_MP3_MPG123_DYNAMIC)
            list(APPEND SDLMixerX_LINK_LIBS ${MPG123_LIBRARIES})
        endif()
        list(APPEND SDLMixerX_SOURCES ${CMAKE_CURRENT_LIST_DIR}/music_mpg123.c)
    endif()
endif()
