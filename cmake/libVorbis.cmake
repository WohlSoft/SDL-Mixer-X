option(USE_OGG_VORBIS      "Build with OGG Vorbis codec" ON)
if(USE_OGG_VORBIS)
    option(USE_OGG_VORBIS_DYNAMIC "Use dynamical loading of OGG Vorbis" OFF)

    if(USE_SYSTEM_AUDIO_LIBRARIES)
        find_package(Vorbis REQUIRED)
        message("Vorbis: [${Vorbis_FOUND}] ${Vorbis_INCLUDE_DIRS} ${Vorbis_LIBRARIES}")
        if(USE_OGG_VORBIS_DYNAMIC)
            message("Dynamic vorbis: ${VorbisDec_DYNAMIC_LIBRARY}")
            add_definitions(-DOGG_DYNAMIC=\"${VorbisDec_DYNAMIC_LIBRARY}\")
        endif()
    else()
        if(DOWNLOAD_AUDIO_CODECS_DEPENDENCY)
            set(Vorbis_LIBRARIES vorbisfile vorbis)
        else()
            find_library(LIBVORBISFILE_LIB NAMES vorbisfile
                         HINTS "${AUDIO_CODECS_INSTALL_PATH}/lib")
            find_library(LIBVORBIS_LIB NAMES vorbis
                         HINTS "${AUDIO_CODECS_INSTALL_PATH}/lib")
            set(Vorbis_LIBRARIES ${LIBVORBISFILE_LIB} ${LIBVORBIS_LIB})
        endif()
        set(Vorbis_FOUND 1)
        set(Vorbis_INCLUDE_DIRS
            ${AUDIO_CODECS_PATH}/libogg/include
            ${AUDIO_CODECS_PATH}/libvorbis/include
        )
    endif()

    if(Vorbis_FOUND)
        message("== using Vorbis ==")
        add_definitions(-DMUSIC_OGG)
        list(APPEND SDL_MIXER_INCLUDE_PATHS ${Vorbis_INCLUDE_DIRS})
        if(NOT USE_SYSTEM_AUDIO_LIBRARIES OR NOT USE_OGG_VORBIS_DYNAMIC)
            list(APPEND SDLMixerX_LINK_LIBS ${Vorbis_LIBRARIES})
            set(LIBOGG_NEEDED ON)
        endif()
        list(APPEND SDLMixerX_SOURCES
            ${SDLMixerX_SOURCE_DIR}/src/codecs/music_ogg.c)
    endif()
endif()
