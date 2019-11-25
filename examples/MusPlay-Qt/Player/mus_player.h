#ifndef MUS_PLAYER_H
#define MUS_PLAYER_H

#define SDL_MAIN_HANDLED
#   include "SDL.h"
#ifdef USE_SDL_MIXER_X
#   include "SDL_mixer_ext.h"
#else
#   include "SDL_mixer.h"
#endif

#if (SDL_MIXER_MAJOR_VERSION > 2) || (SDL_MIXER_MAJOR_VERSION == 2 && SDL_MIXER_MINOR_VERSION >= 1)
#define SDL_MIXER_GE21
#endif

#ifdef MUSPLAY_USE_WINAPI
#define DebugLog(msg)
#include <assert.h>
#include "../defines.h"
#else
#include <QString>
#include <QDebug>
#define DebugLog(msg) qDebug() << msg;
#endif

#if !defined(SDL_MIXER_X) // Fallback for into raw SDL2_mixer
#   define Mix_PlayingMusicStream(music) Mix_PlayingMusic()
#   define Mix_PausedMusicStream(music) Mix_PausedMusic()
#   define Mix_ResumeMusicStream(music) Mix_ResumeMusic()
#   define Mix_PauseMusicStream(music) Mix_PauseMusic()
#   define Mix_HaltMusicStream(music) Mix_HaltMusic()
#   define Mix_VolumeMusicStream(music, volume) Mix_VolumeMusic(volume)

#   define Mix_SetMusicStreamPosition(music, value) Mix_SetMusicPosition(value)
#   define Mix_GetMusicTotalTime(music) -1.0
#   define Mix_GetMusicPosition(music) -1.0
#   define Mix_GetMusicLoopStartTime(music) -1.0
#   define Mix_GetMusicLoopEndTime(music) -1.0
#   define Mix_GetMusicTempo(music) -1.0

inline int Mix_PlayChannelTimedVolume(int which, Mix_Chunk *chunk, int loops, int ticks, int volume)
{
    int ret = Mix_PlayChannelTimed(which, chunk, loops, ticks);
    if(ret >= 0)
        Mix_Volume(which, volume);
    return ret;
}

inline int Mix_FadeInChannelTimedVolume(int which, Mix_Chunk *chunk, int loops, int ms, int ticks, int volume)
{
    int ret = Mix_FadeInChannelTimed(which, chunk, loops, ms, ticks);
    if(ret >= 0)
        Mix_Volume(which, volume);
    return ret;
}
#endif

namespace PGE_MusicPlayer
{
    extern Mix_Music *play_mus;
    extern Mix_MusicType type;
    extern bool reverbEnabled;
    extern void initHooks();
    extern void setMainWindow(void *mwp);
    extern const char* musicTypeC();
    extern QString musicType();
    extern void MUS_stopMusic();
#ifndef MUSPLAY_USE_WINAPI
    extern QString MUS_getMusTitle();
    extern QString MUS_getMusArtist();
    extern QString MUS_getMusAlbum();
    extern QString MUS_getMusCopy();
#else
    extern const char* MUS_getMusTitle();
    extern const char* MUS_getMusArtist();
    extern const char* MUS_getMusAlbum();
    extern const char* MUS_getMusCopy();
#endif
    extern void setPlayListMode(bool playList);
    extern void setMusicLoops(int loops);
    extern bool MUS_playMusic();
    extern void MUS_changeVolume(int volume);
    extern bool MUS_openFile(QString musFile);
    extern void startWavRecording(QString target);
    extern void stopWavRecording();
}

#endif // MUS_PLAYER_H
