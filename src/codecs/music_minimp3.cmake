option(USE_MP3_MINIMP3  "Build with MINIMP3 codec" ON)

if(USE_MP3_MINIMP3)
    message("== using MINIMP3 (CC0-1.0) ==")
    setLicense(LICENSE_PUBLICDOMAIN)
    list(APPEND SDL_MIXER_DEFINITIONS -DMUSIC_MP3_MINIMP3)
    list(APPEND SDLMixerX_SOURCES
        ${CMAKE_CURRENT_LIST_DIR}/music_minimp3.c
        ${CMAKE_CURRENT_LIST_DIR}/music_minimp3.h
    )
endif()
