# C++-coded alternative MIDI-sequencer
if(CPP_MIDI_SEQUENCER_NEEDED)
    list(APPEND SDL_MIXER_DEFINITIONS -DMUSIC_MID_NATIVE -DMUSIC_MID_NATIVE_ALT)
    list(APPEND SDLMixerX_SOURCES
        ${CMAKE_CURRENT_LIST_DIR}/midi_seq/mix_midi_seq.cpp)
    set(STDCPP_NEEDED TRUE)
    # LGPL license because of XMI/MUS support modules. Disabling them will give MIT
    setLicense(LICENSE_LGPL_2_1p)
endif()
