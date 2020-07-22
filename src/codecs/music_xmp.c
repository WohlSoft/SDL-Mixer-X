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

#ifdef MUSIC_MOD_XMP

#include "SDL_loadso.h"

#include "music_xmp.h"

#include <xmp.h>

typedef struct {
    int loaded;
    void *handle;

    xmp_context (*xmp_create_context)(void);
    int (*xmp_load_module_from_memory)(xmp_context, void *, long);
    int (*xmp_start_player)(xmp_context, int, int);
    void (*xmp_get_module_info)(xmp_context, struct xmp_module_info *);
    int (*xmp_play_buffer)(xmp_context, void *, int, int);
    int (*xmp_seek_time)(xmp_context, int);
    void (*xmp_get_frame_info)(xmp_context, struct xmp_frame_info *);
    void (*xmp_stop_module)(xmp_context);
    void (*xmp_release_module)(xmp_context);
    void (*xmp_free_context)(xmp_context);
    int (*xmp_set_tempo_factor)(xmp_context, double);
} xmp_loader;

static xmp_loader xmp = {
    0, NULL
};

#ifdef XMP_DYNAMIC
#define FUNCTION_LOADER(FUNC, SIG) \
    xmp.FUNC = (SIG) SDL_LoadFunction(xmp.handle, #FUNC); \
    if (xmp.FUNC == NULL) { SDL_UnloadObject(xmp.handle); return -1; }
#define FUNCTION_LOADER_OPTIONAL(FUNC, SIG) \
    xmp.FUNC = (SIG) SDL_LoadFunction(xmp.handle, #FUNC);
#else
#define FUNCTION_LOADER(FUNC, SIG) \
    xmp.FUNC = FUNC;
#define FUNCTION_LOADER_OPTIONAL(FUNC, SIG) \
    xmp.FUNC = FUNC;
#endif

static int XMP_Load(void)
{
    if (xmp.loaded == 0) {
#ifdef XMP_DYNAMIC
        xmp.handle = SDL_LoadObject(XMP_DYNAMIC);
        if (xmp.handle == NULL) {
            return -1;
        }
#elif defined(__MACOSX__)
        extern xmp_context xmp_create_context(void) __attribute__((weak_import));
        if (xmp_create_context == NULL) {
            /* Missing weakly linked framework */
            Mix_SetError("Missing XMP.framework");
            return -1;
        }
#endif
        FUNCTION_LOADER(xmp_create_context, xmp_context(*)(void))
        FUNCTION_LOADER(xmp_load_module_from_memory, int(*)(xmp_context,void *,long))
        FUNCTION_LOADER(xmp_start_player, int(*)(xmp_context,int,int))
        FUNCTION_LOADER(xmp_get_module_info, void(*)(xmp_context,struct xmp_module_info*))
        FUNCTION_LOADER(xmp_play_buffer, int(*)(xmp_context,void*,int,int))
        FUNCTION_LOADER(xmp_seek_time, int(*)(xmp_context,int))
        FUNCTION_LOADER(xmp_get_frame_info, void(*)(xmp_context,struct xmp_frame_info*))
        FUNCTION_LOADER(xmp_stop_module, void(*)(xmp_context))
        FUNCTION_LOADER(xmp_release_module, void(*)(xmp_context))
        FUNCTION_LOADER(xmp_free_context, void(*)(xmp_context))
#if XMP_VER_MAJOR > 4 || (XMP_VER_MAJOR == 4 && XMP_VER_MINOR >= 5)
        FUNCTION_LOADER_OPTIONAL(xmp_set_tempo_factor, int(*)(xmp_context, double))
#endif
    }
    ++xmp.loaded;

    return 0;
}

static void XMP_Unload(void)
{
    if (xmp.loaded == 0) {
        return;
    }
    if (xmp.loaded == 1) {
#ifdef XMP_DYNAMIC
        SDL_UnloadObject(xmp.handle);
#endif
    }
    --xmp.loaded;
}


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

    music->ctx = xmp.xmp_create_context();
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
        error = xmp.xmp_load_module_from_memory(music->ctx, buffer, (long)size);
        SDL_free(buffer);
        if (error < 0) {
            switch(error) {
            case -XMP_ERROR_FORMAT:
                Mix_SetError("xmp_load_module: Unsupported file format");
                break;
            case -XMP_ERROR_LOAD:
                Mix_SetError("xmp_load_module: Error loading file");
                break;
            case -XMP_ERROR_SYSTEM:
                Mix_SetError("xmp_load_module: System error has occured");
                break;
            default:
                Mix_SetError("xmp_load_module: Can't load file");
                break;
            }
            XMP_Delete(music);
            return NULL;
        }
    }

    error = xmp.xmp_start_player(music->ctx, music_spec.freq, 0);
    if (error < 0) {
        switch(error) {
        case -XMP_ERROR_INVALID:
            Mix_SetError("xmp_start_player: Invalid parameter");
            break;
        case -XMP_ERROR_STATE:
            Mix_SetError("xmp_start_player: Bad state");
            break;
        case -XMP_ERROR_SYSTEM:
            Mix_SetError("xmp_start_player: System error has occured");
            break;
        case -XMP_ERROR_INTERNAL:
        default:
            Mix_SetError("xmp_start_player: Can't start player");
            break;
        }
        XMP_Delete(music);
        return NULL;
    }

    music->has_module = SDL_TRUE;

    meta_tags_init(&music->tags);
    xmp.xmp_get_module_info(music->ctx, &music->mi);
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

    ret = xmp.xmp_play_buffer(music->ctx, music->buffer, music->buffer_size, music->play_count < 0 ? -1 : 0);
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
    xmp.xmp_seek_time(music->ctx, (int)(position * 1000));
    xmp.xmp_play_buffer(music->ctx, NULL, 2, 0);
    return 0;
}

static double XMP_Tell(void *context)
{
    XMP_Music *music = (XMP_Music *)context;
    xmp.xmp_get_frame_info(music->ctx, &music->fi);
    return ((double)music->fi.time) / 1000.0;
}

static double XMP_Duration(void *context)
{
    XMP_Music *music = (XMP_Music *)context;
    xmp.xmp_get_frame_info(music->ctx, &music->fi);
    return ((double)music->fi.total_time) / 1000.0;
}

static int XMP_SetTempo(void *music_p, double tempo)
{
#if XMP_VER_MAJOR > 4 || (XMP_VER_MAJOR == 4 && XMP_VER_MINOR >= 5)
    XMP_Music *music = (XMP_Music *)music_p;
    if (xmp.xmp_set_tempo_factor && music && (tempo > 0.0)) {
        xmp.xmp_set_tempo_factor(music->ctx, (1.0 / tempo));
        music->tempo = tempo;
        return 0;
    }
#else
    (void)music_p;
    (void)tempo;
#endif
    return -1;
}

static double XMP_GetTempo(void *music_p)
{
#if XMP_VER_MAJOR > 4 || (XMP_VER_MAJOR == 4 && XMP_VER_MINOR >= 5)
    XMP_Music *music = (XMP_Music *)music_p;
    if (music) {
        return music->tempo;
    }
#else
    (void)music_p;
#endif
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
            xmp.xmp_stop_module(music->ctx);
            xmp.xmp_release_module(music->ctx);
        }
        xmp.xmp_free_context(music->ctx);
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

    XMP_Load,
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
    XMP_Duration,
    XMP_SetTempo,   /* Set Tempo multiplier [MIXER-X] */
    XMP_GetTempo,   /* Get Tempo multiplier [MIXER-X] */
    NULL,   /* LoopStart [MIXER-X] */
    NULL,   /* LoopEnd [MIXER-X] */
    NULL,   /* LoopLength [MIXER-X]*/
    XMP_GetMetaTag, /* GetMetaTag [MIXER-X]*/
    NULL, /* GetUserTag [MIXER-X]*/
    NULL,   /* Pause */
    NULL,   /* Resume */
    NULL,   /* Stop */
    XMP_Delete,
    NULL,   /* Close */
    XMP_Unload
};

#endif /* MUSIC_MOD_XMP */

/* vi: set ts=4 sw=4 expandtab: */
