option(USE_OPUS      "Build with OPUS codec" ON)
if(USE_OPUS)
    if(USE_SYSTEM_AUDIO_LIBRARIES)
        find_path(LIBOPUS_INCLUDE_DIR opus/opusfile.h)
        find_library(LIBOPUSFILE_LIB NAMES opusfile)
        find_library(LIBOPUS_LIB NAMES opus)
        if(LIBOPUS_INCLUDE_DIR AND LIBOPUSFILE_LIB AND LIBOPUS_LIB)
            set(FOUND_OPUS 1)
        endif()
        message("Opus: [${FOUND_OPUS}] ${LIBOPUS_INCLUDE_DIR} ${LIBOPUSFILE_LIB} ${LIBOPUS_LIB}")
        if(FOUND_OPUS)
            list(APPEND SDL_MIXER_INCLUDE_PATHS
                ${LIBOPUS_INCLUDE_DIR}/opus
            )
        endif()
    else()
        set(FOUND_OPUS 1)
        if(DOWNLOAD_AUDIO_CODECS_DEPENDENCY)
            set(LIBOPUSFILE_LIB opusfile)
            set(LIBOPUS_LIB opus)
        else()
            find_library(LIBOPUSFILE_LIB NAMES opusfile
                         HINTS "${AUDIO_CODECS_INSTALL_PATH}/lib")
            find_library(LIBOPUS_LIB NAMES opus
                         HINTS "${AUDIO_CODECS_INSTALL_PATH}/lib")
        endif()
    endif()
    if(FOUND_OPUS)
        message("== using Opus ==")
        add_definitions(-DMUSIC_OPUS)
        if(AUDIO_CODECS_REPO_PATH)
            list(APPEND SDL_MIXER_INCLUDE_PATHS
                ${AUDIO_CODECS_PATH}/libogg/include
                ${AUDIO_CODECS_PATH}/libopus/include
                ${AUDIO_CODECS_PATH}/libopusfile/include
            )
        endif()
        if(AUDIO_CODECS_INSTALL_DIR)
            list(APPEND SDL_MIXER_INCLUDE_PATHS ${AUDIO_CODECS_INSTALL_DIR}/include/opus)
        endif()
        list(APPEND SDLMixerX_LINK_LIBS ${LIBOPUSFILE_LIB} ${LIBOPUS_LIB})
        set(LIBOGG_NEEDED ON)
        list(APPEND SDLMixerX_SOURCES
            ${SDLMixerX_SOURCE_DIR}/src/codecs/music_opus.c)
    endif()
endif()
