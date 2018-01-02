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
Uint32      music_interface_default_capabilities();
void       *music_interface_dummy_cb_open(SDL_RWops* src, int freesrc);
void       *music_interface_dummy_cb_openEx(SDL_RWops* src, int freesrc, const char *extraSettings);
void        music_interface_dummy_cb_void_1arg(Mix_MusicInterfaceStream* music);
int         music_interface_dummy_cb_int_1arg(Mix_MusicInterfaceStream* music);
const char *music_interface_dummy_meta_tag(Mix_MusicInterfaceStream* music);
void        music_interface_dummy_cb_regulator(Mix_MusicInterfaceStream* music, int value);
void        music_interface_dummy_cb_seek(Mix_MusicInterfaceStream* music, double position);
double      music_interface_dummy_cb_tell(Mix_MusicInterfaceStream* music);
int         music_interface_dummy_playAudio(Mix_MusicInterfaceStream* music, Uint8* data, int length);

/* A generic audio playing codec interface interface */
typedef struct
{
    int     isValid;

    /* Capabilities of the codec */
    Uint32 (*capabilities)(void);

    /* Create a music object from an SDL_RWops stream
     * If the function returns NULL, 'src' will be freed if needed by the caller.
     */
    Mix_MusicInterfaceStream* (*open)(SDL_RWops* src, int freesrc);
    Mix_MusicInterfaceStream* (*openEx)(SDL_RWops* src, int freesrc, const char *extraSettings);

    /* Create a music object from a file, if SDL_RWops are not supported */
    /* Mix_MusicInterfaceStream* (*CreateFromFile)(const char *file); */

    /* Close the library and clean up */
    void  (*close)(Mix_MusicInterfaceStream* music);

    /* Start playing music from the beginning with an optional loop count */
    void  (*play)(Mix_MusicInterfaceStream* music);

    void  (*pause)(Mix_MusicInterfaceStream* music);
    void  (*resume)(Mix_MusicInterfaceStream* music);
    void  (*stop)(Mix_MusicInterfaceStream* music);

    /* Returns SDL_TRUE if music is still playing */
    int   (*isPlaying)(Mix_MusicInterfaceStream* music);
    int   (*isPaused)(Mix_MusicInterfaceStream* music);

    /* Set the count of loops */
    void  (*setLoops)(Mix_MusicInterfaceStream* music, int loopsCount);

    /* Set the volume */
    void  (*setVolume)(Mix_MusicInterfaceStream* music, int volume);

    /* Tell a play position (in seconds) */
    double (*getCurrentTime)(Mix_MusicInterfaceStream* music);

    /* Seek to a play position (in seconds) */
    void  (*jumpToTime)(Mix_MusicInterfaceStream* music, double position);

    /* Tell a total track length (in seconds) */
    double (*getTimeLength)(Mix_MusicInterfaceStream* music);

    /* Tell a loop start position (in seconds) */
    double (*getLoopStartTime)(Mix_MusicInterfaceStream* music);

    /* Tell a loop end position (in seconds) */
    double (*getLoopEndTime)(Mix_MusicInterfaceStream* music);

    /* Tell a loop length (in seconds) */
    double (*getLoopLengthTime)(Mix_MusicInterfaceStream* music);

    const char* (*metaTitle)(Mix_MusicInterfaceStream* music);
    const char* (*metaArtist)(Mix_MusicInterfaceStream* music);
    const char* (*metaAlbum)(Mix_MusicInterfaceStream* music);
    const char* (*metaCopyright)(Mix_MusicInterfaceStream* music);

    /* Get music data, returns the number of bytes left */
    int   (*playAudio)(Mix_MusicInterfaceStream* music, Uint8* data, int length);

} Mix_MusicInterface;

/*
    Set all function pointers to dummy calls
 */
void initMusicInterface(Mix_MusicInterface *interface);

#endif /* AUDIO_CODEC_H*/

