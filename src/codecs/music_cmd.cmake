if(UNIX) # CMD Music is not supported on Windows
    option(USE_CMD             "Build with CMD music player support" ON)
    if(USE_CMD)
        message("== using CMD Music (ZLib) ==")
        list(APPEND SDL_MIXER_DEFINITIONS -DMUSIC_CMD -D_POSIX_C_SOURCE=1)

        set(fork_found OFF)
        if(NOT fork_found)
            check_symbol_exists(fork sys/unistd.h HAVE_FORK)
            if(HAVE_FORK)
                set(fork_found ON)
                list(APPEND SDL_MIXER_DEFINITIONS -DHAVE_FORK)
            endif()
        endif()

        if(NOT fork_found)
            check_symbol_exists(fork sys/unistd.h HAVE_VFORK)
            if(HAVE_VFORK)
                set(fork_found ON)
                list(APPEND SDL_MIXER_DEFINITIONS -DHAVE_VFORK)
            endif()
        endif()

        if(NOT fork_found)
            message(FATAL_ERROR "Neither fork() or vfork() or available on this platform. Reconfigure with -DUSE_CMD=OFF.")
        endif()

        list(APPEND SDLMixerX_SOURCES
            ${CMAKE_CURRENT_LIST_DIR}/music_cmd.c
            ${CMAKE_CURRENT_LIST_DIR}/music_cmd.h
        )
        appendOtherFormats("CMD Music")
    endif()
endif()
