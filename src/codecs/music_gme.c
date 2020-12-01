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

#ifdef MUSIC_GME

#include "SDL_loadso.h"

#include "music_gme.h"

#include <gme.h>

typedef struct {
    int loaded;
    void *handle;

    gme_err_t (*gme_open_data)(void const* data, long size, Music_Emu** out, int sample_rate);
    int (*gme_track_count)(Music_Emu const*);
    gme_err_t (*gme_start_track)( Music_Emu*, int index);
    int (*gme_track_ended)( Music_Emu const*);
    void (*gme_set_tempo)(Music_Emu*, double tempo);
#if GME_VERSION >= 0x000700
    void (*gme_set_fade)(Music_Emu*, int start_msec, int fade_msec);
#else
    void (*gme_set_fade)(Music_Emu*, int start_msec);
#endif
    void (*gme_set_autoload_playback_limit)(Music_Emu*, int do_autoload_limit);
    gme_err_t (*gme_track_info)(Music_Emu const*, gme_info_t** out, int track);
    void (*gme_free_info)(gme_info_t*);
    gme_err_t (*gme_seek)(Music_Emu*, int msec);
    int (*gme_tell)(Music_Emu const*);
    gme_err_t (*gme_play)(Music_Emu*, int count, short out[]);
    void (*gme_delete)(Music_Emu*);
} gme_loader;

static gme_loader gme = {
    0, NULL
};

#ifdef GME_DYNAMIC
#define FUNCTION_LOADER(FUNC, SIG) \
    gme.FUNC = (SIG) SDL_LoadFunction(gme.handle, #FUNC); \
    if (gme.FUNC == NULL) { SDL_UnloadObject(gme.handle); return -1; }
#else
#define FUNCTION_LOADER(FUNC, SIG) \
    gme.FUNC = FUNC;
#endif

static int GME_Load(void)
{
    if (gme.loaded == 0) {
#ifdef GME_DYNAMIC
        gme.handle = SDL_LoadObject(GME_DYNAMIC);
        if (gme.handle == NULL) {
            return -1;
        }
#elif defined(__MACOSX__)
        extern gme_err_t gme_open_data(void const*,long,Music_Emu**,int) __attribute__((weak_import));
        if (gme_open_data == NULL) {
            /* Missing weakly linked framework */
            Mix_SetError("Missing GME.framework");
            return -1;
        }
#endif
        FUNCTION_LOADER(gme_open_data, gme_err_t (*)(void const*,long,Music_Emu**,int))
        FUNCTION_LOADER(gme_track_count, int (*)(Music_Emu const*))
        FUNCTION_LOADER(gme_start_track, gme_err_t (*)( Music_Emu*,int))
        FUNCTION_LOADER(gme_track_ended, int (*)( Music_Emu const*))
        FUNCTION_LOADER(gme_set_tempo, void (*)(Music_Emu*,double))
#if GME_VERSION >= 0x000700
        FUNCTION_LOADER(gme_set_fade, void (*)(Music_Emu*,int,int))
#else
        FUNCTION_LOADER(gme_set_fade, void (*)(Music_Emu*,int))
#endif
        FUNCTION_LOADER(gme_track_info, gme_err_t (*)(Music_Emu const*, gme_info_t**, int))
        FUNCTION_LOADER(gme_free_info, void (*)(gme_info_t*))
        FUNCTION_LOADER(gme_seek, gme_err_t (*)(Music_Emu*,int))
        FUNCTION_LOADER(gme_tell, int (*)(Music_Emu const*))
        FUNCTION_LOADER(gme_play, gme_err_t (*)(Music_Emu*, int, short[]))
        FUNCTION_LOADER(gme_delete, void (*)(Music_Emu*))
#if defined(GME_DYNAMIC)
        gme.gme_set_autoload_playback_limit = (void (*)(Music_Emu*,int)) SDL_LoadFunction(gme.handle, "gme_set_autoload_playback_limit");
#elif defined(GME_HAS_SET_AUTOLOAD_PLAYBACK_LIMIT)
        gme.gme_set_autoload_playback_limit = gme_set_autoload_playback_limit;
#else
        gme.gme_set_autoload_playback_limit = NULL;
#endif
    }
    ++gme.loaded;

    return 0;
}

static void GME_Unload(void)
{
    if (gme.loaded == 0) {
        return;
    }
    if (gme.loaded == 1) {
#ifdef GME_DYNAMIC
        SDL_UnloadObject(gme.handle);
#endif
    }
    --gme.loaded;
}

/* Global flags which are applying on initializing of GME player with a file */
typedef struct {
    int track_number;
    double tempo;
    double gain;
} Gme_Setup;

static Gme_Setup gme_setup = {
    0, 1.0, 1.0
};

static void GME_SetDefault(Gme_Setup *setup)
{
    setup->track_number = 0;
    setup->tempo = 1.0;
    setup->gain = 1.0;
}


/* This file supports Game Music Emulator music streams */
typedef struct
{
    int play_count;
    Music_Emu* game_emu;
    SDL_bool has_track_length;
    int track_length;
    int intro_length;
    int loop_length;
    int volume;
    double tempo;
    double gain;
    SDL_AudioStream *stream;
    void *buffer;
    size_t buffer_size;
    Mix_MusicMetaTags tags;
} GME_Music;

static void GME_delete(void *context);

/* Set the volume for a GME stream */
void GME_setvolume(void *music_p, int volume)
{
    GME_Music *music = (GME_Music*)music_p;
    double v = SDL_floor(((double)(volume) * music->gain) + 0.5);
    music->volume = (int)v;
}

/* Get the volume for a GME stream */
int GME_getvolume(void *music_p)
{
    GME_Music *music = (GME_Music*)music_p;
    double v = SDL_floor(((double)(music->volume) / music->gain) + 0.5);
    return (int)v;
}

static void process_args(const char *args, Gme_Setup *setup)
{
#define ARG_BUFFER_SIZE    1024
    char arg[ARG_BUFFER_SIZE];
    char type = '-';
    size_t maxlen = 0;
    size_t i, j = 0;
    /* first value is an integer without prefox sign. So, begin the scan with opened value state */
    int value_opened = 1;

    if (args == NULL) {
        return;
    }
    maxlen = SDL_strlen(args);
    if (maxlen == 0) {
        return;
    }

    maxlen += 1;
    GME_SetDefault(setup);

    for (i = 0; i < maxlen; i++) {
        char c = args[i];
        if (value_opened == 1) {
            if ((c == ';') || (c == '\0')) {
                int value;
                arg[j] = '\0';
                value = SDL_atoi(arg);
                switch(type)
                {
                case '-':
                    setup->track_number = value;
                    break;
                case 't':
                    if (arg[0] == '=') {
                        setup->tempo = SDL_strtod(arg + 1, NULL);
                        if (setup->tempo <= 0.0) {
                            setup->tempo = 1.0;
                        }
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

GME_Music *GME_LoadSongRW(SDL_RWops *src, const char *args)
{
    void *bytes = 0;
    long spcsize, bytes_l;
    unsigned char byte[1];
    gme_info_t *musInfo;
    GME_Music *music;
    Gme_Setup setup = gme_setup;
    const char *err;
    SDL_bool has_loop_length = SDL_TRUE;

    Sint64 length = 0;

    if (src == NULL) {
        return NULL;
    }

    process_args(args, &setup);

    music = (GME_Music *)SDL_calloc(1, sizeof(GME_Music));

    music->tempo = setup.tempo;
    music->gain = setup.gain;

    music->stream = SDL_NewAudioStream(AUDIO_S16, 2, music_spec.freq,
                                       music_spec.format, music_spec.channels, music_spec.freq);
    if (!music->stream) {
        GME_delete(music);
        return NULL;
    }

    music->buffer_size = music_spec.samples * sizeof(Sint16) * 2/*channels*/ * music_spec.channels;
    music->buffer = SDL_malloc(music->buffer_size);
    if (!music->buffer) {
        SDL_OutOfMemory();
        GME_delete(music);
        return NULL;
    }

    length = SDL_RWseek(src, 0, RW_SEEK_END);
    if (length < 0) {
        GME_delete(music);
        Mix_SetError("GAME-EMU: wrong file\n");
        return NULL;
    }

    SDL_RWseek(src, 0, RW_SEEK_SET);
    bytes = SDL_malloc((size_t)length);
    if (!bytes) {
        SDL_OutOfMemory();
        GME_delete(music);
        return NULL;
    }

    spcsize = 0;
    while ((bytes_l = (long)SDL_RWread(src, &byte, sizeof(unsigned char), 1)) != 0) {
        ((unsigned char *)bytes)[spcsize] = byte[0];
        spcsize++;
    }

    if (spcsize == 0) {
        GME_delete(music);
        Mix_SetError("GAME-EMU: wrong file\n");
        return NULL;
    }

    err = gme.gme_open_data(bytes, spcsize, &music->game_emu, music_spec.freq);
    SDL_free(bytes);
    if (err != 0) {
        GME_delete(music);
        Mix_SetError("GAME-EMU: %s", err);
        return NULL;
    }

    if ((setup.track_number < 0) || (setup.track_number >= gme.gme_track_count(music->game_emu))) {
        setup.track_number = gme.gme_track_count(music->game_emu) - 1;
    }

    /* Set this flag BEFORE calling the gme_start_track() to fix an inability to loop forever */
    if(gme.gme_set_autoload_playback_limit)
        gme.gme_set_autoload_playback_limit(music->game_emu, 0);

    err = gme.gme_start_track(music->game_emu, setup.track_number);
    if (err != 0) {
        GME_delete(music);
        Mix_SetError("GAME-EMU: %s", err);
        return NULL;
    }

    gme.gme_set_tempo(music->game_emu, music->tempo);

    music->volume = MIX_MAX_VOLUME;
    meta_tags_init(&music->tags);

    err = gme.gme_track_info(music->game_emu, &musInfo, setup.track_number);
    if (err != 0) {
        GME_delete(music);
        Mix_SetError("GAME-EMU: %s", err);
        return NULL;
    }

    music->track_length = musInfo->length;
    music->intro_length = musInfo->intro_length;
    music->loop_length = musInfo->loop_length;

    music->has_track_length = SDL_TRUE;
    if (music->track_length <= 0 ) {
        music->track_length = (int)(2.5 * 60 * 1000);
        music->has_track_length = SDL_FALSE;
    }

    if (music->intro_length < 0 ) {
        music->intro_length = 0;
    }
    if (music->loop_length <= 0 ) {
        if (music->track_length > 0) {
            music->loop_length = music->track_length;
        } else {
            music->loop_length = (int)(2.5 * 60 * 1000);
        }
        has_loop_length = SDL_FALSE;
    }

    if (!music->has_track_length && has_loop_length) {
        music->track_length = music->intro_length + music->loop_length;
        music->has_track_length = SDL_TRUE;
    }

    meta_tags_set(&music->tags, MIX_META_TITLE, musInfo->song);
    meta_tags_set(&music->tags, MIX_META_ARTIST, musInfo->author);
    meta_tags_set(&music->tags, MIX_META_ALBUM, musInfo->game);
    meta_tags_set(&music->tags, MIX_META_COPYRIGHT, musInfo->copyright);
    gme.gme_free_info(musInfo);

    return music;
}

/* Load a Game Music Emulators stream from an SDL_RWops object */
static void *GME_new_RWEx(struct SDL_RWops *src, int freesrc, const char *extraSettings)
{
    GME_Music *gmeMusic;

    gmeMusic = GME_LoadSongRW(src, extraSettings);
    if (!gmeMusic) {
        Mix_SetError("GAME-EMU: Can't load file");
        return NULL;
    }
    if (freesrc) {
        SDL_RWclose(src);
    }
    return gmeMusic;
}

static void *GME_new_RW(struct SDL_RWops *src, int freesrc)
{
    return GME_new_RWEx(src, freesrc, "0");
}

/* Start playback of a given Game Music Emulators stream */
static int GME_play(void *music_p, int play_count)
{
    GME_Music *music = (GME_Music*)music_p;
    int fade_start;
    if (music) {
        music->play_count = play_count;
        fade_start = play_count > 0 ? music->intro_length + (music->loop_length * play_count) : -1;
#if GME_VERSION >= 0x000700
        gme.gme_set_fade(music->game_emu, fade_start, 8000);
#else
        gme.gme_set_fade(music->game_emu, fade_start);
#endif
        gme.gme_seek(music->game_emu, 0);
    }
    return 0;
}

static int GME_GetSome(void *context, void *data, int bytes, SDL_bool *done)
{
    GME_Music *music = (GME_Music*)context;
    int filled;
    const char *err = NULL;

    filled = SDL_AudioStreamGet(music->stream, data, bytes);
    if (filled != 0) {
        return filled;
    }

    if (gme.gme_track_ended(music->game_emu)) {
        /* All done */
        *done = SDL_TRUE;
        return 0;
    }

    /* Align bytes length to correctly capture a stereo input */
    if ((bytes % 4) != 0) {
        bytes += (4 - (bytes % 4));
    }

    err = gme.gme_play(music->game_emu, (bytes / 2), (short*)music->buffer);
    if (err != NULL) {
        Mix_SetError("GAME-EMU: %s", err);
        return 0;
    }

    if (SDL_AudioStreamPut(music->stream, music->buffer, bytes) < 0) {
        return -1;
    }
    return 0;
}

/* Play some of a stream previously started with GME_play() */
static int GME_playAudio(void *music_p, void *data, int bytes)
{
    GME_Music *music = (GME_Music*)music_p;
    return music_pcm_getaudio(music_p, data, bytes, music->volume, GME_GetSome);
}

/* Close the given Game Music Emulators stream */
static void GME_delete(void *context)
{
    GME_Music *music = (GME_Music*)context;
    if (music) {
        meta_tags_clear(&music->tags);
        if (music->game_emu) {
            gme.gme_delete(music->game_emu);
            music->game_emu = NULL;
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

static const char* GME_GetMetaTag(void *context, Mix_MusicMetaTag tag_type)
{
    GME_Music *music = (GME_Music *)context;
    return meta_tags_get(&music->tags, tag_type);
}

/* Jump (seek) to a given position (time is in seconds) */
static int GME_Seek(void *music_p, double time)
{
    GME_Music *music = (GME_Music*)music_p;
    gme.gme_seek(music->game_emu, (int)(SDL_floor((time * 1000.0) + 0.5)));
    return 0;
}

static double GME_Tell(void *music_p)
{
    GME_Music *music = (GME_Music*)music_p;
    return (double)(gme.gme_tell(music->game_emu)) / 1000.0;
}

static double GME_Duration(void *music_p)
{
    GME_Music *music = (GME_Music*)music_p;
    if (music->has_track_length) {
        return (double)(music->track_length) / 1000.0;
    } else {

        return -1.0;
    }
}

static int GME_SetTempo(void *music_p, double tempo)
{
    GME_Music *music = (GME_Music *)music_p;
    if (music && (tempo > 0.0)) {
        gme.gme_set_tempo(music->game_emu, tempo);
        music->tempo = tempo;
        return 0;
    }
    return -1;
}

static double GME_GetTempo(void *music_p)
{
    GME_Music *music = (GME_Music *)music_p;
    if (music) {
        return music->tempo;
    }
    return -1.0;
}

Mix_MusicInterface Mix_MusicInterface_GME =
{
    "GME",
    MIX_MUSIC_GME,
    MUS_GME,
    SDL_FALSE,
    SDL_FALSE,

    GME_Load,
    NULL,   /* Open */
    GME_new_RW,
    GME_new_RWEx,   /* CreateFromRWex [MIXER-X]*/
    NULL,   /* CreateFromFile */
    NULL,   /* CreateFromFileEx [MIXER-X]*/
    GME_setvolume,
    GME_getvolume,   /* GetVolume [MIXER-X]*/
    GME_play,
    NULL,   /* IsPlaying */
    GME_playAudio,
    GME_Seek,   /* Seek */
    GME_Tell,   /* Tell [MIXER-X]*/
    GME_Duration,
    GME_SetTempo,   /* Set Tempo multiplier [MIXER-X] */
    GME_GetTempo,   /* Get Tempo multiplier [MIXER-X] */
    NULL,   /* LoopStart [MIXER-X]*/
    NULL,   /* LoopEnd [MIXER-X]*/
    NULL,   /* LoopLength [MIXER-X]*/
    GME_GetMetaTag,/* GetMetaTag [MIXER-X]*/
    NULL,   /* Pause */
    NULL,   /* Resume */
    NULL,   /* Stop */
    GME_delete,
    NULL,   /* Close */
    GME_Unload
};

#endif /* MUSIC_GME */
