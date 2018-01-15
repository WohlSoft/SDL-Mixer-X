cmake_minimum_required (VERSION 2.8.11)
project(SDLMixerMusPlayer)

# Tell CMake to run moc when necessary:
set(CMAKE_AUTOMOC ON)

# As moc files are generated in the binary dir, tell CMake
# to always look for includes there:
set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(USE_SDL_MIXER_X_PREFIX "" CACHE STRING "Path to SDL Mixer X")

find_package(Qt5Widgets REQUIRED)
find_package(Qt5Network REQUIRED)

if(APPLE)
    find_library(COREAUDIO_LIBRARY CoreAudio)
    find_library(COREVIDEO_LIBRARY CoreVideo)
    find_library(IOKIT_LIBRARY IOKit)
    find_library(CARBON_LIBRARY Carbon)
    find_library(COCOA_LIBRARY Cocoa)
    find_library(FORCEFEEDBAK_LIBRARY ForceFeedback)
    find_library(METAL_LIBRARY Metal)
    find_library(COREFOUNDATION_LIBRARY CoreFoundation)
    find_library(AUDIOTOOLBOX_LIBRARY AudioToolbox)
    add_definitions(-DQ_OS_MACX) # Workaround for MOC
endif()

# The Qt5Widgets_INCLUDES also includes the include directories for
# dependencies QtCore and QtGui
include_directories(
    ${Qt5Widgets_INCLUDES}
    ${Qt5Network_INCLUDES}
    ${USE_SDL_MIXER_X_PREFIX}/include)

link_directories(
    ${Qt5Widgets_LIBRARIES}
    ${Qt5Network_LIBRARIES}
    ${USE_SDL_MIXER_X_PREFIX}/lib)

# We need add -DQT_WIDGETS_LIB when using QtWidgets in Qt 5.
add_definitions(${Qt5Widgets_DEFINITIONS} ${Qt5Network_DEFINITIONS})

# Executables fail to build with Qt 5 in the default configuration
# without -fPIE. We add that here.
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${Qt5Widgets_EXECUTABLE_COMPILE_FLAGS} ${Qt5Network_EXECUTABLE_COMPILE_FLAGS}")

qt5_wrap_ui(UIS_HDRS
    MainWindow/mainwindow.ui
    AssocFiles/assoc_files.ui
)

set(RESOURCE ${CMAKE_SOURCE_DIR}/_resources/musicplayer.qrc)
qt5_add_resources(RESOURCE_ADDED ${RESOURCE})

add_executable(sdlmixer_musplay
    AssocFiles/assoc_files.cpp
    Effects/reverb.cpp
    MainWindow/musplayer_base.cpp
    MainWindow/musplayer_qt.cpp
    Player/mus_player.cpp
    SingleApplication/localserver.cpp
    SingleApplication/pge_application.cpp
    SingleApplication/singleapplication.cpp
    main.cpp
    wave_writer.c
    ${UIS_HDRS}
    ${RESOURCE_ADDED}
)

if(WIN32)
    target_link_libraries(sdlmixer_musplay SDL2main version SDL2_mixer_ext SDL2 comctl32)
elseif(APPLE)
    target_link_libraries(sdlmixer_musplay SDL2 SDL2_mixer_ext iconv
        ${COREAUDIO_LIBRARY} ${COREVIDEO_LIBRARY} ${COCOA_LIBRARY} ${CARBON_LIBRARY}
        ${FORCEFEEDBAK_LIBRARY} ${IOKIT_LIBRARY} ${COREFOUNDATION_LIBRARY} ${AUDIOTOOLBOX_LIBRARY}
        ${METAL_LIBRARY}
    )
else()
    target_link_libraries(sdlmixer_musplay SDL2 SDL2_mixer_ext)
endif()

qt5_use_modules(sdlmixer_musplay Widgets Network)
