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

#ifdef MUSIC_MID_ADLMIDI

/* This file supports libADLMIDI music streams */

#include "SDL_mixer_ext.h"

#include "music_midi_adl.h"

#include <adlmidi.h>

#include <stdio.h>

/* Global ADLMIDI flags which are applying on initializing of MIDI player with a file */
static int adlmidi_bank         = 58;
static int adlmidi_tremolo      = 1;
static int adlmidi_vibrato      = 1;
static int adlmidi_scalemod     = 0;
static int adlmidi_adlibdrums   = 0;
static int adlmidi_logVolumes   = 0;
static int adlmidi_volumeModel  = 0;
static char adlmidi_customBankPath[2048] = "";

typedef struct
{
    int play_count;
    struct ADL_MIDIPlayer *adlmidi;
    int volume;

    SDL_AudioStream *stream;
    void *buffer;
    size_t buffer_size;

    char *mus_title;
    char *mus_artist;
    char *mus_album;
    char *mus_copyright;
} AdlMIDI_Music;


int ADLMIDI_getBanksCount()
{
    return adl_getBanksCount();
}

const char *const *ADLMIDI_getBankNames()
{
    return adl_getBankNames();
}

int ADLMIDI_getBankID()
{
    return adlmidi_bank;
}

void ADLMIDI_setBankID(int bnk)
{
    adlmidi_bank = bnk;
}

int ADLMIDI_getTremolo()
{
    return adlmidi_tremolo;
}
void ADLMIDI_setTremolo(int tr)
{
    adlmidi_tremolo = tr;
}

int ADLMIDI_getVibrato()
{
    return adlmidi_vibrato;
}

void ADLMIDI_setVibrato(int vib)
{
    adlmidi_vibrato = vib;
}


int ADLMIDI_getAdLibDrumsMode()
{
    return adlmidi_adlibdrums;
}

void ADLMIDI_setAdLibDrumsMode(int ald)
{
    adlmidi_adlibdrums = ald;
}

int ADLMIDI_getScaleMod()
{
    return adlmidi_scalemod;
}

void ADLMIDI_setScaleMod(int sc)
{
    adlmidi_scalemod = sc;
}

int ADLMIDI_getLogarithmicVolumes()
{
    return adlmidi_logVolumes;
}

void ADLMIDI_setLogarithmicVolumes(int vm)
{
    adlmidi_logVolumes = vm;
}

int ADLMIDI_getVolumeModel()
{
    return adlmidi_volumeModel;
}

void ADLMIDI_setVolumeModel(int vm)
{
    adlmidi_volumeModel = vm;
    if(vm < 0)
        adlmidi_volumeModel = 0;
}

void ADLMIDI_setInfiniteLoop(AdlMIDI_Music *music, int loop)
{
    if(music)
        adl_setLoopEnabled(music->adlmidi, loop);
}

void ADLMIDI_setDefaults()
{
    adlmidi_tremolo     = 1;
    adlmidi_vibrato     = 1;
    adlmidi_scalemod    = 0;
    adlmidi_adlibdrums  = 0;
    adlmidi_bank        = 58;
}


/* Set the volume for a ADLMIDI stream */
static void ADLMIDI_setvolume(void *music_p, int volume)
{
    AdlMIDI_Music *music = (AdlMIDI_Music *)music_p;
    if(music)
        music->volume = (int)round(128.0 * sqrt(((double)volume) * (1.0 / 128.0)));
}

void ADLMIDI_setCustomBankFile(const char *bank_wonl_path)
{
    if(bank_wonl_path)
        strcpy(adlmidi_customBankPath, bank_wonl_path);
    else
        adlmidi_customBankPath[0] = '\0';
}

static void ADLMIDI_delete(void *music_p);

static AdlMIDI_Music *ADLMIDI_LoadSongRW(SDL_RWops *src)
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
        music->mus_title = NULL;
        music->mus_artist = NULL;
        music->mus_album = NULL;
        music->mus_copyright = NULL;

        return music;
    }
    return NULL;
}

/* Load ADLMIDI stream from an SDL_RWops object */
static void *ADLMIDI_new_RW(struct SDL_RWops *src, int freesrc)
{
    AdlMIDI_Music *adlmidiMusic;

    adlmidiMusic = ADLMIDI_LoadSongRW(src);
    if(!adlmidiMusic)
        return NULL;
    if(freesrc)
        SDL_RWclose(src);

    return adlmidiMusic;
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
        /* META-TAGS */
        if(music->mus_title)
            SDL_free(music->mus_title);
        if(music->mus_artist)
            SDL_free(music->mus_artist);
        if(music->mus_album)
            SDL_free(music->mus_album);
        if(music->mus_copyright)
            SDL_free(music->mus_copyright);
        /* META-TAGS */

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

/* Jump (seek) to a given position (time is in seconds) */
int ADLMIDI_jump_to_time(void *music_p, double time)
{
    AdlMIDI_Music *music = (AdlMIDI_Music *)music_p;
    adl_positionSeek(music->adlmidi, time);
    return 0;
}

double ADLMIDI_currentPosition(void *music_p)
{
    AdlMIDI_Music *music = (AdlMIDI_Music *)music_p;
    if(music)
        return adl_positionTell(music->adlmidi);
    return -1;
}

double ADLMIDI_songLength(void *music_p)
{
    AdlMIDI_Music *music = (AdlMIDI_Music *)music_p;
    if(music)
        return adl_totalTimeLength(music->adlmidi);
    return -1;
}

double ADLMIDI_loopStart(void *music_p)
{
    AdlMIDI_Music *music = (AdlMIDI_Music *)music_p;
    if(music)
        return adl_loopStartTime(music->adlmidi);
    return -1;
}

double ADLMIDI_loopEnd(void *music_p)
{
    AdlMIDI_Music *music = (AdlMIDI_Music *)music_p;
    if(music)
        return adl_loopEndTime(music->adlmidi);
    return -1;
}

double ADLMIDI_loopLength(void *music_p)
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

/*
 * Initialize the ADLMIDI player, with the given mixer settings
 * This function returns 0, or -1 if there was an error.
 */

/* This is the format of the audio mixer data */
//static SDL_AudioSpec mixer;

//int ADLMIDI_init2(Mix_MusicInterface *codec, SDL_AudioSpec *mixerfmt)
//{
//    mixer = *mixerfmt;

//    initMusicInterface(codec);

//    codec->isValid = 1;

//    codec->capabilities     = music_interface_default_capabilities;

//    codec->open             = ADLMIDI_new_RW;
//    codec->openEx           = music_interface_dummy_cb_openEx;
//    codec->close            = ADLMIDI_delete;

//    codec->play             = ADLMIDI_play;
//    codec->pause            = music_interface_dummy_cb_void_1arg;
//    codec->resume           = music_interface_dummy_cb_void_1arg;
//    codec->stop             = ADLMIDI_stop;

//    codec->isPlaying        = ADLMIDI_playing;
//    codec->isPaused         = music_interface_dummy_cb_int_1arg;

//    codec->setLoops         = ADLMIDI_setLoops;
//    codec->setVolume        = ADLMIDI_setvolume;

//    codec->jumpToTime       = ADLMIDI_jump_to_time;
//    codec->getCurrentTime   = ADLMIDI_currentPosition;
//    codec->getTimeLength    = ADLMIDI_songLength;

//    codec->getLoopStartTime = ADLMIDI_loopStart;
//    codec->getLoopEndTime   = ADLMIDI_loopEnd;
//    codec->getLoopLengthTime= ADLMIDI_loopLength;

//    codec->metaTitle        = music_interface_dummy_meta_tag;
//    codec->metaArtist       = music_interface_dummy_meta_tag;
//    codec->metaAlbum        = music_interface_dummy_meta_tag;
//    codec->metaCopyright    = music_interface_dummy_meta_tag;

//    codec->playAudio        = ADLMIDI_playAudio;

//    return(0);
//}

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
    NULL,   /* CreateFromFile */
    ADLMIDI_setvolume,
    ADLMIDI_play,
    NULL,   /* IsPlaying */
    ADLMIDI_playAudio,
    ADLMIDI_jump_to_time,
    NULL,   /* Pause */
    NULL,   /* Resume */
    NULL,   /* Stop */
    ADLMIDI_delete,
    NULL,   /* Close */
    NULL,   /* Unload */
};

#endif


