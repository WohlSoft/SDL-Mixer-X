option(USE_GME             "Build with Game Music Emulators library" ON)
if(USE_GME AND NOT SDL_MIXER_CLEAR_FOR_ZLIB_LICENSE AND NOT SDL_MIXER_CLEAR_FOR_LGPL_LICENSE)
    option(USE_GME_DYNAMIC "Use dynamical loading of Game Music Emulators library" OFF)

    if(USE_SYSTEM_AUDIO_LIBRARIES)
        find_package(GME QUIET)
        message("GME: [${GME_FOUND}] ${GME_INCLUDE_DIRS} ${GME_LIBRARIES}")
        if(USE_GME_DYNAMIC)
            list(APPEND SDL_MIXER_DEFINITIONS -DGME_DYNAMIC=\"${GME_DYNAMIC_LIBRARY}\")
            message("Dynamic GME: ${GME_DYNAMIC_LIBRARY}")
        endif()

        cpp_needed(${SDLMixerX_SOURCE_DIR}/cmake/tests/cpp_needed/gme.c
            ""
            ${GME_INCLUDE_DIRS}
            "${GME_LIBRARIES};${M_LIBRARY}"
            STDCPP_NEEDED
        )

        try_compile(GME_HAS_SET_AUTOLOAD_PLAYBACK_LIMIT
            ${CMAKE_BINARY_DIR}/compile_tests
            ${SDLMixerX_SOURCE_DIR}/cmake/tests/gme_has_gme_set_autoload_playback_limit.c
            LINK_LIBRARIES ${GME_LIBRARIES} ${STDCPP_LIBRARY} ${M_LIBRARY}
            CMAKE_FLAGS "-DINCLUDE_DIRECTORIES=${GME_INCLUDE_DIRS}"
            OUTPUT_VARIABLE GME_TEST_RESULT
        )
        message("gme_set_autoload_playback_limit() compile test result: ${GME_HAS_SET_AUTOLOAD_PLAYBACK_LIMIT}")
    else()
        if(DOWNLOAD_AUDIO_CODECS_DEPENDENCY)
            set(LIBGME_LIB gme)
            set(LIBZLIB_LIB zlib)
        else()
            find_library(LIBGME_LIB NAMES gme
                         HINTS "${AUDIO_CODECS_INSTALL_PATH}/lib")
            find_library(LIBZLIB_LIB NAMES zlib
                         HINTS "${AUDIO_CODECS_INSTALL_PATH}/lib")
        endif()
        mark_as_advanced(LIBGME_LIB LIBZLIB_LIB)
        set(GME_LIBRARIES ${LIBGME_LIB} ${LIBZLIB_LIB})
        set(GME_FOUND 1)
        set(STDCPP_NEEDED 1) # Statically linking GME which is C++ library
        set(LIBMATH_NEEDED 1)
        set(GME_HAS_SET_AUTOLOAD_PLAYBACK_LIMIT TRUE)
        set(GME_INCLUDE_DIRS
            ${AUDIO_CODECS_INSTALL_PATH}/include/gme
            ${AUDIO_CODECS_INSTALL_PATH}/include
            ${AUDIO_CODECS_PATH}/libgme/include
            ${AUDIO_CODECS_PATH}/zlib/include
        )
    endif()

    if(GME_FOUND)
        message("== using GME (LGPLv2.1+ or GPLv2+ if MAME YM2612 emulator was used) ==")
        if(DOWNLOAD_AUDIO_CODECS_DEPENDENCY)
            setLicense(LICENSE_GPL_2p)  # at AudioCodecs set, the MAME YM2612 emualtor is enabled by default
        else()
            setLicense(LICENSE_LGPL_2_1p)
        endif()
        list(APPEND SDL_MIXER_DEFINITIONS -DMUSIC_GME)
        list(APPEND SDL_MIXER_INCLUDE_PATHS ${GME_INCLUDE_DIRS})
        if(NOT USE_SYSTEM_AUDIO_LIBRARIES OR NOT USE_GME_DYNAMIC)
            list(APPEND SDLMixerX_LINK_LIBS ${GME_LIBRARIES})
        endif()
        list(APPEND SDLMixerX_SOURCES
            ${CMAKE_CURRENT_LIST_DIR}/music_gme.c)
        if(GME_HAS_SET_AUTOLOAD_PLAYBACK_LIMIT)
            list(APPEND SDL_MIXER_DEFINITIONS -DGME_HAS_SET_AUTOLOAD_PLAYBACK_LIMIT)
        endif()
    else()
        message("-- skipping GME --")
    endif()
endif()
