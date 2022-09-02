option(USE_FFMPEG             "Build with FFMPEG library (for AAC and WMA formats) (LGPL)" OFF)
if(USE_FFMPEG AND MIXERX_LGPL)
    list(APPEND SDL_MIXER_DEFINITIONS -DMUSIC_FFMPEG)
    list(APPEND SDLMixerX_LINK_LIBS swresample avcodec avformat avutil)
    list(APPEND SDLMixerX_SOURCES
        ${CMAKE_CURRENT_LIST_DIR}/music_ffmpeg.c
        ${CMAKE_CURRENT_LIST_DIR}/music_ffmpeg.h
    )
    appendPcmFormats("WMA;AAC")
endif()
