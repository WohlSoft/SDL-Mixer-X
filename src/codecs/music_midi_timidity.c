/*
  SDL_mixer:  An audio mixer library based on the SDL library
  Copyright (C) 1997-2017 Sam Lantinga <slouken@libsdl.org>

  This software is provided 'as-is', without any express or implied
  warranty.  In no event will the authors be held liable for any damages
  arising from the use of this software.

  Permission is granted to anyone to use this software for any purpose,
  including commercial applications, and to alter it and redistribute it
  freely, subject to the following restrictions:

  1. The origin of this software must not be misrepresented; you must not
     claim that you wrote the original software. If you use this software
     in a product, an acknowledgment in the product documentation would be
     appreciated but is not required.
  2. Altered source versions must be plainly marked as such, and must not be
     misrepresented as being the original software.
  3. This notice may not be removed or altered from any source distribution.
*/

#include "music_midi_timidity.h"
#include <timidity.h>

/* This is the format of the audio mixer data */
static SDL_AudioSpec music_spec;

typedef struct
{
    int play_count;
    int playing;
    MidiSong *song;
} TIMIDITY_Music;

static int TIMIDITY_Seek(void *context, double position);
static void TIMIDITY_Delete(void *context);

const char   *Timidity_Error(void)
{
    return "Error";
}

void         Timidity_Close(void)
{
    Timidity_Exit();
}

void *TIMIDITY_CreateFromRW(SDL_RWops *src, int freesrc)
{
    TIMIDITY_Music *music;
    SDL_AudioSpec spec;
    music = (TIMIDITY_Music *)SDL_calloc(1, sizeof(*music));
    if (!music) {
        SDL_OutOfMemory();
        return NULL;
    }

    music->playing = 0;
    SDL_memcpy(&spec, &music_spec, sizeof(spec));

    music->song = Timidity_LoadSong(src, &spec);
    if (!music->song) {
        TIMIDITY_Delete(music);
        return NULL;
    }

    if (freesrc) {
        SDL_RWclose(src);
    }
    return music;
}

static void TIMIDITY_SetVolume(void *context, int volume)
{
    TIMIDITY_Music *music = (TIMIDITY_Music *)context;
    Timidity_SetVolume(music->song, volume);
}

static void TIMIDITY_setloops(void *music_p, int loops)
{
    TIMIDITY_Music *music = (TIMIDITY_Music *)music_p;
    if(loops == 0)
        music->play_count = -1;
    music->play_count = (loops - 1);
}

static void TIMIDITY_Play(void *context)
{
    TIMIDITY_Music *music = (TIMIDITY_Music *)context;
    Timidity_Start(music->song);
    TIMIDITY_Seek(music, 0.0);
    music->play_count = -1;
    music->playing = 1;
}

/* Stop playback of a stream previously started with ADLMIDI_play() */
void TIMIDITY_Stop(void *music_p)
{
    TIMIDITY_Music *music = (TIMIDITY_Music *)music_p;
    if(music)
    {
        music->playing = 0;
    }
}

/* Return non-zero if a stream is currently playing */
int TIMIDITY_Active(void *music_p)
{
    TIMIDITY_Music *music = (TIMIDITY_Music *)music_p;
    if(music)
        return music->playing;
    else
        return 0;
}

static int TIMIDITY_GetSome(void *context, Uint8 *data, int bytes)
{
    TIMIDITY_Music *music = (TIMIDITY_Music *)context;
    int amount, expected;

    if (!music->play_count) {
        /* All done */
        return 0;
    }

    expected = bytes;
    amount = Timidity_PlaySome(music->song, data, bytes);

    if (amount < expected) {
        if (music->play_count == 1) {
            /* We didn't consume anything and we're done */
            music->play_count = 0;
        } else {
            int play_count = -1;
            if (music->play_count > 0) {
                play_count = (music->play_count - 1);
            }
            TIMIDITY_Play(music);
        }
    }

    return amount;
}

static int TIMIDITY_Seek(void *context, double position)
{
    TIMIDITY_Music *music = (TIMIDITY_Music *)context;
    Timidity_Seek(music->song, (Uint32)(position * 1000));
    return 0;
}

static Uint32 Timidity_capabilities()
{
    return ACODEC_SINGLETON;
}

static void TIMIDITY_Delete(void *music_p)
{
    TIMIDITY_Music *music = (TIMIDITY_Music *)music_p;
    if (music->song) {
        Timidity_FreeSong(music->song);
    }
    SDL_free(music);
}

void Timidity_addToPathList(const char* path)
{
    Timidity_AddConfigPath(path);
}

int Timidity_init2(Mix_MusicInterface *codec, SDL_AudioSpec *mixer)
{
    music_spec = *mixer;

    initMusicInterface(codec);

    codec->isValid = Timidity_Init() == 0 ? 1 : 0;

    codec->capabilities     = Timidity_capabilities;

    codec->open             = TIMIDITY_CreateFromRW;
    codec->openEx           = music_interface_dummy_cb_openEx;
    codec->close            = TIMIDITY_Delete;

    codec->play             = TIMIDITY_Play;
    codec->pause            = music_interface_dummy_cb_void_1arg;
    codec->resume           = music_interface_dummy_cb_void_1arg;
    codec->stop             = TIMIDITY_Stop;

    codec->isPlaying        = TIMIDITY_Active;
    codec->isPaused         = music_interface_dummy_cb_int_1arg;

    codec->setLoops         = TIMIDITY_setloops;
    codec->setVolume        = TIMIDITY_SetVolume;

    codec->jumpToTime       = music_interface_dummy_cb_seek;
    codec->getCurrentTime   = music_interface_dummy_cb_tell;
    codec->getTimeLength    = music_interface_dummy_cb_tell;

    codec->metaTitle        = music_interface_dummy_meta_tag;
    codec->metaArtist       = music_interface_dummy_meta_tag;
    codec->metaAlbum        = music_interface_dummy_meta_tag;
    codec->metaCopyright    = music_interface_dummy_meta_tag;

    codec->playAudio        = TIMIDITY_GetSome;

    return(codec->isValid ? 0 : -1);
}
