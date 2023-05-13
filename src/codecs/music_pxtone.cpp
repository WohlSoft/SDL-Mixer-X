/*
  SDL_mixer:  An audio mixer library based on the SDL library
  Copyright (C) 1997-2023 Sam Lantinga <slouken@libsdl.org>

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
#ifdef MUSIC_PXTONE

#include "music_pxtone.h"

#include "./pxtone/pxtnService.h"
#include "./pxtone/pxtnError.h"


/* This file supports PXTONE music streams */
typedef struct
{
    SDL_RWops *src;
    Sint64 src_start;
    int freesrc;

    int volume;
    double tempo;
    float gain;

    pxtnService *pxtn;
    bool evals_loaded;
    int flags;

    SDL_AudioStream *stream;
    void *buffer;
    size_t buffer_size;
    size_t buffer_samples;
    Mix_MusicMetaTags tags;
} PXTONE_Music;


static bool _pxtn_r(void* user, void* p_dst, Sint32 size, Sint32 num)
{
    return SDL_RWread((SDL_RWops*)user, p_dst, size, num) == (size_t)num;
}

static bool _pxtn_w(void* user,const void* p_dst, Sint32 size, Sint32 num)
{
    return SDL_RWwrite((SDL_RWops*)user, p_dst, size, num) == (size_t)num;
}

static bool _pxtn_s(void* user, Sint32 mode, Sint32 size)
{
    return SDL_RWseek((SDL_RWops*)user, size, mode) >= 0;
}

static bool _pxtn_p(void* user, int32_t* p_pos)
{
    int i = SDL_RWtell((SDL_RWops*)user);
    if( i < 0 ) {
        return false;
    }
    *p_pos = i;
    return true;
}


static void PXTONE_Delete(void *context);

static void *PXTONE_NewRW(struct SDL_RWops *src, int freesrc)
{
    PXTONE_Music *music = NULL;
    const char *name;
    int32_t name_len;
    const char *comment;
    char *temp_string;
    int32_t comment_len;

    music = (PXTONE_Music *)SDL_calloc(1, sizeof *music);
    if (!music) {
        SDL_OutOfMemory();
        return NULL;
    }

    music->tempo = 1.0;
    music->gain = 1.0f;
    music->volume = MIX_MAX_VOLUME;

    music->pxtn = new pxtnService(_pxtn_r, _pxtn_w, _pxtn_s, _pxtn_p);

    if (music->pxtn->init() != pxtnOK ) {
        PXTONE_Delete(music);
        Mix_SetError("PXTONE: Failed to initialize the library");
        return NULL;
    }

    if (!music->pxtn->set_destination_quality(music_spec.channels, music_spec.freq)) {
        PXTONE_Delete(music);
        Mix_SetError("PXTONE: Failed to set the destination quality");
        return NULL;
    }

    // LOAD MUSIC FILE.
    if (music->pxtn->read(src) != pxtnOK) {
        PXTONE_Delete(music);
        Mix_SetError("PXTONE: Failed to set the source stream");
        return NULL;
    }

    if (music->pxtn->tones_ready() != pxtnOK) {
        PXTONE_Delete(music);
        Mix_SetError("PXTONE: Failed to initialize tones");
        return NULL;
    }

    music->evals_loaded = true;

    /* PREPARATION PLAYING MUSIC */
    music->pxtn->moo_get_total_sample();

    pxtnVOMITPREPARATION prep;
    SDL_memset(&prep, 0, sizeof(pxtnVOMITPREPARATION));
    prep.flags          |= pxtnVOMITPREPFLAG_loop | pxtnVOMITPREPFLAG_unit_mute;
    prep.start_pos_float = 0;
    prep.master_volume   = 1.0f;

    if (!music->pxtn->moo_preparation(&prep)) {
        PXTONE_Delete(music);
        Mix_SetError("PXTONE: Failed to initialize the output (Moo)");
        return NULL;
    }

    music->stream = SDL_NewAudioStream(AUDIO_S16SYS, music_spec.channels, music_spec.freq,
                                       music_spec.format, music_spec.channels, music_spec.freq);

    if (!music->stream) {
        PXTONE_Delete(music);
        return NULL;
    }

    music->buffer_samples = music_spec.samples * music_spec.channels;
    music->buffer_size = music->buffer_samples * sizeof(Sint16);
    music->buffer = SDL_malloc(music->buffer_size);
    if (!music->buffer) {
        SDL_OutOfMemory();
        PXTONE_Delete(music);
        return NULL;
    }

    /* Attempt to load metadata */
    music->freesrc = freesrc;

    name = music->pxtn->text->get_name_buf(&name_len);
    if (name) {
        temp_string = SDL_iconv_string("UTF-8", "Shift-JIS", name, name_len + 1);
        meta_tags_set(&music->tags, MIX_META_TITLE, temp_string);
        SDL_free(temp_string);
    }

    comment = music->pxtn->text->get_comment_buf(&comment_len);
    if (comment) {
        temp_string = SDL_iconv_string("UTF-8", "Shift-JIS", comment, comment_len + 1);
        meta_tags_set(&music->tags, MIX_META_COPYRIGHT, temp_string);
        SDL_free(temp_string);
    }

    return music;
}

/* Close the given PXTONE stream */
static void PXTONE_Delete(void *context)
{
    PXTONE_Music *music = (PXTONE_Music*)context;
    if (music) {
        meta_tags_clear(&music->tags);

        if (music->pxtn) {
            if (!music->evals_loaded) {
                music->pxtn->evels->Release();
            }
            music->evals_loaded = false;
            delete music->pxtn;
        }

        if (music->stream) {
            SDL_FreeAudioStream(music->stream);
        }
        if (music->buffer) {
            SDL_free(music->buffer);
        }

        if (music->src && music->freesrc) {
            SDL_RWclose(music->src);
        }
        SDL_free(music);
    }
}

/* Start playback of a given PXTONE stream */
static int PXTONE_Play(void *music_p, int play_count)
{
    pxtnVOMITPREPARATION prep;
    PXTONE_Music *music = (PXTONE_Music*)music_p;

    if (music) {
        SDL_AudioStreamClear(music->stream);
        SDL_memset(&prep, 0, sizeof(pxtnVOMITPREPARATION));
        prep.flags |= pxtnVOMITPREPFLAG_unit_mute;
        if ((play_count < 0) || (play_count > 1)) {
            prep.flags |= pxtnVOMITPREPFLAG_loop;
        }
        music->flags = prep.flags;
        prep.start_pos_float = 0.0f;
        prep.master_volume   = 1.0f;

        if (!music->pxtn->moo_preparation(&prep, music->tempo)) {
            Mix_SetError("PXTONE: Failed to update the output (Moo)");
            return -1;
        }
        music->pxtn->moo_set_loops_num(play_count);
    }
    return 0;
}

static int PXTONE_GetSome(void *context, void *data, int bytes, SDL_bool *done)
{
    PXTONE_Music *music = (PXTONE_Music *)context;
    int filled;
    bool ret;

    filled = SDL_AudioStreamGet(music->stream, data, bytes);
    if (filled != 0) {
        return filled;
    }

    ret = music->pxtn->Moo(music->buffer, music->buffer_size);
    if (!ret) {
        *done = SDL_TRUE;
        return 0;
    }

    if (SDL_AudioStreamPut(music->stream, music->buffer, music->buffer_size) < 0) {
        return -1;
    }

    return 0;
}

/* Play some of a stream previously started with pxtn->moo_preparation() */
static int PXTONE_PlayAudio(void *music_p, void *data, int bytes)
{
    PXTONE_Music *music = (PXTONE_Music*)music_p;
    return music_pcm_getaudio(music_p, data, bytes, music->volume, PXTONE_GetSome);
}

static const char* PXTONE_GetMetaTag(void *context, Mix_MusicMetaTag tag_type)
{
    PXTONE_Music *music = (PXTONE_Music *)context;
    return meta_tags_get(&music->tags, tag_type);
}

/* Set the volume for a PXTONE stream */
static void PXTONE_SetVolume(void *music_p, int volume)
{
    PXTONE_Music *music = (PXTONE_Music *)music_p;
    float v = SDL_floorf(((float)(volume) * music->gain) + 0.5f);
    music->volume = (int)v;
}

/* Get the volume for a PXTONE stream */
static int PXTONE_GetVolume(void *music_p)
{
    PXTONE_Music *music = (PXTONE_Music *)music_p;
    float v = SDL_floorf(((float)(music->volume) / music->gain) + 0.5f);
    return (int)v;
}

/* Jump (seek) to a given position (time is in seconds) */
static int PXTONE_Seek(void *music_p, double time)
{
    pxtnVOMITPREPARATION prep;
    PXTONE_Music *music = (PXTONE_Music*)music_p;

    SDL_memset(&prep, 0, sizeof(pxtnVOMITPREPARATION));
    prep.flags = music->flags;
    prep.start_pos_sample = (int32_t)((time * music_spec.freq) / music->tempo);
    prep.master_volume   = 1.0f;
    if (!music->pxtn->moo_preparation(&prep, music->tempo)) {
        Mix_SetError("PXTONE: Failed to update the setup of output (Moo) for seek");
        return -1;
    }
    return 0;
}

static double PXTONE_Tell(void *music_p)
{
    PXTONE_Music *music = (PXTONE_Music*)music_p;
    int32_t ret = music->pxtn->moo_get_sampling_offset();
    return ((double)ret / music_spec.freq) * music->tempo;
}

static double PXTONE_Duration(void *music_p)
{
    PXTONE_Music *music = (PXTONE_Music*)music_p;
    int32_t ret = music->pxtn->moo_get_total_sample();
    return ret > 0 ? ((double)ret / music_spec.freq) * music->tempo : -1.0;
}

static int PXTONE_SetTempo(void *music_p, double tempo)
{
    PXTONE_Music *music = (PXTONE_Music *)music_p;
    if (music && (tempo > 0.0)) {
        music->tempo = tempo;
        music->pxtn->moo_set_tempo_mod(music->tempo);
        return 0;
    }
    return -1;
}

static double PXTONE_GetTempo(void *music_p)
{
    PXTONE_Music *music = (PXTONE_Music *)music_p;
    if (music) {
        return music->tempo;
    }
    return -1.0;
}

static int PXTONE_GetTracksCount(void *music_p)
{
    PXTONE_Music *music = (PXTONE_Music *)music_p;
    if (music) {
        return music->pxtn->Unit_Num();
    }
    return -1;
}

static int PXTONE_SetTrackMute(void *music_p, int track, int mute)
{
    PXTONE_Music *music = (PXTONE_Music *)music_p;
    if (music) {
        pxtnUnit *u = music->pxtn->Unit_Get_variable( track );
        if (u) {
            u->set_played( !mute );
        }
        return 0;
    }
    return -1;
}

Mix_MusicInterface Mix_MusicInterface_PXTONE =
{
    "PXTONE",
    MIX_MUSIC_FFMPEG,
    MUS_PXTONE,
    SDL_FALSE,
    SDL_FALSE,

    NULL,   /* Load */
    NULL,   /* Open */
    PXTONE_NewRW,
    NULL,   /* CreateFromRWex [MIXER-X]*/
    NULL,   /* CreateFromFile */
    NULL,   /* CreateFromFileEx [MIXER-X]*/
    PXTONE_SetVolume,
    PXTONE_GetVolume,
    PXTONE_Play,
    NULL,   /* IsPlaying */
    PXTONE_PlayAudio,
    NULL,   /* Jump */
    PXTONE_Seek,
    PXTONE_Tell,
    PXTONE_Duration,
    PXTONE_SetTempo,  /* [MIXER-X] */
    PXTONE_GetTempo,  /* [MIXER-X] */
    NULL,   /* SetSpeed [MIXER-X] */
    NULL,   /* GetSpeed [MIXER-X] */
    NULL,   /* SetPitch [MIXER-X] */
    NULL,   /* GetPitch [MIXER-X] */
    PXTONE_GetTracksCount,
    PXTONE_SetTrackMute,
    NULL,   /* LoopStart */
    NULL,   /* LoopEnd */
    NULL,   /* LoopLength */
    PXTONE_GetMetaTag,
    NULL,   /* GetNumTracks */
    NULL,   /* StartTrack */
    NULL,   /* Pause */
    NULL,   /* Resume */
    NULL,   /* Stop */
    PXTONE_Delete,
    NULL,   /* Close */
    NULL    /* Unload */
};

#endif // MUSIC_PXTONE
