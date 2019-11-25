option(USE_OGG_VORBIS      "Build with OGG Vorbis codec" ON)
if(USE_OGG_VORBIS)
    option(USE_OGG_VORBIS_DYNAMIC "Use dynamical loading of OGG Vorbis" OFF)
    option(USE_OGG_VORBIS_TREMOR "Use the Tremor - an integer-only OGG Vorbis implementation" OFF)

    if(USE_SYSTEM_AUDIO_LIBRARIES)
        if(USE_OGG_VORBIS_TREMOR)
            find_package(Tremor QUIET)
            message("Tremor: [${Tremor_FOUND}] ${Tremor_INCLUDE_DIRS} ${Tremor_LIBRARIES}")
        else()
            find_package(Vorbis QUIET)
            message("Vorbis: [${Vorbis_FOUND}] ${Vorbis_INCLUDE_DIRS} ${Vorbis_LIBRARIES}")
        endif()

        if(USE_OGG_VORBIS_DYNAMIC)
            if(USE_OGG_VORBIS_TREMOR)
                add_definitions(-DOGG_DYNAMIC=\"${VorbisIDec_DYNAMIC_LIBRARY}\")
                message("Dynamic tremor: ${VorbisIDec_DYNAMIC_LIBRARY}")
            else()
                add_definitions(-DOGG_DYNAMIC=\"${VorbisFile_DYNAMIC_LIBRARY}\")
                message("Dynamic vorbis: ${VorbisFile_DYNAMIC_LIBRARY}")
            endif()
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
            ${AUDIO_CODECS_INSTALL_DIR}/include/vorbis
            ${AUDIO_CODECS_INSTALL_DIR}/include/ogg
            ${AUDIO_CODECS_PATH}/libogg/include
            ${AUDIO_CODECS_PATH}/libvorbis/include
        )
    endif()

    if(USE_OGG_VORBIS_TREMOR AND Tremor_FOUND)
        add_definitions(-DMUSIC_OGG -DOGG_USE_TREMOR)
        message("== using Tremor ==")
        list(APPEND SDL_MIXER_INCLUDE_PATHS ${Tremor_INCLUDE_DIRS})
        if(NOT USE_SYSTEM_AUDIO_LIBRARIES OR NOT USE_OGG_VORBIS_DYNAMIC)
            list(APPEND SDLMixerX_LINK_LIBS ${Tremor_LIBRARIES})
            set(LIBOGG_NEEDED ON)
        endif()
    elseif(Vorbis_FOUND)
        message("== using Vorbis ==")
        add_definitions(-DMUSIC_OGG)
        list(APPEND SDL_MIXER_INCLUDE_PATHS ${Vorbis_INCLUDE_DIRS})
        if(NOT USE_SYSTEM_AUDIO_LIBRARIES OR NOT USE_OGG_VORBIS_DYNAMIC)
            list(APPEND SDLMixerX_LINK_LIBS ${Vorbis_LIBRARIES})
            set(LIBOGG_NEEDED ON)
        endif()
    endif()

    if(NOT Vorbis_FOUND AND NOT Tremor_FOUND)
        message("-- skipping Vorbis/Tremor --")
    endif()

    if(Vorbis_FOUND OR Tremor_FOUND)
        list(APPEND SDLMixerX_SOURCES
            ${CMAKE_CURRENT_LIST_DIR}/music_ogg.c)
    endif()

endif()
