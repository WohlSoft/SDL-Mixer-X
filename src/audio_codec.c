/*
  SDL_mixer:  An audio mixer library based on the SDL library
  Copyright (C) 1997-2017 Sam Lantinga <slouken@libsdl.org>

  SDL Mixer X: A fork of SDL_mixer audio mixing library
  Copyright (C) 2015-2017 Vitaly Novichkov <admin@wohlnet.ru>

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

#include "audio_codec.h"

Uint32 audioCodec_default_capabilities()
{
    return ACODEC_NOCAPS;
}

void *audioCodec_dummy_cb_open(SDL_RWops *src, int freesrc)
{
    (void)src; (void)freesrc;
    return NULL;
}

void *audioCodec_dummy_cb_openEx(SDL_RWops *src, int freesrc, const char *extraSettings)
{
    (void)src; (void)freesrc; (void)extraSettings;
    return NULL;
}

void audioCodec_dummy_cb_void_1arg(Mix_MusicInterfaceStream* music)
{
    (void)music;
}

int  audioCodec_dummy_cb_int_1arg(Mix_MusicInterfaceStream* music)
{
    (void)music;
    return 0;
}

const char *audioCodec_dummy_meta_tag(Mix_MusicInterfaceStream* music)
{
    (void)music;
    return "";
}

void audioCodec_dummy_cb_seek(Mix_MusicInterfaceStream* music, double position)
{
    (void)music;
    (void)position;
}

double audioCodec_dummy_cb_tell(Mix_MusicInterfaceStream* music)
{
    (void)music;
    return -1.0;
}

void audioCodec_dummy_cb_regulator(Mix_MusicInterfaceStream *music, int value)
{
    (void)music;
    (void)value;
}

int audioCodec_dummy_playAudio(Mix_MusicInterfaceStream *music, Uint8 *data, int length)
{
    (void)music;
    (void)data;
    (void)length;
    return 0;
}


void initMusicInterface(Mix_MusicInterface *interface)
{
    if(!interface)
        return;

    interface->isValid = 0;

    interface->capabilities     = audioCodec_default_capabilities;

    interface->open             = audioCodec_dummy_cb_open;
    interface->openEx           = audioCodec_dummy_cb_openEx;
    interface->close            = audioCodec_dummy_cb_void_1arg;

    interface->play             = audioCodec_dummy_cb_void_1arg;
    interface->pause            = audioCodec_dummy_cb_void_1arg;
    interface->resume           = audioCodec_dummy_cb_void_1arg;
    interface->stop             = audioCodec_dummy_cb_void_1arg;

    interface->isPlaying        = audioCodec_dummy_cb_int_1arg;
    interface->isPaused         = audioCodec_dummy_cb_int_1arg;

    interface->setLoops         = audioCodec_dummy_cb_regulator;
    interface->setVolume        = audioCodec_dummy_cb_regulator;

    interface->jumpToTime       = audioCodec_dummy_cb_seek;
    interface->getCurrentTime   = audioCodec_dummy_cb_tell;
    interface->getTimeLength    = audioCodec_dummy_cb_tell;

    interface->getLoopStartTime = audioCodec_dummy_cb_tell;
    interface->getLoopEndTime   = audioCodec_dummy_cb_tell;
    interface->getLoopLengthTime= audioCodec_dummy_cb_tell;

    interface->metaTitle        = audioCodec_dummy_meta_tag;
    interface->metaArtist       = audioCodec_dummy_meta_tag;
    interface->metaAlbum        = audioCodec_dummy_meta_tag;
    interface->metaCopyright    = audioCodec_dummy_meta_tag;

    interface->playAudio        = audioCodec_dummy_playAudio;
}
