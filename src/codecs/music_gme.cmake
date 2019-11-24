option(USE_GME             "Build with Game Music Emulators library" ON)
if(USE_GME)

    if(USE_SYSTEM_AUDIO_LIBRARIES)
        find_package(GME)
        message("GME: [${GME_FOUND}] ${GME_INCLUDE_DIRS} ${GME_LIBRARIES}")
    else()
        if(DOWNLOAD_AUDIO_CODECS_DEPENDENCY)
            set(LIBGME_LIB gme)
            set(LIBZLIB_LIB zlib)
        else()
            find_library(LIBGME_LIB NAMES gme
                         HINTS "${AUDIO_CODECS_INSTALL_PATH}/lib")
            find_library(LIBZLIB_LIB NAMES zlib z
                         HINTS "${AUDIO_CODECS_INSTALL_PATH}/lib")
            set(GME_LIBRARIES ${LIBGME_LIB} ${LIBZLIB_LIB})
        endif()
        set(GME_FOUND 1)
        set(GME_INCLUDE_DIRS
            ${AUDIO_CODECS_PATH}/libgme/include
            ${AUDIO_CODECS_PATH}/zlib/include
        )
    endif()

    if(GME_FOUND)
        message("== using GME ==")
        add_definitions(-DMUSIC_GME)
        list(APPEND SDL_MIXER_INCLUDE_PATHS ${GME_INCLUDE_DIRS})
        list(APPEND SDLMixerX_LINK_LIBS ${GME_LIBRARIES})
        list(APPEND SDLMixerX_SOURCES
            ${CMAKE_CURRENT_LIST_DIR}/music_gme.c)
    endif()
endif()
