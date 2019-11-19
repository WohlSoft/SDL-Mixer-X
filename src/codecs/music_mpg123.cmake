
option(USE_MP3_MPG123     "[WIP, DON'T USE IT] Build with MPG123 MP3 codec" OFF)
if(USE_MP3_MPG123)
    add_definitions(-DMUSIC_MP3_MPG123)

    message(WARNING "MPG123 SUPPORT IS WIP, DON'T USE IT IN PRODUCTION!!!")
    # ======= Until AudioCodecs will receive buildable mpg123, detect it externally =======
    find_package(Mpg123 REQUIRED)
    message("MPG123 found in ${MPG123_INCLUDE_DIR} folder")

    list(APPEND SDL_MIXER_INCLUDE_PATHS ${MPG123_INCLUDE_DIRS})
    set(LIBS ${LIBS} ${MPG123_LIBRARIES})
    list(APPEND SDLMixerX_LINK_LIBS mpg123)
    list(APPEND SDLMixerX_SOURCES
        ${CMAKE_CURRENT_LIST_DIR}/music_mpg123.c)
endif()
