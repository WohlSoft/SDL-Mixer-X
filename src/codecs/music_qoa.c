/*
  SDL_mixer:  An audio mixer library based on the SDL library
  Copyright (C) 1997-2025 Sam Lantinga <slouken@libsdl.org>

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

#ifdef MUSIC_QOA

#include "music_qoa.h"

#define QOA_IMPLEMENTATION
#define QOA_NO_STDIO
#define QOA_MALLOC  SDL_malloc
#define QOA_FREE    SDL_free
#define inline      SDL_INLINE
#include "qoa/qoa.h"
#undef inline


typedef struct
{
    int volume;
    int num_channels;
    int play_count;

    qoa_desc info;
    Uint32 first_frame_pos;
    Uint32 sample_pos;
    Uint32 sample_data_pos;
    Uint32 sample_data_len;
    Uint32 skip_samples;

    SDL_AudioStream *stream;

    void *decode_buffer;
    int decode_buffer_size;

    Sint16 *sample_data;
    Uint32 sample_data_size;

    void *buffer;
    int buffer_size;

    SDL_RWops *src;
    int freesrc;

    int loop;
    Uint32 loop_start;
    Uint32 loop_end;
    Uint32 loop_len;
    Mix_MusicMetaTags tags;
} QOA_Music;


static int QOA_Seek(void *ctx, double pos);
static void QOA_Delete(void *ctx);
static void QOA_CleanUp(SDL_RWops *src, QOA_Music *music);

/* Load a libxmp stream from an SDL_RWops object */
void *QOA_CreateFromRW(SDL_RWops *src, int freesrc)
{
    QOA_Music *music;
    unsigned char header[QOA_MIN_FILESIZE];

    music = (QOA_Music *)SDL_calloc(1, sizeof(*music));
    if (!music) {
        SDL_OutOfMemory();
        return NULL;
    }

    music->src = src;
    music->freesrc = freesrc;

    /*TODO: Implement the support of XQOA with loop points and meta-tags. */

    if (SDL_RWread(src, header, 1, QOA_MIN_FILESIZE) != QOA_MIN_FILESIZE) {
        Mix_SetError("QOA: Can't read header.");
        QOA_CleanUp(src, music);
        return NULL;
    }

    music->first_frame_pos = qoa_decode_header(header, QOA_MIN_FILESIZE, &music->info);
    if (!music->first_frame_pos) {
        Mix_SetError("QOA: Can't parse header.");
        QOA_CleanUp(src, music);
        return NULL;
    }

    music->volume = MIX_MAX_VOLUME;
    music->num_channels = music->info.channels;

    SDL_RWseek(src, music->first_frame_pos, SEEK_SET);
    music->sample_pos = 0;
    music->sample_data_len = 0;
    music->sample_data_pos = 0;

    music->decode_buffer_size = qoa_max_frame_size(&music->info);
    music->decode_buffer = SDL_malloc((size_t)music->decode_buffer_size);
    if (!music->decode_buffer) {
        SDL_OutOfMemory();
        QOA_CleanUp(src, music);
        return NULL;
    }

    music->sample_data_size = music->info.channels * QOA_FRAME_LEN * sizeof(Sint16) * 2;
    music->sample_data = (Sint16*)SDL_malloc((size_t)music->sample_data_size);
    if (!music->sample_data) {
        SDL_OutOfMemory();
        QOA_CleanUp(src, music);
        return NULL;
    }

    music->buffer_size = music_spec.samples * sizeof(Sint16) * music->info.channels;
    music->buffer = SDL_malloc((size_t)music->buffer_size);
    if (!music->buffer) {
        SDL_OutOfMemory();
        QOA_CleanUp(src, music);
        return NULL;
    }

    music->stream = SDL_NewAudioStream(AUDIO_S16SYS, music->info.channels, music->info.samplerate,
                                       music_spec.format, music_spec.channels, music_spec.freq);
    if (!music->stream) {
        Mix_SetError("QOA: Can't initialize stream.");
        QOA_CleanUp(src, music);
        return NULL;
    }

    return music;
}

/* Set the volume for a libxmp stream */
static void QOA_SetVolume(void *context, int volume)
{
    QOA_Music *music = (QOA_Music *)context;
    music->volume = volume;
}

/* Get the volume for a libxmp stream */
static int QOA_GetVolume(void *context)
{
    QOA_Music *music = (QOA_Music *)context;
    return music->volume;
}

/* Start playback of a given libxmp stream */
static int QOA_Play(void *context, int play_count)
{
    QOA_Music *music = (QOA_Music *)context;
    music->play_count = play_count;
    SDL_RWseek(music->src, music->first_frame_pos, SEEK_SET);
    music->sample_pos = 0;
    music->sample_data_len = 0;
    music->sample_data_pos = 0;
    return 0;
}

/* Clean-up the output buffer */
static void QOA_Stop(void *context)
{
    QOA_Music *music = (QOA_Music *)context;
    SDL_AudioStreamClear(music->stream);
}

static unsigned int qoaplay_decode_frame(QOA_Music *music)
{
    unsigned int samples_decoded = 0;
    size_t got_bytes, encoded_frame_size;

    encoded_frame_size = qoa_max_frame_size(&music->info);

    got_bytes = SDL_RWread(music->src, music->decode_buffer, 1, encoded_frame_size);

    qoa_decode_frame(music->decode_buffer, got_bytes, &music->info, music->sample_data, &samples_decoded);

    /* required a multiple of 4 */
    samples_decoded &= ~3;

    music->sample_data_pos = 0;
    music->sample_data_len = samples_decoded;

    return samples_decoded;
}

static int qoaplay_decode(QOA_Music *music, Sint16 *out_samples, int num_samples)
{
    int src_index = music->sample_data_pos * music->info.channels;
    int frame_size = (sizeof(Sint16) * music->num_channels);
    int samples_written = 0;
    int i, to_copy, samples_left;

    for (i = 0; i < num_samples; ) {
        /* Do we have to decode more samples? */
        if (music->sample_data_len - music->sample_data_pos == 0) {
            if (!qoaplay_decode_frame(music)) {
                break; /* Reached end of file */
            }
            src_index = 0;
        }

        /* Skip samples to reach the seek destination */
        if (music->skip_samples > 0) {
            music->sample_data_pos += music->skip_samples;
            music->sample_pos += music->skip_samples;
            src_index += music->skip_samples * music->info.channels;
            music->skip_samples = 0;
        }

        samples_left = (int)music->sample_data_len - music->sample_data_pos;

        if (samples_left <= num_samples - i) {
            to_copy = samples_left;
        } else {
            to_copy = num_samples - i;
        }

        if (to_copy <= 0) {
            break; /* Something went wrong... */
        }

        SDL_memcpy(out_samples, music->sample_data + src_index, to_copy * frame_size);
        out_samples += to_copy * music->info.channels;
        src_index += to_copy * music->info.channels;
        music->sample_data_pos += to_copy;
        music->sample_pos += to_copy;
        samples_written += to_copy;

        i += to_copy;
    }

    return samples_written;
}

/* Play some of a stream previously started with xmp_play() */
static int QOA_GetSome(void *context, void *data, int bytes, SDL_bool *done)
{
    QOA_Music *music = (QOA_Music *)context;
    int filled, amount;
    int frame_size = (sizeof(Sint16) * music->num_channels);

    filled = SDL_AudioStreamGet(music->stream, data, bytes);
    if (filled != 0) {
        return filled;
    }

    if (!music->play_count) {
        /* All done */
        *done = SDL_TRUE;
        return 0;
    }

    amount = qoaplay_decode(music, (Sint16*)music->buffer, music->buffer_size / frame_size);
    amount *= frame_size;

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
            if (QOA_Play(music, play_count) < 0) {
                return -1;
            }
        }
    }

    return 0;
}
static int QOA_GetAudio(void *context, void *data, int bytes)
{
    QOA_Music *music = (QOA_Music *)context;
    return music_pcm_getaudio(context, data, bytes, music->volume, QOA_GetSome);
}

/* Jump (seek) to a given position */
static int QOA_Seek(void *context, double pos)
{
    QOA_Music *music = (QOA_Music *)context;
    int dst_pos, dst_frame;
    Sint64 offset;

    dst_pos = (int)(pos * music->info.samplerate);

    if (dst_pos < 0) {
        dst_pos = 0;
    }

    if (dst_pos > (int)music->info.samples) {
        dst_pos = 0;
    }

    dst_frame = dst_pos / QOA_FRAME_LEN;

    music->sample_pos = dst_frame * QOA_FRAME_LEN;
    music->sample_data_len = 0;
    music->sample_data_pos = 0;

    offset = music->first_frame_pos + dst_frame * qoa_max_frame_size(&music->info);

    SDL_RWseek(music->src, offset, SEEK_SET);

    music->skip_samples = dst_pos - (dst_frame * QOA_FRAME_LEN);

    return -1;
}

static double QOA_Tell(void *context)
{
    QOA_Music *music = (QOA_Music *)context;
    return (double)music->sample_pos / (double)music->info.samplerate;
}

static double QOA_Duration(void *context)
{
    QOA_Music *music = (QOA_Music *)context;
    return (double)music->info.samples / (double)music->info.samplerate;
}

static const char* QOA_GetMetaTag(void *context, Mix_MusicMetaTag tag_type)
{
    QOA_Music *music = (QOA_Music *)context;
    return meta_tags_get(&music->tags, tag_type);
}

static void QOA_CleanUp(SDL_RWops *src, QOA_Music *music)
{
    meta_tags_clear(&music->tags);

    if (music->buffer) {
        SDL_free(music->buffer);
    }

    if (music->sample_data) {
        SDL_free(music->sample_data);
    }

    if (music->decode_buffer) {
        SDL_free(music->decode_buffer);
    }

    if (music) {
        SDL_free(music);
    }

    SDL_RWseek(src, 0, SEEK_SET);
}

/* Close the given libxmp stream */
static void QOA_Delete(void *context)
{
    QOA_Music *music = (QOA_Music *)context;
    SDL_RWops *src = music->src;
    int freesrc = music->freesrc;

    QOA_CleanUp(src, music);

    if (freesrc) {
        SDL_RWclose(src);
    }
}

Mix_MusicInterface Mix_MusicInterface_QOA =
{
    "QOA",
    MIX_MUSIC_QOA,
    MUS_QOA,
    SDL_FALSE,
    SDL_FALSE,

    NULL,   /* Load */
    NULL,   /* Open */
    QOA_CreateFromRW,
    NULL,   /* CreateFromRWex [MIXER-X]*/
    NULL,   /* CreateFromFile */
    NULL,   /* CreateFromFileEx [MIXER-X]*/
    QOA_SetVolume,
    QOA_GetVolume,
    NULL,   /* SetGain [MIXER-X]*/
    NULL,   /* GetGain [MIXER-X]*/
    QOA_Play,
    NULL,   /* IsPlaying */
    QOA_GetAudio,
    NULL,   /* Jump */
    QOA_Seek,
    QOA_Tell,
    QOA_Duration,
    NULL,   /* SetTempo [MIXER-X] */
    NULL,   /* GetTemp [MIXER-X] */
    NULL,   /* SetSpeed [MIXER-X] */
    NULL,   /* GetSpeed [MIXER-X] */
    NULL,   /* SetPitch [MIXER-X] */
    NULL,   /* GetPitch [MIXER-X] */
    NULL,   /* GetTracksCount [MIXER-X] */
    NULL,   /* SetTrackMute [MIXER-X] */
    NULL,   /* LoopStart */
    NULL,   /* LoopEnd */
    NULL,   /* LoopLength */
    QOA_GetMetaTag,
    NULL,   /* GetNumTracks */
    NULL,   /* StartTrack */
    NULL,   /* Pause */
    NULL,   /* Resume */
    QOA_Stop,
    QOA_Delete,
    NULL,   /* Close */
    NULL    /* Unload */
};

#endif /* MUSIC_QOA */

/* vi: set ts=4 sw=4 expandtab: */
