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

#ifndef AUDIO_CODEC_H
#define AUDIO_CODEC_H

#include <SDL2/SDL_types.h>
#include <SDL2/SDL_rwops.h>
#include <SDL2/SDL_audio.h>

typedef void Mix_MusicInterfaceStream;

typedef enum
{
    ACODEC_NOCAPS               = 0x00000000,/* Codec has no any special capabilities */

    ACODEC_ASYNC                = 0x00000001,/* Asyncronious player which plays audio into separately opened output
                                                (Examples: CMD-Music, when music playing by external tool,
                                                           Native-Midi, when music playing via external driver
                                                           provided by operating system)*/

    ACODEC_NEED_VOLUME_INIT     = 0x00000002,/* Need to additionally initialize the volume value before playing begin */

    ACODEC_NEED_VOLUME_INIT_POST= 0x00000004,/* Need to additionally initialize the volume value after playing began */

    ACODEC_SINGLETON            = 0x00000008,/* Codec is able to play only one song in same time.
                                                You can't play two or more concurrent tracks playing by this same codec
                                                (Examples: MikMod because of interface of library,
                                                           NativeMidi because one external device
                                                           is able to play only one MIDI file) */
    ACODEC_HAS_PAUSE            = 0x00000010 /* Codec has own pause state */
} AudioCodec_Caps;

/*
    Dummy callbacks. Use them if codec doesn't supports some features
*/
Uint32      audioCodec_default_capabilities();
void       *audioCodec_dummy_cb_open(SDL_RWops* src, int freesrc);
void       *audioCodec_dummy_cb_openEx(SDL_RWops* src, int freesrc, const char *extraSettings);
void        audioCodec_dummy_cb_void_1arg(Mix_MusicInterfaceStream* music);
int         audioCodec_dummy_cb_int_1arg(Mix_MusicInterfaceStream* music);
const char *audioCodec_dummy_meta_tag(Mix_MusicInterfaceStream* music);
void        audioCodec_dummy_cb_regulator(Mix_MusicInterfaceStream* music, int value);
void        audioCodec_dummy_cb_seek(Mix_MusicInterfaceStream* music, double position);
double      audioCodec_dummy_cb_tell(Mix_MusicInterfaceStream* music);
int         audioCodec_dummy_playAudio(Mix_MusicInterfaceStream* music, Uint8* data, int length);

/* A generic audio playing codec interface interface */
typedef struct
{
    int     isValid;

    /* Capabilities of the codec */
    Uint32 (*capabilities)(void);

    Mix_MusicInterfaceStream* (*open)(SDL_RWops* src, int freesrc);
    Mix_MusicInterfaceStream* (*openEx)(SDL_RWops* src, int freesrc, const char *extraSettings);
    void  (*close)(Mix_MusicInterfaceStream* music);

    void  (*play)(Mix_MusicInterfaceStream* music);
    void  (*pause)(Mix_MusicInterfaceStream* music);
    void  (*resume)(Mix_MusicInterfaceStream* music);
    void  (*stop)(Mix_MusicInterfaceStream* music);

    int   (*isPlaying)(Mix_MusicInterfaceStream* music);
    int   (*isPaused)(Mix_MusicInterfaceStream* music);

    void  (*setLoops)(Mix_MusicInterfaceStream* music, int loopsCount);
    void  (*setVolume)(Mix_MusicInterfaceStream* music, int volume);

    double (*getCurrentTime)(Mix_MusicInterfaceStream* music);
    void  (*jumpToTime)(Mix_MusicInterfaceStream* music, double position);
    double (*getTimeLength)(Mix_MusicInterfaceStream* music);

    double (*getLoopStartTime)(Mix_MusicInterfaceStream* music);
    double (*getLoopEndTime)(Mix_MusicInterfaceStream* music);
    double (*getLoopLengthTime)(Mix_MusicInterfaceStream* music);

    const char* (*metaTitle)(Mix_MusicInterfaceStream* music);
    const char* (*metaArtist)(Mix_MusicInterfaceStream* music);
    const char* (*metaAlbum)(Mix_MusicInterfaceStream* music);
    const char* (*metaCopyright)(Mix_MusicInterfaceStream* music);

    int   (*playAudio)(Mix_MusicInterfaceStream* music, Uint8* data, int length);

} Mix_MusicInterface;

/*
    Set all function pointers to dummy calls
 */
void initMusicInterface(Mix_MusicInterface *interface);

#endif /* AUDIO_CODEC_H*/

