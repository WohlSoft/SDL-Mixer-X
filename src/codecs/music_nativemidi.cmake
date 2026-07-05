 # Native MIDI correctly works on Windows and macOS only.
if((WIN32 AND NOT EMSCRIPTEN) OR (APPLE AND CMAKE_HOST_SYSTEM_VERSION VERSION_GREATER 10 AND NOT IOS) OR LINUX)
    set(NATIVE_MIDI_SUPPORTED ON)
else()
    set(NATIVE_MIDI_SUPPORTED OFF)
endif()

option(USE_MIDI_NATIVE     "Build with operating system native MIDI output support" ${NATIVE_MIDI_SUPPORTED})
if(USE_MIDI_NATIVE AND NOT USE_MIDI_NATIVE_ALT)
    if(LINUX)
        find_library(LIB_ASOUND asound)

        if(LIB_ASOUND)
            option(USE_MIDI_NATIVE_ALSA_DYNAMIC   "Build ALSA MIDI with the dynamic mode" ON)
            set(NATIVE_MIDI_FOUND TRUE)
            set(NATIVE_MIDI_LIBS ${LIB_ASOUND})
        else()
            set(NATIVE_MIDI_FOUND FALSE)
        endif()
    else()
        set(NATIVE_MIDI_FOUND TRUE)  # Always true
    endif()

    if(NATIVE_MIDI_FOUND)
        list(APPEND SDL_MIXER_DEFINITIONS -DMUSIC_MID_NATIVE)
        list(APPEND SDLMixerX_SOURCES
            ${CMAKE_CURRENT_LIST_DIR}/music_nativemidi.c
            ${CMAKE_CURRENT_LIST_DIR}/native_midi/native_midi_common.c
            ${CMAKE_CURRENT_LIST_DIR}/music_nativemidi.h
        )
        if(WIN32)
            message("== using Native MIDI for Win32 ==")
            list(APPEND SDLMixerX_SOURCES
                ${CMAKE_CURRENT_LIST_DIR}/native_midi/native_midi_win32.c)
            list(APPEND SDLMixerX_LINK_LIBS winmm)
            set(NATIVE_MIDI_LIBS winmm)
        endif()
        if(APPLE)
            message("== using Native MIDI for macOS ==")
            list(APPEND SDLMixerX_SOURCES
                ${CMAKE_CURRENT_LIST_DIR}/native_midi/native_midi_macosx.c)
        endif()

        if(LINUX)
            message("== using Native MIDI for Linux ALSA ==")

            if(USE_MIDI_NATIVE_ALSA_DYNAMIC)
                list(APPEND SDL_MIXER_DEFINITIONS -DSDL_NATIVE_MIDI_ALSA_DYNAMIC=\"${LIB_ASOUND}\")
            endif()

            list(APPEND SDLMixerX_SOURCES
                ${CMAKE_CURRENT_LIST_DIR}/native_midi/native_midi_linux_alsa.c)
            list(APPEND SDLMixerX_LINK_LIBS asound)

            if(DEFINED FLAG_C99)
                set_source_files_properties("${CMAKE_CURRENT_LIST_DIR}/native_midi/native_midi_linux_alsa.c"
                    COMPILE_FLAGS ${FLAG_C99}
                )
            endif()
        endif()
        appendMidiFormats("MIDI;RIFF MIDI")
    else()
        message("-- skipping Native MIDI --")
    endif()
endif()
