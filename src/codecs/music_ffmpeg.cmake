option(USE_FFMPEG             "Build with FFMPEG library (for AAC and WMA formats) (LGPL)" OFF)
if(USE_FFMPEG AND MIXERX_LGPL)
    list(APPEND SDL_MIXER_DEFINITIONS -DMUSIC_FFMPEG)

    option(USE_CUSTOM_FFMPEG  "Use the custom minified FFMPEG build" OFF)
    if(USE_CUSTOM_FFMPEG)
        list(APPEND SDL_MIXER_INCLUDE_PATHS "/home/vitaly/_git_repos_glosaw/ffmpeg-5.1.1-wma/include")
        list(APPEND SDLMixerX_LINK_LIBS
               /home/vitaly/_git_repos_glosaw/ffmpeg-5.1.1-wma/lib/libswresamplemixerx.a
               /home/vitaly/_git_repos_glosaw/ffmpeg-5.1.1-wma/lib/libavformatmixerx.a
               /home/vitaly/_git_repos_glosaw/ffmpeg-5.1.1-wma/lib/libavcodecmixerx.a
               /home/vitaly/_git_repos_glosaw/ffmpeg-5.1.1-wma/lib/libavutilmixerx.a
        )
    else()
        list(APPEND SDLMixerX_LINK_LIBS
            swresample avformat avcodec avutil
        )
    endif()

    list(APPEND SDLMixerX_SOURCES
        ${CMAKE_CURRENT_LIST_DIR}/music_ffmpeg.c
        ${CMAKE_CURRENT_LIST_DIR}/music_ffmpeg.h
    )
    set_source_files_properties("${CMAKE_CURRENT_LIST_DIR}/music_ffmpeg.c"
        COMPILE_FLAGS
            "-std=c99"
    )
    appendPcmFormats("WMA;AAC;OPUS")
endif()
