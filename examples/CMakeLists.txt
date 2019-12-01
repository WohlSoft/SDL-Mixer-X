
if(USE_SYSTEM_AUDIO_LIBRARIES OR USE_SYSTEM_SDL2)
    find_package(SDL2 REQUIRED)
    if(WIN32)
        find_library(SDL2main_LIBRARY NAMES SDL2main${MIX_DEBUG_SUFFIX} SDL2main)
        string(REGEX REPLACE "-mwindows" "" SDL2_LIBRARIES "${SDL2_LIBRARIES}")
        string(REGEX REPLACE "-lmingw32" "" SDL2_LIBRARIES "${SDL2_LIBRARIES}")
        string(STRIP "${SDL2_LIBRARIES}" SDL2_LIBRARIES)
    endif()
else()
    if(DOWNLOAD_AUDIO_CODECS_DEPENDENCY)
        if(WIN32)
            set(SDL2_LIBRARIES SDL2${MIX_DEBUG_SUFFIX} SDL2main${MIX_DEBUG_SUFFIX})
            set(SDL2main_LIBRARY SDL2main${MIX_DEBUG_SUFFIX})
        else()
            set(SDL2_LIBRARIES SDL2${MIX_DEBUG_SUFFIX})
        endif()
    else()
        set(CMAKE_FIND_LIBRARY_SUFFIXES ${FIND_PREFER_SHARED})
        find_library(SDL2_LIBRARY NAMES SDL2 SDL2d
                     HINTS "${AUDIO_CODECS_INSTALL_PATH}/lib")
        mark_as_advanced(SDL2_LIBRARY)
        set(SDL2_LIBRARIES ${SDL2_LIBRARY})
        if(WIN32)
            find_library(SDL2main_LIBRARY NAMES SDL2main SDL2maind
                        HINTS "${AUDIO_CODECS_INSTALL_PATH}/lib")
            mark_as_advanced(SDL2main_LIBRARY)
        endif()
    endif()
    set(SDL2_INCLUDE_DIRS ${AUDIO_CODECS_INSTALL_DIR}/include/SDL2)
endif()

message("Examples: SDL2 [${SDL2_LIBRARIES}] SDL2main: [${SDL2main_LIBRARY}]")

add_library(libSDLMixerX INTERFACE)
if(MINGW)
    target_link_libraries(libSDLMixerX INTERFACE -lmingw32 ${SDL2main_LIBRARY})
endif()
if(SDL_MIXER_X_SHARED)
    target_link_libraries(libSDLMixerX INTERFACE SDL2_mixer_ext ${SDL2_LIBRARIES})
elseif(SDL_MIXER_X_STATIC)
    target_link_libraries(libSDLMixerX INTERFACE SDL2_mixer_ext_Static ${SDL2_LIBRARIES})
endif()

include(NoPie)

function(console_app ${_target})
    if(WIN32)
        if(MSVC)
            set_target_properties(${_target} PROPERTIES LINK_FLAGS /SUBSYSTEM:CONSOLE)
        else()
            set_target_properties(${_target} PROPERTIES LINK_FLAGS -mconsole)
        endif()
    endif()
endfunction()


add_executable(playmus playmus.c)
pge_set_nopie(playmus)
target_include_directories(playmus PRIVATE ${SDL2_INCLUDE_DIRS})
target_link_libraries(playmus PRIVATE libSDLMixerX)
console_app(playmus)


add_executable(playwave playwave.c)
pge_set_nopie(playwave)
target_include_directories(playwave PRIVATE ${SDL2_INCLUDE_DIRS})
target_link_libraries(playwave PRIVATE libSDLMixerX)
console_app(playwave)

set(PLAYWAVE_TEST_MACROS)

option(PLAYWAVE_TEST_DECODERS "PlayWave: Enable Decoders test" OFF)
if(PLAYWAVE_TEST_DECODERS)
    list(APPEND PLAYWAVE_TEST_MACROS -DTEST_MIX_DECODERS)
endif()

option(PLAYWAVE_TEST_CHANNELFINISHED "PlayWave: Enable Channel-Finished test" OFF)
if(PLAYWAVE_TEST_CHANNELFINISHED)
    list(APPEND PLAYWAVE_TEST_MACROS -DTEST_MIX_CHANNELFINISHED)
endif()

option(PLAYWAVE_TEST_PANNING "PlayWave: Enable panning test" OFF)
if(PLAYWAVE_TEST_PANNING)
    list(APPEND PLAYWAVE_TEST_MACROS -DTEST_MIX_PANNING)
endif()

option(PLAYWAVE_TEST_VERSIONS "PlayWave: Enable versions test" OFF)
if(PLAYWAVE_TEST_VERSIONS)
    list(APPEND PLAYWAVE_TEST_MACROS -DTEST_MIX_VERSIONS)
endif()

option(PLAYWAVE_TEST_DISTANCE "PlayWave: Enable distance test" OFF)
if(PLAYWAVE_TEST_DISTANCE)
    list(APPEND PLAYWAVE_TEST_MACROS -DTEST_MIX_DISTANCE)
endif()

option(PLAYWAVE_TEST_POSITION "PlayWave: Enable position test" OFF)
if(PLAYWAVE_TEST_POSITION)
    list(APPEND PLAYWAVE_TEST_MACROS -DTEST_MIX_POSITION)
endif()

target_compile_definitions(playwave PRIVATE ${PLAYWAVE_TEST_MACROS})



option(BUILD_EXAMPLES_MUSPLAY_QT "Build MusPlay tool example (Qt5 Required)" OFF)
if(BUILD_EXAMPLES_MUSPLAY_QT)
    add_subdirectory(MusPlay-Qt)
endif()
