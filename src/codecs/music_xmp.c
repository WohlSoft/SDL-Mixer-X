/*
  SDL_mixer:  An audio mixer library based on the SDL library
  Copyright (C) 1997-2019 Sam Lantinga <slouken@libsdl.org>

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

#ifdef MUSIC_MOD_XMP

#include "SDL_loadso.h"

#include "music_xmp.h"

#include <xmp/xmp.h>

typedef struct
{
    int volume;
    double tempo;
    int play_count;
    struct xmp_module_info mi;
    struct xmp_frame_info fi;
    xmp_context ctx;
    int has_module;
    SDL_AudioStream *stream;
    void *buffer;
    int buffer_size;
    Mix_MusicMetaTags tags;
} XMP_Music;


static int XMP_Seek(void *context, double position);
static void XMP_Delete(void *context);

/* Load a modplug stream from an SDL_RWops object */
void *XMP_CreateFromRW(SDL_RWops *src, int freesrc)
{
    XMP_Music *music;
    void *buffer;
    size_t size;
    int error;

    music = (XMP_Music *)SDL_calloc(1, sizeof(*music));
    if (!music) {
        SDL_OutOfMemory();
        return NULL;
    }

    music->ctx = NULL;
    music->has_module = SDL_FALSE;

    music->volume = MIX_MAX_VOLUME;
    music->tempo = 1.0;

    music->stream = SDL_NewAudioStream(AUDIO_S16, 2, music_spec.freq,
                                       music_spec.format, music_spec.channels, music_spec.freq);
    if (!music->stream) {
        XMP_Delete(music);
        return NULL;
    }

    music->ctx = xmp_create_context();
    if (!music->ctx) {
        XMP_Delete(music);
        return NULL;
    }

    music->buffer_size = music_spec.samples * 2 * 2;
    music->buffer = SDL_malloc((size_t)music->buffer_size);
    if (!music->buffer) {
        SDL_OutOfMemory();
        XMP_Delete(music);
        return NULL;
    }

    buffer = SDL_LoadFile_RW(src, &size, SDL_FALSE);
    if (buffer) {
        error = xmp_load_module_from_memory(music->ctx, buffer, (long)size);
        if (error < 0) {
            switch(error) {
            case -XMP_ERROR_FORMAT:
                Mix_SetError("xmp_load_module failed: Unsupported file format");
                break;
            case -XMP_ERROR_LOAD:
                Mix_SetError("xmp_load_module failed: Error loading file");
                break;
            case -XMP_ERROR_SYSTEM:
                Mix_SetError("xmp_load_module failed: System error has occured");
                break;
            default:
                Mix_SetError("xmp_load_module failed: Can't load file");
                break;
            }
        }
        SDL_free(buffer);
    }

    if (xmp_start_player(music->ctx, music_spec.freq, 0) != 0) {
        XMP_Delete(music);
        return NULL;
    }

    music->has_module = SDL_TRUE;

    meta_tags_init(&music->tags);
    xmp_get_module_info(music->ctx, &music->mi);
    if (music->mi.comment) {
        meta_tags_set(&music->tags, MIX_META_TITLE, music->mi.comment);
    }

    if (freesrc) {
        SDL_RWclose(src);
    }
    return music;
}

/* Set the volume for a modplug stream */
static void XMP_SetVolume(void *context, int volume)
{
    XMP_Music *music = (XMP_Music *)context;
    music->volume = volume;
}

/* Get the volume for a modplug stream */
static int XMP_GetVolume(void *context)
{
    XMP_Music *music = (XMP_Music *)context;
    return music->volume;
}

/* Start playback of a given modplug stream */
static int XMP_Play(void *context, int play_count)
{
    XMP_Music *music = (XMP_Music *)context;
    music->play_count = play_count;
    return XMP_Seek(music, 0.0);
}

/* Play some of a stream previously started with modplug_play() */
static int XMP_GetSome(void *context, void *data, int bytes, SDL_bool *done)
{
    XMP_Music *music = (XMP_Music *)context;
    int filled, amount, ret;

    filled = SDL_AudioStreamGet(music->stream, data, bytes);
    if (filled != 0) {
        return filled;
    }

    if (!music->play_count) {
        /* All done */
        *done = SDL_TRUE;
        return 0;
    }

    ret = xmp_play_buffer(music->ctx, music->buffer, music->buffer_size, music->play_count < 0 ? -1 : 0);
    amount = music->buffer_size;

    if (ret == 0) {
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
            if (XMP_Play(music, play_count) < 0) {
                return -1;
            }
        }
    }
    return 0;
}
static int XMP_GetAudio(void *context, void *data, int bytes)
{
    XMP_Music *music = (XMP_Music *)context;
    return music_pcm_getaudio(context, data, bytes, music->volume, XMP_GetSome);
}

/* Jump (seek) to a given position */
static int XMP_Seek(void *context, double position)
{
    XMP_Music *music = (XMP_Music *)context;
    char dummy[2];
    xmp_seek_time(music->ctx, (int)(position*1000));
    xmp_play_buffer(music->ctx, dummy, 2, 0);
    return 0;
}

static double XMP_Tell(void *context)
{
    XMP_Music *music = (XMP_Music *)context;
    xmp_get_frame_info(music->ctx, &music->fi);
    return ((double)music->fi.time) / 1000.0;
}

static double XMP_Length(void *context)
{
    XMP_Music *music = (XMP_Music *)context;
    xmp_get_frame_info(music->ctx, &music->fi);
    return ((double)music->fi.total_time) / 1000.0;
}

static int XMP_setTempo(void *music_p, double tempo)
{
    XMP_Music *music = (XMP_Music *)music_p;
    if (music && (tempo > 0.0)) {
        xmp_set_tempo_factor(music->ctx, (1.0 / tempo));
        music->tempo = tempo;
        return 0;
    }
    return -1;
}

static double XMP_getTempo(void *music_p)
{
    XMP_Music *music = (XMP_Music *)music_p;
    if (music) {
        return music->tempo;
    }
    return -1.0;
}

static const char* XMP_GetMetaTag(void *context, Mix_MusicMetaTag tag_type)
{
    XMP_Music *music = (XMP_Music *)context;
    return meta_tags_get(&music->tags, tag_type);
}

/* Close the given modplug stream */
static void XMP_Delete(void *context)
{
    XMP_Music *music = (XMP_Music *)context;
    meta_tags_clear(&music->tags);
    if (music->ctx) {
        if (music->has_module) {
            xmp_stop_module(music->ctx);
            xmp_release_module(music->ctx);
        }
        xmp_free_context(music->ctx);
    }
    if (music->stream) {
        SDL_FreeAudioStream(music->stream);
    }
    if (music->buffer) {
        SDL_free(music->buffer);
    }
    SDL_free(music);
}

Mix_MusicInterface Mix_MusicInterface_LIBXMP =
{
    "LIBXMP",
    MIX_MUSIC_LIBXMP,
    MUS_MOD,
    SDL_FALSE,
    SDL_FALSE,

    NULL, /* Load */
    NULL, /* Open */
    XMP_CreateFromRW,
    NULL,   /* CreateFromRWex [MIXER-X]*/
    NULL,   /* CreateFromFile */
    NULL,   /* CreateFromFileEx [MIXER-X]*/
    XMP_SetVolume,
    XMP_GetVolume,   /* GetVolume [MIXER-X]*/
    XMP_Play,
    NULL,   /* IsPlaying */
    XMP_GetAudio,
    XMP_Seek,
    XMP_Tell, /* Tell [MIXER-X]*/
    XMP_Length, /* FullLength [MIXER-X]*/
    XMP_setTempo,   /* Set Tempo multiplier [MIXER-X] */
    XMP_getTempo,   /* Get Tempo multiplier [MIXER-X] */
    NULL,   /* LoopStart [MIXER-X] */
    NULL,   /* LoopEnd [MIXER-X] */
    NULL,   /* LoopLength [MIXER-X]*/
    XMP_GetMetaTag, /* GetMetaTag [MIXER-X]*/
    NULL,   /* Pause */
    NULL,   /* Resume */
    NULL,   /* Stop */
    XMP_Delete,
    NULL,   /* Close */
    NULL,   /* Unload */
};

#endif /* MUSIC_MOD_XMP */

/* vi: set ts=4 sw=4 expandtab: */
