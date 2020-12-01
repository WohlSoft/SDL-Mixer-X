if(UNIX) # CMD Music is not supported on Windows
    option(USE_CMD             "Build with CMD music player support" ON)
    if(USE_CMD)
        message("== using CMD Music (ZLib) ==")
        list(APPEND SDL_MIXER_DEFINITIONS -DMUSIC_CMD -D_POSIX_C_SOURCE=1)
        CHECK_FUNCTION_EXISTS(fork HAVE_FORK)
        if(HAVE_FORK)
            list(APPEND SDL_MIXER_DEFINITIONS -DHAVE_FORK)
        endif()
        list(APPEND SDLMixerX_SOURCES
            ${CMAKE_CURRENT_LIST_DIR}/music_cmd.c)
    endif()
endif()
