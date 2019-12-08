#include "mus_player.h"

#include <QMessageBox>
#include "../MainWindow/musplayer_qt.h"

#include "../wave_writer.h"

/*!
 *  SDL Mixer wrapper
 */
namespace PGE_MusicPlayer
{
    Mix_Music *s_playMus = nullptr;
    Mix_MusicType type  = MUS_NONE;
    bool reverbEnabled = false;

    static bool g_playlistMode = false;
    static int  g_loopsCount = -1;
    static bool g_hooksDisabled = false;

    static void musicStoppedHook();

    void initHooks()
    {
        Mix_HookMusicFinished(&musicStoppedHook);
    }

    static MusPlayer_Qt *mw = nullptr;
    void setMainWindow(void *mwp)
    {
        mw = reinterpret_cast<MusPlayer_Qt*>(mwp);
    }

    const char *musicTypeC()
    {
        return (
                   type == MUS_NONE ? "No Music" :
                   type == MUS_CMD ? "CMD" :
                   type == MUS_WAV ? "PCM Wave" :
                   type == MUS_MOD ? "Tracker music" :
                   type == MUS_MID ? "MIDI" :
                   type == MUS_OGG ? "OGG" :
                   type == MUS_MP3 ? "MP3" :
                   type == MUS_FLAC ? "FLAC" :
#ifdef SDL_MIXER_X
#   if SDL_MIXER_MAJOR_VERSION > 2 || \
    (SDL_MIXER_MAJOR_VERSION == 2 && SDL_MIXER_MINOR_VERSION >= 2)
                   type == MUS_OPUS ? "OPUS" :
#   endif
                   type == MUS_ADLMIDI ? "IMF/MUS/XMI" :
                   type == MUS_GME ? "Game Music Emulator" :
#else
#   if SDL_MIXER_MAJOR_VERSION > 2 || \
    (SDL_MIXER_MAJOR_VERSION == 2 && SDL_MIXER_MINOR_VERSION > 0) || \
    (SDL_MIXER_MAJOR_VERSION == 2 && SDL_MIXER_MINOR_VERSION == 0 && SDL_MIXER_PATCHLEVEL >= 4)
                   type == MUS_OPUS ? "OPUS" :
#   endif
#endif
                   "<Unknown>");
    }
    QString musicType()
    {
        return QString(musicTypeC());
    }

    /*!
     * \brief Spawn warning message box with specific text
     * \param msg text to spawn in message box
     */
    static void error(QString msg)
    {
        QMessageBox::warning(nullptr,
                             "SDL2 Mixer ext error",
                             msg,
                             QMessageBox::Ok);
    }

    void stopMusic()
    {
        Mix_HaltMusicStream(PGE_MusicPlayer::s_playMus);
    }

    void disableHooks()
    {
        g_hooksDisabled = true;
    }

    void enableHooks()
    {
        g_hooksDisabled = false;
    }

    QString getMusTitle()
    {
        if(s_playMus)
            return QString(Mix_GetMusicTitle(s_playMus));
        else
            return QString("[No music]");
    }

    QString getMusArtist()
    {
        if(s_playMus)
            return QString(Mix_GetMusicArtistTag(s_playMus));
        else
            return QString("[Unknown Artist]");
    }

    QString getMusAlbum()
    {
        if(s_playMus)
            return QString(Mix_GetMusicAlbumTag(s_playMus));
        else
            return QString("[Unknown Album]");
    }

    QString getMusCopy()
    {
        if(s_playMus)
            return QString(Mix_GetMusicCopyrightTag(s_playMus));
        else
            return QString("");
    }

    static void musicStoppedHook()
    {
        if(g_hooksDisabled)
            return;
        if(mw)
            QMetaObject::invokeMethod(mw, "musicStopped", Qt::QueuedConnection);
    }

    void setMusicLoops(int loops)
    {
        g_loopsCount = loops;
    }

    bool playMusic()
    {
        if(s_playMus)
        {
            if(Mix_PlayMusic(s_playMus, g_playlistMode ? 0 : g_loopsCount) == -1)
            {
                error(QString("Mix_PlayMusic: ") + Mix_GetError());
                return false;
                // well, there's no music, but most games don't break without music...
            }
        }
        else
        {
            error(QString("Play nothing: Mix_PlayMusic: ") + Mix_GetError());
            return false;
        }
        return true;
    }

    void changeVolume(int volume)
    {
        Mix_VolumeMusicStream(PGE_MusicPlayer::s_playMus, volume);
        DebugLog(QString("Mix_VolumeMusic: %1\n").arg(volume));
    }

    bool openFile(QString musFile)
    {
        type = MUS_NONE;
        if(s_playMus != nullptr)
        {
            Mix_FreeMusic(s_playMus);
            s_playMus = nullptr;
        }

        QByteArray p = musFile.toUtf8();
        s_playMus = Mix_LoadMUS(p.data());
        if(!s_playMus)
        {
            error(QString("Mix_LoadMUS(\"" + QString(musFile) + "\"): ") + Mix_GetError());
            return false;
        }
        type = Mix_GetMusicType(s_playMus);
        DebugLog(QString("Music type: %1").arg(musicType()));
        return true;
    }




    static void* s_wavCtx = nullptr;

    // make a music play function
    // it expects udata to be a pointer to an int
    static void myMusicPlayer(void *udata, Uint8 *stream, int len)
    {
        ctx_wave_write(udata, reinterpret_cast<const short *>(stream), len / 2);
    }

    void startWavRecording(QString target)
    {
        if(s_wavCtx)
            return;
        if(!s_playMus)
            return;

        /* Record 20 seconds to wave file */
        s_wavCtx = ctx_wave_open(44100, target.toLocal8Bit().data());
        ctx_wave_enable_stereo(s_wavCtx);
        Mix_SetPostMix(myMusicPlayer, s_wavCtx);
    }

    void stopWavRecording()
    {
        if(!s_wavCtx)
            return;
        ctx_wave_close(s_wavCtx);
        s_wavCtx = nullptr;
        Mix_SetPostMix(nullptr, nullptr);
    }

    bool isWavRecordingWorks()
    {
        return (s_wavCtx != nullptr);
    }
}
