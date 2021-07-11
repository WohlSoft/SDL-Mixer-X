# FIXME: Detect a version of SDL2 for pkg-config script

if(USE_SYSTEM_AUDIO_LIBRARIES OR USE_SYSTEM_SDL2)
    if(HAIKU OR VITA)
        find_library(SDL2_LIBRARY SDL2)
        find_path(SDL2_INCLUDE_DIR "SDL.h" PATH_SUFFIXES SDL2)

        if(NOT SDL2_INCLUDE_DIR)
            message("SDL2 include dir was not found.")
        endif()

        if(NOT SDL2_LIBRARY)
            message(FATAL_ERROR "The SDL2 Library was not found!")
        endif()

        set(SDL2_INCLUDE_DIRS ${SDL2_INCLUDE_DIR})
        set(SDL2_LIBRARIES ${SDL2_LIBRARY})
    else()
        find_package(SDL2 REQUIRED)
        if(TARGET SDL2::SDL2)
            set(SDL2_LIBRARIES SDL2::SDL2main SDL2::SDL2)
        endif()
    endif()
    set(LIBSDL2CUSTOM_LIB ${SDL2_LIBRARIES})
    set(LIBSDL2CUSTOM_INCLUDES ${SDL2_INCLUDE_DIRS})
    message("== SDL2: ${SDL2_INCLUDE_DIRS} ${SDL2_LIBRARIES} ==")

else()
    if(DOWNLOAD_AUDIO_CODECS_DEPENDENCY)
        set(LIBSDL2CUSTOM_LIB SDL2${MIX_DEBUG_SUFFIX})
    else()
        set(CMAKE_FIND_LIBRARY_SUFFIXES ${FIND_PREFER_SHARED})
        find_library(LIBSDL2CUSTOM_LIB NAMES SDL2
                     HINTS "${AUDIO_CODECS_INSTALL_PATH}/lib")
    endif()
    set(LIBSDL2CUSTOM_INCLUDES ${AUDIO_CODECS_INSTALL_DIR}/include/SDL2)

endif()

list(APPEND SDLMixerX_LINK_LIBS ${LIBSDL2CUSTOM_LIB})
list(APPEND SDL_MIXER_INCLUDE_PATHS ${LIBSDL2CUSTOM_INCLUDES})
