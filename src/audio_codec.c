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

Uint32 music_interface_default_capabilities()
{
    return ACODEC_NOCAPS;
}

void *music_interface_dummy_cb_open(SDL_RWops *src, int freesrc)
{
    (void)src; (void)freesrc;
    return NULL;
}

void *music_interface_dummy_cb_openEx(SDL_RWops *src, int freesrc, const char *extraSettings)
{
    (void)src; (void)freesrc; (void)extraSettings;
    return NULL;
}

void music_interface_dummy_cb_void_1arg(Mix_MusicInterfaceStream* music)
{
    (void)music;
}

int  music_interface_dummy_cb_int_1arg(Mix_MusicInterfaceStream* music)
{
    (void)music;
    return 0;
}

const char *music_interface_dummy_meta_tag(Mix_MusicInterfaceStream* music)
{
    (void)music;
    return "";
}

void music_interface_dummy_cb_seek(Mix_MusicInterfaceStream* music, double position)
{
    (void)music;
    (void)position;
}

double music_interface_dummy_cb_tell(Mix_MusicInterfaceStream* music)
{
    (void)music;
    return -1.0;
}

void music_interface_dummy_cb_regulator(Mix_MusicInterfaceStream *music, int value)
{
    (void)music;
    (void)value;
}

int music_interface_dummy_playAudio(Mix_MusicInterfaceStream *music, Uint8 *data, int length)
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

    interface->capabilities     = music_interface_default_capabilities;

    interface->open             = music_interface_dummy_cb_open;
    interface->openEx           = music_interface_dummy_cb_openEx;
    interface->close            = music_interface_dummy_cb_void_1arg;

    interface->play             = music_interface_dummy_cb_void_1arg;
    interface->pause            = music_interface_dummy_cb_void_1arg;
    interface->resume           = music_interface_dummy_cb_void_1arg;
    interface->stop             = music_interface_dummy_cb_void_1arg;

    interface->isPlaying        = music_interface_dummy_cb_int_1arg;
    interface->isPaused         = music_interface_dummy_cb_int_1arg;

    interface->setLoops         = music_interface_dummy_cb_regulator;
    interface->setVolume        = music_interface_dummy_cb_regulator;

    interface->jumpToTime       = music_interface_dummy_cb_seek;
    interface->getCurrentTime   = music_interface_dummy_cb_tell;
    interface->getTimeLength    = music_interface_dummy_cb_tell;

    interface->getLoopStartTime = music_interface_dummy_cb_tell;
    interface->getLoopEndTime   = music_interface_dummy_cb_tell;
    interface->getLoopLengthTime= music_interface_dummy_cb_tell;

    interface->metaTitle        = music_interface_dummy_meta_tag;
    interface->metaArtist       = music_interface_dummy_meta_tag;
    interface->metaAlbum        = music_interface_dummy_meta_tag;
    interface->metaCopyright    = music_interface_dummy_meta_tag;

    interface->playAudio        = music_interface_dummy_playAudio;
}
