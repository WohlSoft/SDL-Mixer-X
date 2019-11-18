if(NOT WIN32 AND NOT EMSCRIPTEN) # CMD Music is not supported on Windows
    option(USE_CMD             "Build with CMD music player support" ON)
    if(USE_CMD)
        message("== using CMD Music ==")
        add_definitions(-DMUSIC_CMD -D_POSIX_C_SOURCE=1)
        CHECK_FUNCTION_EXISTS(fork HAVE_FORK)
        if(HAVE_FORK)
            add_definitions(-DHAVE_FORK)
        endif()
        list(APPEND SDLMixerX_SOURCES
            ${SDLMixerX_SOURCE_DIR}/src/codecs/music_cmd.c)
    endif()
endif()
