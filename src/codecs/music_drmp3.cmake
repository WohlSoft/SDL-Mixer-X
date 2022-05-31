option(USE_MP3_DRMP3  "Build with MP3 codec using dr_mp3" ON)
option(USE_MP3_MINIMP3  "[compatibility fallback] Build with MP3 codec using dr_mp3" OFF)

mark_as_advanced(USE_MP3_MINIMP3)

if(USE_MP3_DRMP3 OR USE_MP3_MINIMP3)
    message("== using DRMP3 (public domain or MIT-0) ==")
    setLicense(LICENSE_PUBLICDOMAIN)
    list(APPEND SDL_MIXER_DEFINITIONS -DMUSIC_MP3_DRMP3)
    list(APPEND SDLMixerX_SOURCES
        ${CMAKE_CURRENT_LIST_DIR}/music_drmp3.c
        ${CMAKE_CURRENT_LIST_DIR}/music_drmp3.h
    )
    appendPcmFormats("MP3")
endif()
