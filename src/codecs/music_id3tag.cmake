option(USE_MP3_ID3TAG   "Build with MP3 Meta tags support provided by libID3Tag library" ON)
if(USE_MP3_ID3TAG AND NOT USE_SYSTEM_AUDIO_LIBRARIES)
    message("== using ID3Tag-SDL ==")
    list(APPEND SDL_MIXER_DEFINITIONS -DMUSIC_MP3_ID3TAG)
    if(AUDIO_CODECS_REPO_PATH)
        list(APPEND SDL_MIXER_INCLUDE_PATHS ${AUDIO_CODECS_PATH}/libid3tag/include)
    endif()
    if(DOWNLOAD_AUDIO_CODECS_DEPENDENCY)
        set(LIBID3TAG_LIB id3tag)
    else()
        find_library(LIBID3TAG_LIB NAMES id3tag
                     HINTS "${AUDIO_CODECS_INSTALL_PATH}/lib")
    endif()
    list(APPEND SDLMixerX_LINK_LIBS ${LIBID3TAG_LIB})
endif()
