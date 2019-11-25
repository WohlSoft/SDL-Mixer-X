
find_package(SDL2 REQUIRED)

add_library(libSDLMixerX INTERFACE)
if(SDL_MIXER_X_SHARED)
    target_link_libraries(libSDLMixerX INTERFACE SDL2_mixer_ext)
elseif(SDL_MIXER_X_STATIC)
    target_link_libraries(libSDLMixerX INTERFACE SDL2_mixer_ext_Static)
endif()

add_executable(playmus playmus.c)
target_include_directories(playmus PRIVATE ${SDL2_INCLUDE_DIRS})
target_link_libraries(playmus PRIVATE libSDLMixerX ${SDL2_LIBRARIES})

add_executable(playwave playwave.c)
target_include_directories(playwave PRIVATE ${SDL2_INCLUDE_DIRS})
target_link_libraries(playwave PRIVATE libSDLMixerX ${SDL2_LIBRARIES})

option(BUILD_EXAMPLES_MUSPLAY_QT "Build MusPlay tool example (Qt5 Required)" OFF)
if(BUILD_EXAMPLES_MUSPLAY_QT)
    add_subdirectory(MusPlay-Qt)
endif()
