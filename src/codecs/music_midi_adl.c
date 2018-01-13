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

/* This file supports libADLMIDI music streams */

#include "music_midi_adl.h"

/* Global ADLMIDI flags which are applying on initializing of MIDI player with a file */
static int adlmidi_bank         = 58;
static int adlmidi_tremolo      = 1;
static int adlmidi_vibrato      = 1;
static int adlmidi_scalemod     = 0;
static int adlmidi_adlibdrums   = 0;
static int adlmidi_logVolumes   = 0;
static int adlmidi_volumeModel  = 0;
static char adlmidi_customBankPath[2048] = "";

#ifdef MUSIC_MID_ADLMIDI
#include <adlmidi.h>
#include <stdio.h>
#endif//MUSIC_MID_ADLMIDI

int SDLCALLCC Mix_ADLMIDI_getTotalBanks()
{
    #ifdef MUSIC_MID_ADLMIDI
    return adl_getBanksCount();
    #else
    return 0;
    #endif
}

const char *const * SDLCALLCC Mix_ADLMIDI_getBankNames()
{
    #ifdef MUSIC_MID_ADLMIDI
    return adl_getBankNames();
    #else
    return NULL;
    #endif
}

int SDLCALLCC Mix_ADLMIDI_getBankID()
{
    return adlmidi_bank;
}

void SDLCALLCC Mix_ADLMIDI_setBankID(int bnk)
{
    adlmidi_bank = bnk;
}

int SDLCALLCC Mix_ADLMIDI_getTremolo()
{
    return adlmidi_tremolo;
}
void SDLCALLCC Mix_ADLMIDI_setTremolo(int tr)
{
    adlmidi_tremolo = tr;
}

int SDLCALLCC Mix_ADLMIDI_getVibrato()
{
    return adlmidi_vibrato;
}

void SDLCALLCC Mix_ADLMIDI_setVibrato(int vib)
{
    adlmidi_vibrato = vib;
}


int SDLCALLCC Mix_ADLMIDI_getAdLibMode()
{
    return adlmidi_adlibdrums;
}

void SDLCALLCC Mix_ADLMIDI_setAdLibMode(int ald)
{
    adlmidi_adlibdrums = ald;
}

int SDLCALLCC Mix_ADLMIDI_getScaleMod()
{
    return adlmidi_scalemod;
}

void SDLCALLCC Mix_ADLMIDI_setScaleMod(int sc)
{
    adlmidi_scalemod = sc;
}

int SDLCALLCC Mix_ADLMIDI_getLogarithmicVolumes()
{
    return adlmidi_logVolumes;
}

void SDLCALLCC Mix_ADLMIDI_setLogarithmicVolumes(int vm)
{
    adlmidi_logVolumes = vm;
}

int SDLCALLCC Mix_ADLMIDI_getVolumeModel()
{
    return adlmidi_volumeModel;
}

void SDLCALLCC Mix_ADLMIDI_setVolumeModel(int vm)
{
    adlmidi_volumeModel = vm;
    if(vm < 0)
        adlmidi_volumeModel = 0;
}

void SDLCALLCC Mix_ADLMIDI_setSetDefaults()
{
    adlmidi_tremolo     = 1;
    adlmidi_vibrato     = 1;
    adlmidi_scalemod    = 0;
    adlmidi_adlibdrums  = 0;
    adlmidi_bank        = 58;
}

void SDLCALLCC Mix_ADLMIDI_setCustomBankFile(const char *bank_wonl_path)
{
    if(bank_wonl_path)
        SDL_strlcpy(adlmidi_customBankPath, bank_wonl_path, 2048);
    else
        adlmidi_customBankPath[0] = '\0';
}

#ifdef MUSIC_MID_ADLMIDI

typedef struct
{
    int play_count;
    struct ADL_MIDIPlayer *adlmidi;
    int volume;

    SDL_AudioStream *stream;
    void *buffer;
    size_t buffer_size;
    Mix_MusicMetaTags tags;
} AdlMIDI_Music;

/* Set the volume for a ADLMIDI stream */
static void ADLMIDI_setvolume(void *music_p, int volume)
{
    AdlMIDI_Music *music = (AdlMIDI_Music *)music_p;
    music->volume = (int)(round(128.0 * sqrt(((double)volume) * (1.0 / 128.0))));
}

static void process_args(const char *args)
{
    char arg[1024];
    char type = 'x';
    size_t maxlen = 0;
    size_t i, j = 0;
    int value_opened = 0;
    if (args == NULL) {
        return;
    }

    maxlen = SDL_strlen(args) + 1;

    for (i = 0; i < maxlen; i++) {
        char c = args[i];
        if(value_opened == 1) {
            if ((c == ';') || (c == '\0')) {
                int value;
                arg[j] = '\0';
                value = atoi(arg);
                switch(type)
                {
                case 'b':
                    Mix_ADLMIDI_setBankID(value);
                    break;
                case 't':
                    Mix_ADLMIDI_setTremolo(value);
                    break;
                case 'v':
                    Mix_ADLMIDI_setVibrato(value);
                    break;
                case 'a':
                    Mix_ADLMIDI_setAdLibMode(value);
                    break;
                case 'm':
                    Mix_ADLMIDI_setScaleMod(value);
                    break;
                case 'l':
                    Mix_ADLMIDI_setVolumeModel(value);
                    break;
                case '\0':
                    break;
                default:
                    break;
                }
                value_opened = 0;
            }
            arg[j++] = c;
        } else {
            if (c == '\0') {
                return;
            }
            type = c;
            value_opened = 1;
            j = 0;
        }
    }
}

static void ADLMIDI_delete(void *music_p);

static AdlMIDI_Music *ADLMIDI_LoadSongRW(SDL_RWops *src, const char *args)
{
    if(src != NULL)
    {
        void *bytes = 0;
        long filesize = 0;
        int err = 0;
        Sint64 length = 0;
        size_t bytes_l;
        unsigned char byte[1];
        AdlMIDI_Music *music = NULL;

        process_args(args);

        music = (AdlMIDI_Music *)SDL_calloc(1, sizeof(AdlMIDI_Music));

        music->stream = SDL_NewAudioStream(AUDIO_S16, 2, music_spec.freq,
                                           music_spec.format, music_spec.channels, music_spec.freq);
        if (!music->stream) {
            ADLMIDI_delete(music);
            return NULL;
        }

        music->buffer_size = music_spec.samples * sizeof(Sint16) * 2/*channels*/ * music_spec.channels;
        music->buffer = SDL_malloc(music->buffer_size);
        if (!music->buffer) {
            ADLMIDI_delete(music);
            return NULL;
        }

        length = SDL_RWseek(src, 0, RW_SEEK_END);
        if(length < 0)
        {
            Mix_SetError("ADL-MIDI: wrong file\n");
            return NULL;
        }

        SDL_RWseek(src, 0, RW_SEEK_SET);
        bytes = SDL_malloc((size_t)length);

        filesize = 0;
        while((bytes_l = SDL_RWread(src, &byte, sizeof(Uint8), 1)) != 0)
        {
            ((unsigned char *)bytes)[filesize] = byte[0];
            filesize++;
        }

        if(filesize == 0)
        {
            SDL_free(bytes);
            ADLMIDI_delete(music);
            Mix_SetError("ADL-MIDI: wrong file\n");
            return NULL;
        }

        music->adlmidi = adl_init(music_spec.freq);

        adl_setHVibrato(music->adlmidi, adlmidi_vibrato);
        adl_setHTremolo(music->adlmidi, adlmidi_tremolo);
        if(adlmidi_customBankPath[0] != '\0')
            err = adl_openBankFile(music->adlmidi, (char*)adlmidi_customBankPath);
        else
            err = adl_setBank(music->adlmidi, adlmidi_bank);
        if(err < 0)
        {
            Mix_SetError("ADL-MIDI: %s", adl_errorInfo(music->adlmidi));
            SDL_free(bytes);
            ADLMIDI_delete(music);
            return NULL;
        }

        adl_setScaleModulators(music->adlmidi, adlmidi_scalemod);
        adl_setPercMode(music->adlmidi, adlmidi_adlibdrums);
        adl_setLogarithmicVolumes(music->adlmidi, adlmidi_logVolumes);
        adl_setVolumeRangeModel(music->adlmidi, adlmidi_volumeModel);
        adl_setNumCards(music->adlmidi, 4);

        err = adl_openData(music->adlmidi, bytes, (unsigned long)filesize);
        SDL_free(bytes);

        if(err != 0)
        {
            Mix_SetError("ADL-MIDI: %s", adl_errorInfo(music->adlmidi));
            ADLMIDI_delete(music);
            return NULL;
        }

        music->volume                 = MIX_MAX_VOLUME;
        meta_tags_init(&music->tags);
        meta_tags_set(&music->tags, MIX_META_TITLE, adl_metaMusicTitle(music->adlmidi));
        meta_tags_set(&music->tags, MIX_META_COPYRIGHT, adl_metaMusicCopyright(music->adlmidi));
        return music;
    }
    return NULL;
}

/* Load ADLMIDI stream from an SDL_RWops object */
static void *ADLMIDI_new_RWex(struct SDL_RWops *src, int freesrc, const char *args)
{
    AdlMIDI_Music *adlmidiMusic;

    adlmidiMusic = ADLMIDI_LoadSongRW(src, args);
    if(!adlmidiMusic)
        return NULL;
    if(freesrc)
        SDL_RWclose(src);

    return adlmidiMusic;
}

static void *ADLMIDI_new_RW(struct SDL_RWops *src, int freesrc)
{
    return ADLMIDI_new_RWex(src, freesrc, NULL);
}


/* Start playback of a given Game Music Emulators stream */
static int ADLMIDI_play(void *music_p, int play_counts)
{
    AdlMIDI_Music *music = (AdlMIDI_Music *)music_p;
    adl_positionRewind(music->adlmidi);
    music->play_count = play_counts;
    adl_setLoopEnabled(music->adlmidi, (play_counts < 0));
    return 0;
}

static int ADLMIDI_playSome(void *context, void *data, int bytes, SDL_bool *done)
{
    AdlMIDI_Music *music = (AdlMIDI_Music *)context;
    int filled, gottenLen, amount;

    filled = SDL_AudioStreamGet(music->stream, data, bytes);
    if (filled != 0) {
        return filled;
    }

    if (!music->play_count) {
        /* All done */
        *done = SDL_TRUE;
        return 0;
    }

    /* Align bytes length to correctly capture a stereo input */
    if ((bytes % 4) != 0) {
        bytes += (4 - (bytes % 4));
    }

    gottenLen = adl_play(music->adlmidi, (bytes / 2), (short*)music->buffer);
    if (gottenLen <= 0) {
        *done = SDL_TRUE;
        return 0;
    }

    amount = gottenLen * 2;
    if (amount > 0) {
        if (SDL_AudioStreamPut(music->stream, music->buffer, amount) < 0) {
            return -1;
        }
    } else {
        if (music->play_count == 1) {
            music->play_count = 0;
            SDL_AudioStreamFlush(music->stream);
        } else {
            int play_count = -1;
            if (music->play_count > 0) {
                play_count = (music->play_count - 1);
            }
            adl_positionRewind(music->adlmidi);
            music->play_count = play_count;
        }
    }

    return 0;
}

/* Play some of a stream previously started with ADLMIDI_play() */
static int ADLMIDI_playAudio(void *music_p, void *stream, int len)
{
    AdlMIDI_Music *music = (AdlMIDI_Music *)music_p;
    return music_pcm_getaudio(music_p, stream, len, music->volume, ADLMIDI_playSome);
}

/* Close the given Game Music Emulators stream */
static void ADLMIDI_delete(void *music_p)
{
    AdlMIDI_Music *music = (AdlMIDI_Music *)music_p;
    if(music)
    {
        meta_tags_clear(&music->tags);
        if(music->adlmidi) {
            adl_close(music->adlmidi);
        }
        if (music->stream) {
            SDL_FreeAudioStream(music->stream);
        }
        if (music->buffer) {
            SDL_free(music->buffer);
        }
        SDL_free(music);
    }
}

static const char* ADLMIDI_GetMetaTag(void *context, Mix_MusicMetaTag tag_type)
{
    AdlMIDI_Music *music = (AdlMIDI_Music *)context;
    return meta_tags_get(&music->tags, tag_type);
}

/* Jump (seek) to a given position (time is in seconds) */
static int ADLMIDI_jump_to_time(void *music_p, double time)
{
    AdlMIDI_Music *music = (AdlMIDI_Music *)music_p;
    adl_positionSeek(music->adlmidi, time);
    return 0;
}

static double ADLMIDI_currentPosition(void *music_p)
{
    AdlMIDI_Music *music = (AdlMIDI_Music *)music_p;
    if(music)
        return adl_positionTell(music->adlmidi);
    return -1;
}

static double ADLMIDI_songLength(void *music_p)
{
    AdlMIDI_Music *music = (AdlMIDI_Music *)music_p;
    if(music)
        return adl_totalTimeLength(music->adlmidi);
    return -1;
}

static double ADLMIDI_loopStart(void *music_p)
{
    AdlMIDI_Music *music = (AdlMIDI_Music *)music_p;
    if(music)
        return adl_loopStartTime(music->adlmidi);
    return -1;
}

static double ADLMIDI_loopEnd(void *music_p)
{
    AdlMIDI_Music *music = (AdlMIDI_Music *)music_p;
    if(music)
        return adl_loopEndTime(music->adlmidi);
    return -1;
}

static double ADLMIDI_loopLength(void *music_p)
{
    AdlMIDI_Music *music = (AdlMIDI_Music *)music_p;
    if(music)
    {
        double start = adl_loopStartTime(music->adlmidi);
        double end = adl_loopEndTime(music->adlmidi);
        if(start >= 0 && end >= 0)
            return (end - start);
    }
    return -1;
}


Mix_MusicInterface Mix_MusicInterface_ADLMIDI =
{
    "ADLMIDI",
    MIX_MUSIC_ADLMIDI,
    MUS_MID,
    SDL_FALSE,
    SDL_FALSE,

    NULL,   /* Load */
    NULL,   /* Open */
    ADLMIDI_new_RW,
    ADLMIDI_new_RWex,   /* CreateFromRWex [MIXER-X]*/
    NULL,   /* CreateFromFile */
    NULL,   /* CreateFromFileEx [MIXER-X]*/
    ADLMIDI_setvolume,
    ADLMIDI_play,
    NULL,   /* IsPlaying */
    ADLMIDI_playAudio,
    ADLMIDI_jump_to_time,
    ADLMIDI_currentPosition,   /* Tell [MIXER-X]*/
    ADLMIDI_songLength,   /* FullLength [MIXER-X]*/
    ADLMIDI_loopStart,   /* LoopStart [MIXER-X]*/
    ADLMIDI_loopEnd,   /* LoopEnd [MIXER-X]*/
    ADLMIDI_loopLength,   /* LoopLength [MIXER-X]*/
    ADLMIDI_GetMetaTag,   /* GetMetaTag [MIXER-X]*/
    NULL,   /* Pause */
    NULL,   /* Resume */
    NULL,   /* Stop */
    ADLMIDI_delete,
    NULL,   /* Close */
    NULL,   /* Unload */
};

/* Same as Mix_MusicInterface_ADLMIDI. Created to play special music formats separately from off MIDI interfaces */
Mix_MusicInterface Mix_MusicInterface_ADLIMF =
{
    "ADLIMF",
    MIX_MUSIC_ADLMIDI,
    MUS_ADLMIDI,
    SDL_FALSE,
    SDL_FALSE,

    NULL,   /* Load */
    NULL,   /* Open */
    ADLMIDI_new_RW,
    ADLMIDI_new_RWex,   /* CreateFromRWex [MIXER-X]*/
    NULL,   /* CreateFromFile */
    NULL,   /* CreateFromFileEx [MIXER-X]*/
    ADLMIDI_setvolume,
    ADLMIDI_play,
    NULL,   /* IsPlaying */
    ADLMIDI_playAudio,
    ADLMIDI_jump_to_time,
    ADLMIDI_currentPosition,   /* Tell [MIXER-X]*/
    ADLMIDI_songLength,   /* FullLength [MIXER-X]*/
    ADLMIDI_loopStart,   /* LoopStart [MIXER-X]*/
    ADLMIDI_loopEnd,   /* LoopEnd [MIXER-X]*/
    ADLMIDI_loopLength,   /* LoopLength [MIXER-X]*/
    ADLMIDI_GetMetaTag,   /* GetMetaTag [MIXER-X]*/
    NULL,   /* Pause */
    NULL,   /* Resume */
    NULL,   /* Stop */
    ADLMIDI_delete,
    NULL,   /* Close */
    NULL,   /* Unload */
};

#endif //MUSIC_MID_ADLMIDI


