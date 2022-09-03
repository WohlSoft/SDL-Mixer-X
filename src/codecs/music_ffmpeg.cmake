option(USE_FFMPEG             "Build with FFMPEG library (for AAC and WMA formats) (LGPL)" OFF)
if(USE_FFMPEG AND MIXERX_LGPL)
    list(APPEND SDL_MIXER_DEFINITIONS -DMUSIC_FFMPEG)
    list(APPEND SDLMixerX_LINK_LIBS
           /usr/local/lib/libswresample.so
           /usr/local/lib/libavcodec.so
           /usr/local/lib/libavformat.so
           /usr/local/lib/libavutil.so
    )
    list(APPEND SDLMixerX_SOURCES
        ${CMAKE_CURRENT_LIST_DIR}/music_ffmpeg.c
        ${CMAKE_CURRENT_LIST_DIR}/music_ffmpeg.h
    )
    appendPcmFormats("WMA;AAC")
endif()
