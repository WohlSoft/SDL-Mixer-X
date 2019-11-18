option(USE_WAV "Build with WAV codec" ON)
if(USE_WAV)
    add_definitions(-DMUSIC_WAV)
    list(APPEND SDLMixerX_SOURCES
        ${SDLMixerX_SOURCE_DIR}/src/codecs/load_aiff.c
        ${SDLMixerX_SOURCE_DIR}/src/codecs/load_voc.c
        ${SDLMixerX_SOURCE_DIR}/src/codecs/music_wav.c)
endif()

