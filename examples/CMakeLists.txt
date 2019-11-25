
find_package(SDL2 REQUIRED)

add_library(libSDLMixerX INTERFACE)
if(SDL_MIXER_X_SHARED)
    target_link_libraries(libSDLMixerX INTERFACE SDL2_mixer_ext)
elseif(SDL_MIXER_X_STATIC)
    target_link_libraries(libSDLMixerX INTERFACE SDL2_mixer_ext_Static)
endif()

add_executable(playmus playmus.c)
target_link_libraries(playmus PUBLIC libSDLMixerX ${SDL2_LIBRARIES})

add_executable(playwave playwave.c)
target_link_libraries(playwave PUBLIC libSDLMixerX ${SDL2_LIBRARIES})
