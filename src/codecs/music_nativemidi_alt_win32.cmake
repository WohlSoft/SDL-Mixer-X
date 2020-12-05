# Alternative Native MIDI for Windows
if(WIN32 AND NOT EMSCRIPTEN)
    option(USE_MIDI_NATIVE_ALT	 "Build with an alternative operating system native MIDI output support" ON)
endif()
if(USE_MIDI_NATIVE_ALT AND WIN32)
    list(APPEND SDL_MIXER_DEFINITIONS -DMUSIC_MID_NATIVE -DMUSIC_MID_NATIVE_ALT)
    list(APPEND SDLMixerX_SOURCES
        ${CMAKE_CURRENT_LIST_DIR}/music_nativemidi_alt_win32.c
        ${CMAKE_CURRENT_LIST_DIR}/midi_seq/win32_seq.cpp)
    list(APPEND SDLMixerX_LINK_LIBS winmm)
    set(STDCPP_NEEDED TRUE)
    # LGPL license because of XMI/MUS support modules. Disabling them will give MIT
    setLicense(LICENSE_LGPL_2_1p)
endif()
