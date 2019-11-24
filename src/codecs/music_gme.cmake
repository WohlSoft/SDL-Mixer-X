option(USE_GME             "Build with Game Music Emulators library" ON)
if(USE_GME)
    option(USE_GME_DYNAMIC "Use dynamical loading of Game Music Emulators library" OFF)

    if(USE_SYSTEM_AUDIO_LIBRARIES)
        find_package(GME)
        message("GME: [${GME_FOUND}] ${GME_INCLUDE_DIRS} ${GME_LIBRARIES}")
        if(USE_GME_DYNAMIC)
            add_definitions(-DGME_DYNAMIC=\"${GME_DYNAMIC_LIBRARY}\")
            message("Dynamic GME: ${GME_DYNAMIC_LIBRARY}")
        endif()
    else()
        if(DOWNLOAD_AUDIO_CODECS_DEPENDENCY)
            set(LIBGME_LIB gme)
            set(LIBZLIB_LIB zlib)
        else()
            find_library(LIBGME_LIB NAMES gme
                         HINTS "${AUDIO_CODECS_INSTALL_PATH}/lib")
            find_library(LIBZLIB_LIB NAMES zlib z
                         HINTS "${AUDIO_CODECS_INSTALL_PATH}/lib")
        endif()
        set(GME_LIBRARIES ${LIBGME_LIB} ${LIBZLIB_LIB})
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
        if(NOT USE_SYSTEM_AUDIO_LIBRARIES OR NOT USE_GME_DYNAMIC)
            list(APPEND SDLMixerX_LINK_LIBS ${GME_LIBRARIES})
        endif()
        list(APPEND SDLMixerX_SOURCES
            ${CMAKE_CURRENT_LIST_DIR}/music_gme.c)
    endif()
endif()
