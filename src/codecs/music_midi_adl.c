/*
  SDL_mixer:  An audio mixer library based on the SDL library
  Copyright (C) 1997-2020 Sam Lantinga <slouken@libsdl.org>

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

#ifdef MUSIC_MID_ADLMIDI

#include "SDL_loadso.h"
#include "utils.h"

#include <adlmidi.h>

typedef struct {
    int loaded;
    void *handle;

    int (*adl_getBanksCount)(void);
    const char *const *(*adl_getBankNames)(void);
    struct ADL_MIDIPlayer *(*adl_init)(long sample_rate);
    void (*adl_close)(struct ADL_MIDIPlayer *device);
    void (*adl_setHVibrato)(struct ADL_MIDIPlayer *device, int hvibro);
    void (*adl_setHTremolo)(struct ADL_MIDIPlayer *device, int htremo);
    int (*adl_openBankFile)(struct ADL_MIDIPlayer *device, const char *filePath);
    int (*adl_setBank)(struct ADL_MIDIPlayer *device, int bank);
    const char *(*adl_errorInfo)(struct ADL_MIDIPlayer *device);
    int (*adl_switchEmulator)(struct ADL_MIDIPlayer *device, int emulator);
    void (*adl_setScaleModulators)(struct ADL_MIDIPlayer *device, int smod);
    void (*adl_setVolumeRangeModel)(struct ADL_MIDIPlayer *device, int volumeModel);
    void (*adl_setFullRangeBrightness)(struct ADL_MIDIPlayer *device, int fr_brightness);
    void (*adl_setSoftPanEnabled)(struct ADL_MIDIPlayer *device, int softPanEn);
    int (*adl_setNumChips)(struct ADL_MIDIPlayer *device, int numChips);
    int (*adl_setNumFourOpsChn)(struct ADL_MIDIPlayer *device, int ops4);
    void (*adl_setTempo)(struct ADL_MIDIPlayer *device, double tempo);
    int (*adl_openData)(struct ADL_MIDIPlayer *device, const void *mem, unsigned long size);
    const char *(*adl_metaMusicTitle)(struct ADL_MIDIPlayer *device);
    const char *(*adl_metaMusicCopyright)(struct ADL_MIDIPlayer *device);
    void (*adl_positionRewind)(struct ADL_MIDIPlayer *device);
    void (*adl_setLoopEnabled)(struct ADL_MIDIPlayer *device, int loopEn);
    int  (*adl_playFormat)(struct ADL_MIDIPlayer *device, int sampleCount,
                           ADL_UInt8 *left, ADL_UInt8 *right,
                           const struct ADLMIDI_AudioFormat *format);
    void (*adl_positionSeek)(struct ADL_MIDIPlayer *device, double seconds);
    double (*adl_positionTell)(struct ADL_MIDIPlayer *device);
    double (*adl_totalTimeLength)(struct ADL_MIDIPlayer *device);
    double (*adl_loopStartTime)(struct ADL_MIDIPlayer *device);
    double (*adl_loopEndTime)(struct ADL_MIDIPlayer *device);
} adlmidi_loader;

static adlmidi_loader ADLMIDI = {
    0, NULL
};

#ifdef ADLMIDI_DYNAMIC
#define FUNCTION_LOADER(FUNC, SIG) \
    ADLMIDI.FUNC = (SIG) SDL_LoadFunction(ADLMIDI.handle, #FUNC); \
    if (ADLMIDI.FUNC == NULL) { SDL_UnloadObject(ADLMIDI.handle); return -1; }
#else
#define FUNCTION_LOADER(FUNC, SIG) \
    ADLMIDI.FUNC = FUNC;
#endif

static int ADLMIDI_Load(void)
{
    if (ADLMIDI.loaded == 0) {
#ifdef ADLMIDI_DYNAMIC
        ADLMIDI.handle = SDL_LoadObject(ADLMIDI_DYNAMIC);
        if (ADLMIDI.handle == NULL) {
            return -1;
        }
#elif defined(__MACOSX__)
        extern struct ADL_MIDIPlayer *adl_init(long) __attribute__((weak_import));
        if (adl_init == NULL) {
            /* Missing weakly linked framework */
            Mix_SetError("Missing ADLMIDI.framework");
            return -1;
        }
#endif
        FUNCTION_LOADER(adl_getBanksCount, int (*)(void))
        FUNCTION_LOADER(adl_getBankNames, const char *const *(*)(void))
        FUNCTION_LOADER(adl_init, struct ADL_MIDIPlayer *(*)(long))
        FUNCTION_LOADER(adl_close, void(*)(struct ADL_MIDIPlayer*))
        FUNCTION_LOADER(adl_setHVibrato, void(*)(struct ADL_MIDIPlayer*,int))
        FUNCTION_LOADER(adl_setHTremolo, void(*)(struct ADL_MIDIPlayer*,int))
        FUNCTION_LOADER(adl_openBankFile, int(*)(struct ADL_MIDIPlayer*,const char*))
        FUNCTION_LOADER(adl_setBank, int(*)(struct ADL_MIDIPlayer *, int))
        FUNCTION_LOADER(adl_errorInfo, const char *(*)(struct ADL_MIDIPlayer*))
        FUNCTION_LOADER(adl_switchEmulator, int(*)(struct ADL_MIDIPlayer*,int))
        FUNCTION_LOADER(adl_setScaleModulators, void(*)(struct ADL_MIDIPlayer*,int))
        FUNCTION_LOADER(adl_setVolumeRangeModel, void(*)(struct ADL_MIDIPlayer*,int))
        FUNCTION_LOADER(adl_setFullRangeBrightness, void(*)(struct ADL_MIDIPlayer*,int))
        FUNCTION_LOADER(adl_setSoftPanEnabled, void(*)(struct ADL_MIDIPlayer*,int))
        FUNCTION_LOADER(adl_setNumChips, int(*)(struct ADL_MIDIPlayer *, int))
        FUNCTION_LOADER(adl_setNumFourOpsChn, int(*)(struct ADL_MIDIPlayer*,int))
        FUNCTION_LOADER(adl_setTempo, void(*)(struct ADL_MIDIPlayer*,double))
        FUNCTION_LOADER(adl_openData, int(*)(struct ADL_MIDIPlayer *, const void *, unsigned long))
        FUNCTION_LOADER(adl_metaMusicTitle, const char*(*)(struct ADL_MIDIPlayer*))
        FUNCTION_LOADER(adl_metaMusicCopyright, const char*(*)(struct ADL_MIDIPlayer*))
        FUNCTION_LOADER(adl_positionRewind, void (*)(struct ADL_MIDIPlayer*))
        FUNCTION_LOADER(adl_setLoopEnabled, void(*)(struct ADL_MIDIPlayer*,int))
        FUNCTION_LOADER(adl_playFormat, int(*)(struct ADL_MIDIPlayer *,int,
                               ADL_UInt8*,ADL_UInt8*,const struct ADLMIDI_AudioFormat*))
        FUNCTION_LOADER(adl_positionSeek, void(*)(struct ADL_MIDIPlayer*,double))
        FUNCTION_LOADER(adl_positionTell, double(*)(struct ADL_MIDIPlayer*))
        FUNCTION_LOADER(adl_totalTimeLength, double(*)(struct ADL_MIDIPlayer*))
        FUNCTION_LOADER(adl_loopStartTime, double(*)(struct ADL_MIDIPlayer*))
        FUNCTION_LOADER(adl_loopEndTime, double(*)(struct ADL_MIDIPlayer*))
    }
    ++ADLMIDI.loaded;

    return 0;
}

static void ADLMIDI_Unload(void)
{
    if (ADLMIDI.loaded == 0) {
        return;
    }
    if (ADLMIDI.loaded == 1) {
#ifdef ADLMIDI_DYNAMIC
        SDL_UnloadObject(ADLMIDI.handle);
#endif
    }
    --ADLMIDI.loaded;
}


/* Global ADLMIDI flags which are applying on initializing of MIDI player with a file */
typedef struct {
    int bank;
    int tremolo;
    int vibrato;
    int scalemod;
    int volume_model;
    int chips_count;
    int four_op_channels;
    int full_brightness_range;
    int soft_pan;
    int emulator;
    char custom_bank_path[2048];
    double tempo;
    double gain;
} AdlMidi_Setup;

#define ADLMIDI_DEFAULT_CHIPS_COUNT     4

static AdlMidi_Setup adlmidi_setup = {
    58, -1, -1, -1, 0, -1, -1, 0, 1, ADLMIDI_EMU_DOSBOX, "", 1.0, 2.0
};

static void ADLMIDI_SetDefault(AdlMidi_Setup *setup)
{
    setup->bank        = 58;
    setup->tremolo     = -1;
    setup->vibrato     = -1;
    setup->scalemod    = -1;
    setup->volume_model = 0;
    setup->chips_count = -1;
    setup->four_op_channels = -1;
    setup->full_brightness_range = 0;
    setup->soft_pan = 1;
    setup->emulator = -1;
    setup->custom_bank_path[0] = '\0';
    setup->tempo = 1.0;
    setup->gain = 2.0;
}
#endif /* MUSIC_MID_ADLMIDI */

int SDLCALLCC Mix_ADLMIDI_getTotalBanks()
{
#ifdef MUSIC_MID_ADLMIDI
    /* Calling of this static call requires pre-loaindg of the library */
    if (ADLMIDI.loaded == 0) {
        ADLMIDI_Load();
    }
    if (ADLMIDI.adl_getBanksCount) {
        return ADLMIDI.adl_getBanksCount();
    }
#endif
    return 0;
}

const char *const * SDLCALLCC Mix_ADLMIDI_getBankNames()
{
#ifdef MUSIC_MID_ADLMIDI
    /* Calling of this static call requires pre-loaindg of the library */
    if (ADLMIDI.loaded == 0) {
        ADLMIDI_Load();
    }
    if (ADLMIDI.adl_getBankNames) {
        return ADLMIDI.adl_getBankNames();
    }
#endif
    return NULL;
}

int SDLCALLCC Mix_ADLMIDI_getBankID()
{
#ifdef MUSIC_MID_ADLMIDI
    return adlmidi_setup.bank;
#else
    return 0;
#endif
}

void SDLCALLCC Mix_ADLMIDI_setBankID(int bnk)
{
#ifdef MUSIC_MID_ADLMIDI
    adlmidi_setup.bank = bnk;
#else
    (void)bnk;
#endif
}

int SDLCALLCC Mix_ADLMIDI_getTremolo()
{
#ifdef MUSIC_MID_ADLMIDI
    return adlmidi_setup.tremolo;
#else
    return -1;
#endif
}
void SDLCALLCC Mix_ADLMIDI_setTremolo(int tr)
{
#ifdef MUSIC_MID_ADLMIDI
    adlmidi_setup.tremolo = tr;
#else
    (void)tr;
#endif
}

int SDLCALLCC Mix_ADLMIDI_getVibrato()
{
#ifdef MUSIC_MID_ADLMIDI
    return adlmidi_setup.vibrato;
#else
    return -1;
#endif
}

void SDLCALLCC Mix_ADLMIDI_setVibrato(int vib)
{
#ifdef MUSIC_MID_ADLMIDI
    adlmidi_setup.vibrato = vib;
#else
    (void)vib;
#endif
}

int SDLCALLCC Mix_ADLMIDI_getScaleMod()
{
#ifdef MUSIC_MID_ADLMIDI
    return adlmidi_setup.scalemod;
#else
    return -1;
#endif
}

void SDLCALLCC Mix_ADLMIDI_setScaleMod(int sc)
{
#ifdef MUSIC_MID_ADLMIDI
    adlmidi_setup.scalemod = sc;
#else
    (void)sc;
#endif
}

int SDLCALLCC Mix_ADLMIDI_getLogarithmicVolumes()
{
#ifdef MUSIC_MID_ADLMIDI
    return (adlmidi_setup.volume_model == ADLMIDI_VolumeModel_NativeOPL3);
#else
    return -1;
#endif
}

void SDLCALLCC Mix_ADLMIDI_setLogarithmicVolumes(int vm)
{
#ifdef MUSIC_MID_ADLMIDI
    adlmidi_setup.volume_model = vm ? ADLMIDI_VolumeModel_NativeOPL3 : ADLMIDI_VolumeModel_AUTO;
#else
    (void)vm;
#endif
}

int SDLCALLCC Mix_ADLMIDI_getVolumeModel()
{
#ifdef MUSIC_MID_ADLMIDI
    return adlmidi_setup.volume_model;
#else
    return -1;
#endif
}

void SDLCALLCC Mix_ADLMIDI_setVolumeModel(int vm)
{
#ifdef MUSIC_MID_ADLMIDI
    adlmidi_setup.volume_model = vm;
    if (vm < 0) {
        adlmidi_setup.volume_model = 0;
    }
#else
    (void)vm;
#endif
}

int SDLCALLCC Mix_ADLMIDI_getFullRangeBrightness()
{
#ifdef MUSIC_MID_ADLMIDI
    return adlmidi_setup.full_brightness_range;
#else
    return -1;
#endif
}

void SDLCALLCC Mix_ADLMIDI_setFullRangeBrightness(int frb)
{
#ifdef MUSIC_MID_ADLMIDI
    adlmidi_setup.full_brightness_range = frb;
#else
    (void)frb;
#endif
}

int SDLCALLCC Mix_ADLMIDI_getFullPanStereo()
{
#ifdef MUSIC_MID_ADLMIDI
    return adlmidi_setup.soft_pan;
#else
    return -1;
#endif
}

void SDLCALLCC Mix_ADLMIDI_setFullPanStereo(int fp)
{
#ifdef MUSIC_MID_ADLMIDI
    adlmidi_setup.soft_pan = fp;
#else
    (void)fp;
#endif
}

int SDLCALLCC Mix_ADLMIDI_getEmulator()
{
#ifdef MUSIC_MID_ADLMIDI
    return adlmidi_setup.emulator;
#else
    return -1;
#endif
}

void SDLCALLCC Mix_ADLMIDI_setEmulator(int emu)
{
#ifdef MUSIC_MID_ADLMIDI
    adlmidi_setup.emulator = emu;
#else
    (void)emu;
#endif
}

int SDLCALLCC Mix_ADLMIDI_getChipsCount()
{
#ifdef MUSIC_MID_ADLMIDI
    return adlmidi_setup.chips_count;
#else
    return -1;
#endif
}

void SDLCALLCC Mix_ADLMIDI_setChipsCount(int chips)
{
#ifdef MUSIC_MID_ADLMIDI
    adlmidi_setup.chips_count = chips;
#else
    (void)chips;
#endif
}

void SDLCALLCC Mix_ADLMIDI_setSetDefaults()
{
#ifdef MUSIC_MID_ADLMIDI
    ADLMIDI_SetDefault(&adlmidi_setup);
#endif
}

void SDLCALLCC Mix_ADLMIDI_setCustomBankFile(const char *bank_wonl_path)
{
#ifdef MUSIC_MID_ADLMIDI
    if (bank_wonl_path) {
        SDL_strlcpy(adlmidi_setup.custom_bank_path, bank_wonl_path, 2048);
    } else {
        adlmidi_setup.custom_bank_path[0] = '\0';
    }
#else
    (void)bank_wonl_path;
#endif
}

#ifdef MUSIC_MID_ADLMIDI

typedef struct
{
    int play_count;
    struct ADL_MIDIPlayer *adlmidi;
    int volume;
    double tempo;
    double gain;

    SDL_AudioStream *stream;
    void *buffer;
    size_t buffer_size;
    Mix_MusicMetaTags tags;
    struct ADLMIDI_AudioFormat sample_format;
} AdlMIDI_Music;

/* Set the volume for a ADLMIDI stream */
static void ADLMIDI_setvolume(void *music_p, int volume)
{
    AdlMIDI_Music *music = (AdlMIDI_Music *)music_p;
    double v = SDL_floor(((double)(volume) * music->gain) + 0.5);
    music->volume = (int)v;
}

/* Get the volume for a ADLMIDI stream */
static int ADLMIDI_getvolume(void *music_p)
{
    AdlMIDI_Music *music = (AdlMIDI_Music *)music_p;
    double v = SDL_floor(((double)(music->volume) / music->gain) + 0.5);
    return (int)v;
}

static void process_args(const char *args, AdlMidi_Setup *setup)
{
#define ARG_BUFFER_SIZE    1024
    char arg[ARG_BUFFER_SIZE];
    char type = '-';
    size_t maxlen = 0;
    size_t i, j = 0;
    int value_opened = 0;
    if (args == NULL) {
        return;
    }
    maxlen = SDL_strlen(args);
    if (maxlen == 0) {
        return;
    }

    maxlen += 1;
    ADLMIDI_SetDefault(setup);

    for (i = 0; i < maxlen; i++) {
        char c = args[i];
        if (value_opened == 1) {
            if ((c == ';') || (c == '\0')) {
                int value = 0;
                arg[j] = '\0';
                if (type != 'x') {
                    value = SDL_atoi(arg);
                }
                switch(type)
                {
                case 'c':
                    setup->chips_count = value;
                    break;
                case 'f':
                    setup->four_op_channels = value;
                    break;
                case 'b':
                    setup->bank = value;
                    break;
                case 't':
                    if (arg[0] == '=') {
                        setup->tempo = SDL_strtod(arg + 1, NULL);
                        if (setup->tempo <= 0.0) {
                            setup->tempo = 1.0;
                        }
                    } else {
                        setup->tremolo = value;
                    }
                    break;
                case 'g':
                    if (arg[0] == '=') {
                        setup->gain = SDL_strtod(arg + 1, NULL);
                        if (setup->gain < 0.0) {
                            setup->gain = 1.0;
                        }
                    }
                    break;
                case 'v':
                    setup->vibrato = value;
                    break;
                case 'a':
                    /* Deprecated and useless */
                    break;
                case 'm':
                    setup->scalemod = value;
                    break;
                case 'l':
                    setup->volume_model = value;
                    break;
                case 'r':
                    setup->full_brightness_range = value;
                    break;
                case 'p':
                    setup->soft_pan = value;
                    break;
                case 'e':
                    setup->emulator = value;
                    break;
                case 'x':
                    if (arg[0] == '=') {
                        SDL_strlcpy(setup->custom_bank_path, arg + 1, (ARG_BUFFER_SIZE - 1));
                    }
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
#undef ARG_BUFFER_SIZE
}

static void ADLMIDI_delete(void *music_p);

static AdlMIDI_Music *ADLMIDI_LoadSongRW(SDL_RWops *src, const char *args)
{
    void *bytes = 0;
    long filesize = 0;
    int err = 0;
    Sint64 length = 0;
    size_t bytes_l;
    unsigned char byte[1];
    AdlMIDI_Music *music = NULL;
    AdlMidi_Setup setup = adlmidi_setup;
    unsigned short src_format = music_spec.format;

    if (src == NULL) {
        return NULL;
    }

    process_args(args, &setup);

    music = (AdlMIDI_Music *)SDL_calloc(1, sizeof(AdlMIDI_Music));

    music->tempo = setup.tempo;
    music->gain = setup.gain;
    music->volume = MIX_MAX_VOLUME;

    switch (music_spec.format) {
    case AUDIO_U8:
        music->sample_format.type = ADLMIDI_SampleType_U8;
        music->sample_format.containerSize = sizeof(Uint8);
        music->sample_format.sampleOffset = sizeof(Uint8) * 2;
        break;
    case AUDIO_S8:
        music->sample_format.type = ADLMIDI_SampleType_S8;
        music->sample_format.containerSize = sizeof(Sint8);
        music->sample_format.sampleOffset = sizeof(Sint8) * 2;
        break;
    case AUDIO_S16:
        music->sample_format.type = ADLMIDI_SampleType_S16;
        music->sample_format.containerSize = sizeof(Sint16);
        music->sample_format.sampleOffset = sizeof(Sint16) * 2;
        break;
    case AUDIO_U16:
        music->sample_format.type = ADLMIDI_SampleType_U16;
        music->sample_format.containerSize = sizeof(Uint16);
        music->sample_format.sampleOffset = sizeof(Uint16) * 2;
        break;
    case AUDIO_S32:
        music->sample_format.type = ADLMIDI_SampleType_S32;
        music->sample_format.containerSize = sizeof(Sint32);
        music->sample_format.sampleOffset = sizeof(Sint32) * 2;
        break;
    case AUDIO_F32:
    default:
        music->sample_format.type = ADLMIDI_SampleType_F32;
        music->sample_format.containerSize = sizeof(float);
        music->sample_format.sampleOffset = sizeof(float) * 2;
        src_format = AUDIO_F32;
    }

    music->stream = SDL_NewAudioStream(src_format, 2, music_spec.freq,
                                       music_spec.format, music_spec.channels, music_spec.freq);

    if (!music->stream) {
        ADLMIDI_delete(music);
        return NULL;
    }

    music->buffer_size = music_spec.samples * music->sample_format.containerSize * 2/*channels*/ * music_spec.channels;
    music->buffer = SDL_malloc(music->buffer_size);
    if (!music->buffer) {
        SDL_OutOfMemory();
        ADLMIDI_delete(music);
        return NULL;
    }

    length = SDL_RWseek(src, 0, RW_SEEK_END);
    if (length < 0) {
        Mix_SetError("ADL-MIDI: wrong file\n");
        ADLMIDI_delete(music);
        return NULL;
    }

    SDL_RWseek(src, 0, RW_SEEK_SET);
    bytes = SDL_malloc((size_t)length);
    if (!bytes) {
        SDL_OutOfMemory();
        ADLMIDI_delete(music);
        return NULL;
    }

    filesize = 0;
    while ((bytes_l = SDL_RWread(src, &byte, sizeof(Uint8), 1)) != 0) {
        ((Uint8 *)bytes)[filesize] = byte[0];
        filesize++;
    }

    if (filesize == 0) {
        SDL_free(bytes);
        ADLMIDI_delete(music);
        Mix_SetError("ADL-MIDI: wrong file\n");
        return NULL;
    }

    music->adlmidi = ADLMIDI.adl_init(music_spec.freq);
    if (!music->adlmidi) {
        SDL_OutOfMemory();
        ADLMIDI_delete(music);
        return NULL;
    }

    ADLMIDI.adl_setHVibrato(music->adlmidi, setup.vibrato);
    ADLMIDI.adl_setHTremolo(music->adlmidi, setup.tremolo);

    if (setup.custom_bank_path[0] != '\0') {
        err = ADLMIDI.adl_openBankFile(music->adlmidi, (char*)setup.custom_bank_path);
    } else {
        err = ADLMIDI.adl_setBank(music->adlmidi, setup.bank);
    }

    if (err < 0) {
        Mix_SetError("ADL-MIDI: %s", ADLMIDI.adl_errorInfo(music->adlmidi));
        SDL_free(bytes);
        ADLMIDI_delete(music);
        return NULL;
    }

    ADLMIDI.adl_switchEmulator( music->adlmidi, (setup.emulator >= 0) ? setup.emulator : ADLMIDI_EMU_DOSBOX );
    ADLMIDI.adl_setScaleModulators(music->adlmidi, setup.scalemod);
    ADLMIDI.adl_setVolumeRangeModel(music->adlmidi, setup.volume_model);
    ADLMIDI.adl_setFullRangeBrightness(music->adlmidi, setup.full_brightness_range);
    ADLMIDI.adl_setSoftPanEnabled(music->adlmidi, setup.soft_pan);
    ADLMIDI.adl_setNumChips(music->adlmidi, (setup.chips_count >= 0) ? setup.chips_count : ADLMIDI_DEFAULT_CHIPS_COUNT);
    if (setup.four_op_channels >= 0) {
        ADLMIDI.adl_setNumFourOpsChn(music->adlmidi, setup.four_op_channels);
    }
    ADLMIDI.adl_setTempo(music->adlmidi, music->tempo);

    err = ADLMIDI.adl_openData(music->adlmidi, bytes, (unsigned long)filesize);
    SDL_free(bytes);

    if (err != 0) {
        Mix_SetError("ADL-MIDI: %s", ADLMIDI.adl_errorInfo(music->adlmidi));
        ADLMIDI_delete(music);
        return NULL;
    }

    meta_tags_init(&music->tags);
    meta_tags_set_from_midi(&music->tags, MIX_META_TITLE, ADLMIDI.adl_metaMusicTitle(music->adlmidi));
    meta_tags_set_from_midi(&music->tags, MIX_META_COPYRIGHT, ADLMIDI.adl_metaMusicCopyright(music->adlmidi));
    return music;
}

/* Load ADLMIDI stream from an SDL_RWops object */
static void *ADLMIDI_new_RWex(struct SDL_RWops *src, int freesrc, const char *args)
{
    AdlMIDI_Music *adlmidiMusic;

    adlmidiMusic = ADLMIDI_LoadSongRW(src, args);
    if (!adlmidiMusic) {
        return NULL;
    }
    if (freesrc) {
        SDL_RWclose(src);
    }

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
    ADLMIDI.adl_positionRewind(music->adlmidi);
    music->play_count = play_counts;
    ADLMIDI.adl_setLoopEnabled(music->adlmidi, (play_counts < 0));
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
    if ((bytes % (int)music->sample_format.sampleOffset) != 0) {
        bytes += ((int)music->sample_format.sampleOffset - (bytes % (int)music->sample_format.sampleOffset));
    }

    gottenLen = ADLMIDI.adl_playFormat(music->adlmidi,
                                      (bytes / (int)music->sample_format.containerSize),
                                      (ADL_UInt8*)music->buffer,
                                      (ADL_UInt8*)music->buffer + music->sample_format.containerSize,
                                      &music->sample_format);

    if (gottenLen <= 0) {
        *done = SDL_TRUE;
        return 0;
    }

    amount = gottenLen * (int)music->sample_format.containerSize;
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
            ADLMIDI.adl_positionRewind(music->adlmidi);
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
    if (music) {
        meta_tags_clear(&music->tags);
        if (music->adlmidi) {
            ADLMIDI.adl_close(music->adlmidi);
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
static int ADLMIDI_Seek(void *music_p, double time)
{
    AdlMIDI_Music *music = (AdlMIDI_Music *)music_p;
    ADLMIDI.adl_positionSeek(music->adlmidi, time);
    return 0;
}

static double ADLMIDI_Tell(void *music_p)
{
    AdlMIDI_Music *music = (AdlMIDI_Music *)music_p;
    if (music) {
        return ADLMIDI.adl_positionTell(music->adlmidi);
    }
    return -1;
}

static double ADLMIDI_Duration(void *music_p)
{
    AdlMIDI_Music *music = (AdlMIDI_Music *)music_p;
    if (music) {
        return ADLMIDI.adl_totalTimeLength(music->adlmidi);
    }
    return -1;
}

static int ADLMIDI_SetTempo(void *music_p, double tempo)
{
    AdlMIDI_Music *music = (AdlMIDI_Music *)music_p;
    if (music && (tempo > 0.0)) {
        ADLMIDI.adl_setTempo(music->adlmidi, tempo);
        music->tempo = tempo;
        return 0;
    }
    return -1;
}

static double ADLMIDI_GetTempo(void *music_p)
{
    AdlMIDI_Music *music = (AdlMIDI_Music *)music_p;
    if (music) {
        return music->tempo;
    }
    return -1.0;
}

static double ADLMIDI_LoopStart(void *music_p)
{
    AdlMIDI_Music *music = (AdlMIDI_Music *)music_p;
    if (music) {
        return ADLMIDI.adl_loopStartTime(music->adlmidi);
    }
    return -1;
}

static double ADLMIDI_LoopEnd(void *music_p)
{
    AdlMIDI_Music *music = (AdlMIDI_Music *)music_p;
    if (music) {
        return ADLMIDI.adl_loopEndTime(music->adlmidi);
    }
    return -1;
}

static double ADLMIDI_LoopLength(void *music_p)
{
    AdlMIDI_Music *music = (AdlMIDI_Music *)music_p;
    if (music) {
        double start = ADLMIDI.adl_loopStartTime(music->adlmidi);
        double end = ADLMIDI.adl_loopEndTime(music->adlmidi);
        if (start >= 0 && end >= 0) {
            return (end - start);
        }
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

    ADLMIDI_Load,
    NULL,   /* Open */
    ADLMIDI_new_RW,
    ADLMIDI_new_RWex,   /* CreateFromRWex [MIXER-X]*/
    NULL,   /* CreateFromFile */
    NULL,   /* CreateFromFileEx [MIXER-X]*/
    ADLMIDI_setvolume,
    NULL,   /* GetVolume [MIXER-X]*/
    ADLMIDI_play,
    NULL,   /* IsPlaying */
    ADLMIDI_playAudio,
    ADLMIDI_Seek,
    ADLMIDI_Tell,   /* Tell [MIXER-X]*/
    ADLMIDI_Duration,
    ADLMIDI_SetTempo,   /* Set Tempo multiplier [MIXER-X] */
    ADLMIDI_GetTempo,   /* Get Tempo multiplier [MIXER-X] */
    ADLMIDI_LoopStart,   /* LoopStart [MIXER-X]*/
    ADLMIDI_LoopEnd,   /* LoopEnd [MIXER-X]*/
    ADLMIDI_LoopLength,   /* LoopLength [MIXER-X]*/
    ADLMIDI_GetMetaTag,   /* GetMetaTag [MIXER-X]*/
    NULL,   /* Pause */
    NULL,   /* Resume */
    NULL,   /* Stop */
    ADLMIDI_delete,
    NULL,   /* Close */
    ADLMIDI_Unload
};

/* Same as Mix_MusicInterface_ADLMIDI. Created to play special music formats separately from off MIDI interfaces */
Mix_MusicInterface Mix_MusicInterface_ADLIMF =
{
    "ADLIMF",
    MIX_MUSIC_ADLMIDI,
    MUS_ADLMIDI,
    SDL_FALSE,
    SDL_FALSE,

    ADLMIDI_Load,
    NULL,   /* Open */
    ADLMIDI_new_RW,
    ADLMIDI_new_RWex,   /* CreateFromRWex [MIXER-X]*/
    NULL,   /* CreateFromFile */
    NULL,   /* CreateFromFileEx [MIXER-X]*/
    ADLMIDI_setvolume,
    ADLMIDI_getvolume, /* GetVolume [MIXER-X]*/
    ADLMIDI_play,
    NULL,   /* IsPlaying */
    ADLMIDI_playAudio,
    ADLMIDI_Seek,
    ADLMIDI_Tell,   /* Tell [MIXER-X]*/
    ADLMIDI_Duration,
    ADLMIDI_SetTempo,   /* Set Tempo multiplier [MIXER-X] */
    ADLMIDI_GetTempo,   /* Get Tempo multiplier [MIXER-X] */
    ADLMIDI_LoopStart,   /* LoopStart [MIXER-X]*/
    ADLMIDI_LoopEnd,   /* LoopEnd [MIXER-X]*/
    ADLMIDI_LoopLength,   /* LoopLength [MIXER-X]*/
    ADLMIDI_GetMetaTag,   /* GetMetaTag [MIXER-X]*/
    NULL,   /* GetUserTag [MIXER-X]*/
    NULL,   /* Pause */
    NULL,   /* Resume */
    NULL,   /* Stop */
    ADLMIDI_delete,
    NULL,   /* Close */
    ADLMIDI_Unload
};

#endif /* MUSIC_MID_ADLMIDI */
