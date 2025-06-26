option(USE_QOA   "Build with Quite OK Audio (QOA) support" ON)
if(USE_QOA)
    message("== using QOA (MIT license) ==")

    list(APPEND SDLMixerX_SOURCES
        ${CMAKE_CURRENT_LIST_DIR}/qoa/qoa.h
    )
    list(APPEND SDL_MIXER_DEFINITIONS -DMUSIC_QOA)

    list(APPEND SDLMixerX_SOURCES
        ${CMAKE_CURRENT_LIST_DIR}/music_qoa.c
        ${CMAKE_CURRENT_LIST_DIR}/music_qoa.h
    )

    if(DEFINED FLAG_C99)
        set_source_files_properties("${CMAKE_CURRENT_LIST_DIR}/music_qoa.c"
            COMPILE_FLAGS ${FLAG_C99}
        )
    endif()

    appendPcmFormats("QOA;XQOA")
endif()
