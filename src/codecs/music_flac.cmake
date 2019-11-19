option(USE_FLAC            "Build with FLAC codec" ON)
if(USE_FLAC)
    if(USE_SYSTEM_AUDIO_LIBRARIES)
        find_path(LIBFLAC_INCLUDE_DIR "FLAC/all.h")
        find_library(LIBFLAC_LIB NAMES FLAC)
        if(LIBFLAC_INCLUDE_DIR AND LIBFLAC_LIB)
            set(FOUND_FLAC 1)
        endif()
        message("FLAC: [${FOUND_FLAC}] ${LIBFLAC_INCLUDE_DIR} ${LIBFLAC_LIB}")
    else()
        set(FOUND_FLAC 1)
        if(DOWNLOAD_AUDIO_CODECS_DEPENDENCY)
            set(LIBFLAC_LIB FLAC)
        else()
            find_library(LIBFLAC_LIB NAMES FLAC
                         HINTS "${AUDIO_CODECS_INSTALL_PATH}/lib")
        endif()
    endif()
    if(FOUND_FLAC)
        message("== using FLAC ==")
        add_definitions(-DMUSIC_FLAC -DFLAC__NO_DLL)
        if(AUDIO_CODECS_REPO_PATH)
            list(APPEND SDL_MIXER_INCLUDE_PATHS
                ${AUDIO_CODECS_PATH}/libogg/include
                ${AUDIO_CODECS_PATH}/libFLAC/include
            )
        endif()
        set(LIBOGG_NEEDED ON)
        list(APPEND SDLMixerX_LINK_LIBS ${LIBFLAC_LIB})
        list(APPEND SDLMixerX_SOURCES
            ${CMAKE_CURRENT_LIST_DIR}/music_flac.c)
    endif()
endif()

