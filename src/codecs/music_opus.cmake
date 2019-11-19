option(USE_OPUS      "Build with OPUS codec" ON)
if(USE_OPUS)
    option(USE_OPUS_DYNAMIC "Use dynamical loading of Opus" OFF)

    if(USE_SYSTEM_AUDIO_LIBRARIES)
        find_package(Opus)
        message("Opus: [${Opus_FOUND}] ${Opus_INCLUDE_DIRS} ${Opus_LIBRARIES} ${LIBOPUS_LIB}")
        if(USE_OPUS_DYNAMIC)
            add_definitions(-DOPUS_DYNAMIC=\"${OpusFile_DYNAMIC_LIBRARY}\")
            message("Dynamic Opus: ${OpusFile_DYNAMIC_LIBRARY}")
        endif()
    else()
        set(Opus_FOUND 1)
        if(DOWNLOAD_AUDIO_CODECS_DEPENDENCY)
            set(Opus_LIBRARIES opusfile)
            set(LIBOPUS_LIB opus)
        else()
            find_library(Opus_LIBRARIES NAMES opusfile
                         HINTS "${AUDIO_CODECS_INSTALL_PATH}/lib")
            find_library(LIBOPUS_LIB NAMES opus
                         HINTS "${AUDIO_CODECS_INSTALL_PATH}/lib")
        endif()
        set(Opus_INCLUDE_DIRS
            ${AUDIO_CODECS_PATH}/libogg/include
            ${AUDIO_CODECS_PATH}/libopus/include
            ${AUDIO_CODECS_PATH}/libopusfile/include
        )
    endif()

    if(Opus_FOUND)
        message("== using Opus ==")
        add_definitions(-DMUSIC_OPUS)
        if(NOT USE_SYSTEM_AUDIO_LIBRARIES OR NOT USE_OPUS_DYNAMIC)
            list(APPEND SDLMixerX_LINK_LIBS ${Opus_LIBRARIES} ${LIBOPUS_LIB})
            set(LIBOGG_NEEDED ON)
        endif()
        list(APPEND SDL_MIXER_INCLUDE_PATHS ${Opus_INCLUDE_DIRS})
        list(APPEND SDLMixerX_SOURCES
            ${CMAKE_CURRENT_LIST_DIR}/music_opus.c)
    endif()
endif()
