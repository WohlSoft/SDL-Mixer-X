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
/*Workaround to avoid conflicts*/
#define qoa_encode_header   mixerx_private_qoa_encode_header
#define qoa_encode_frame    mixerx_private_qoa_encode_frame
#define qoa_encode          mixerx_private_qoa_encode
#define qoa_max_frame_size  mixerx_private_qoa_max_frame_size
#define qoa_decode_header   mixerx_private_qoa_decode_header
#define qoa_decode_frame    mixerx_private_qoa_decode_frame
#define qoa_decode          mixerx_private_qoa_decode
#include "qoa/qoa.h"
#undef inline

#ifdef USE_CUSTOM_AUDIO_STREAM
#   include "stream_custom.h"
#endif

#ifndef PRIu32
#   define PRIu32 "u"
#endif

/* Global flags which are applying on initializing of Vorbis player with a file */
typedef struct {
    double speed;
} QOAVorbis_Setup;

static QOAVorbis_Setup qoa_setup = {
    1.0
};

static void QOAVorbis_SetDefault(QOAVorbis_Setup *setup)
{
    setup->speed = 1.0;
}

typedef struct
{
    int volume;
    int num_channels;
    int play_count;

    qoa_desc info;
    Uint32 xqoa_data_offset;
    Uint32 xqoa_data_size;
    Uint32 first_frame_pos;
    SDL_bool at_end;
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

    int computed_src_rate;
    double speed;

    SDL_bool multitrack;
    Uint32 multitrack_mute[256];
    Uint32 multitrack_channels;
    Uint32 multitrack_tracks;

    Mix_MusicMetaTags tags;
} QOA_Music;


static int QOA_Seek(void *ctx, double pos);
static void QOA_Delete(void *ctx);
static void QOA_CleanUp(SDL_RWops *src, QOA_Music *music);

static int QOA_UpdateSpeed(QOA_Music *music)
{
    if (music->computed_src_rate != -1) {
        return 0;
    }

    music->computed_src_rate = music->info.samplerate * music->speed;
    if (music->computed_src_rate < 1000) {
        music->computed_src_rate = 1000;
    }

    if (music->stream) {
        SDL_FreeAudioStream(music->stream);
    }

    music->stream = SDL_NewAudioStream(AUDIO_S16SYS, (Uint8)(music->multitrack ? music->multitrack_channels : music->info.channels), music->computed_src_rate,
                                       music_spec.format, music_spec.channels, music_spec.freq);
    if (!music->stream) {
        return -1;
    }

    return 0;
}

static SDL_bool _XQOA_ReadMetaTag(QOA_Music *music, const char *tag_name, Mix_MusicMetaTag tag_type)
{
    Uint8 read_buf[4];
    char *tag_data = NULL;
    Uint32 tag_size = 0;

    if (SDL_RWread(music->src, read_buf, 1, 4) != 4) {
        Mix_SetError("XQOA: Can't read %s meta-tag size.", tag_name);
        return SDL_FALSE;
    }
    tag_size = SDL_SwapBE32(*(Uint32*)read_buf);

    if (tag_size > 0) {
        tag_data = (char *)SDL_calloc(1, tag_size + 1);
        if (!tag_data) {
            SDL_OutOfMemory();
            return SDL_FALSE;
        }

        if (SDL_RWread(music->src, tag_data, 1, tag_size) != tag_size) {
            Mix_SetError("XQOA: Can't read the %s meta-tag data.", tag_name);
            SDL_free(tag_data);
            return SDL_FALSE;
        }

        meta_tags_set(&music->tags, tag_type, tag_data);
        SDL_free(tag_data);
    }

    return SDL_TRUE;
}

static void process_args(const char *args, QOAVorbis_Setup *setup)
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
    QOAVorbis_SetDefault(setup);

    for (i = 0; i < maxlen; i++) {
        char c = args[i];
        if (value_opened == 1) {
            if ((c == ';') || (c == '\0')) {
                /* int value = 0; */
                arg[j] = '\0';
                /* value = SDL_atoi(arg); */
                switch(type)
                {
                case 's':
                    if (arg[0] == '=') {
                        setup->speed = SDL_strtod(arg + 1, NULL);
                        if (setup->speed < 0.0) {
                            setup->speed = 1.0;
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

/* Load a libxmp stream from an SDL_RWops object */
void *QOA_CreateFromRWex(SDL_RWops *src, int freesrc, const char *args)
{
    QOA_Music *music;
    Uint8 header[QOA_MIN_FILESIZE];
    Uint8 read_buf[4];
    Uint32 xqoa_head_size;
    Uint32 xqoa_data_size;
    Uint8 in_channels;
    QOAVorbis_Setup setup = qoa_setup;

    music = (QOA_Music *)SDL_calloc(1, sizeof(*music));
    if (!music) {
        SDL_OutOfMemory();
        return NULL;
    }

    music->src = src;
    music->xqoa_data_offset = 0;
    music->xqoa_data_size = SDL_RWsize(src);
    music->volume = MIX_MAX_VOLUME;

    process_args(args, &setup);

    music->speed = setup.speed;

    if (SDL_RWread(src, header, 1, QOA_MIN_FILESIZE) != QOA_MIN_FILESIZE) {
        Mix_SetError("QOA: Can't read header.");
        QOA_CleanUp(src, music);
        return NULL;
    }

    /* Extra header that supports more features */
    if (SDL_memcmp(header, "XQOA", 4) == 0) {
        SDL_RWseek(src, 4, RW_SEEK_SET);

        if (SDL_RWread(src, read_buf, 1, 4) != 4) {
            Mix_SetError("XQOA: Can't read header size field.");
            QOA_CleanUp(src, music);
            return NULL;
        }
        xqoa_head_size = SDL_SwapBE32(*(Uint32*)read_buf);

        if((Sint64)xqoa_head_size >= music->xqoa_data_size)
        {
            Mix_SetError("XQOA: Header size is larger than the file!");
            QOA_CleanUp(src, music);
            return NULL;
        }

        if (SDL_RWread(src, read_buf, 1, 4) != 4) {
            Mix_SetError("XQOA: Can't read data size field.");
            QOA_CleanUp(src, music);
            return NULL;
        }
        xqoa_data_size = SDL_SwapBE32(*(Uint32*)read_buf);

        if (xqoa_data_size + xqoa_head_size != music->xqoa_data_size) {
            Mix_SetError("XQOA: Sum of header size (%" PRIu32 ") and the data size (%" PRIu32 ") doesn't match the file size (%" PRIu32 ").",
                         xqoa_head_size, xqoa_data_size, music->xqoa_data_size);
            QOA_CleanUp(src, music);
            return NULL;
        }

        music->xqoa_data_offset = xqoa_head_size;
        music->xqoa_data_size = xqoa_data_size;

        if (SDL_RWread(src, read_buf, 1, 4) != 4) {
            Mix_SetError("XQOA: Can't read loop start field.");
            QOA_CleanUp(src, music);
            return NULL;
        }
        music->loop_start = SDL_SwapBE32(*(Uint32*)read_buf);

        if (SDL_RWread(src, read_buf, 1, 4) != 4) {
            Mix_SetError("XQOA: Can't read loop end field.");
            QOA_CleanUp(src, music);
            return NULL;
        }
        music->loop_end = SDL_SwapBE32(*(Uint32*)read_buf);

        music->loop_len = music->loop_end - music->loop_start;


        if (SDL_RWread(src, read_buf, 1, 4) != 4) {
            Mix_SetError("XQOA: Can't read multitrack channels field.");
            QOA_CleanUp(src, music);
            return NULL;
        }
        music->multitrack_channels = SDL_SwapBE32(*(Uint32*)read_buf);

        if (SDL_RWread(src, read_buf, 1, 4) != 4) {
            Mix_SetError("XQOA: Can't read multitrack tracks field.");
            QOA_CleanUp(src, music);
            return NULL;
        }
        music->multitrack_tracks = SDL_SwapBE32(*(Uint32*)read_buf);


        meta_tags_init(&music->tags);

        if(!_XQOA_ReadMetaTag(music, "TITLE", MIX_META_TITLE)) {
            QOA_CleanUp(src, music);
            return NULL;
        }

        if(!_XQOA_ReadMetaTag(music, "ARTIST", MIX_META_ARTIST)) {
            QOA_CleanUp(src, music);
            return NULL;
        }

        if(!_XQOA_ReadMetaTag(music, "ALBUM", MIX_META_ALBUM)) {
            QOA_CleanUp(src, music);
            return NULL;
        }

        if(!_XQOA_ReadMetaTag(music, "COPYRIGHT", MIX_META_COPYRIGHT)) {
            QOA_CleanUp(src, music);
            return NULL;
        }


        SDL_RWseek(src, music->xqoa_data_offset, RW_SEEK_SET);

        /* Read the normal QOA header */
        if (SDL_RWread(src, header, 1, QOA_MIN_FILESIZE) != QOA_MIN_FILESIZE) {
            Mix_SetError("QOA: Can't read header.");
            QOA_CleanUp(src, music);
            return NULL;
        }
    }

    music->first_frame_pos = qoa_decode_header(header, QOA_MIN_FILESIZE, &music->info);
    if (!music->first_frame_pos) {
        Mix_SetError("QOA: Can't parse header.");
        QOA_CleanUp(src, music);
        return NULL;
    }

    if (music->xqoa_data_offset > 0) {
        music->first_frame_pos += music->xqoa_data_offset;
    }

    if (music->info.channels == 0 || music->info.channels > 0xFF) {
        Mix_SetError("QOA: Invalid number of channels.");
        QOA_CleanUp(src, music);
        return NULL;
    }

    music->num_channels = music->info.channels;

    SDL_RWseek(src, music->first_frame_pos, RW_SEEK_SET);
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

    music->computed_src_rate = music->info.samplerate * music->speed;
    if (music->computed_src_rate < 1000) {
        music->computed_src_rate = 1000;
    }

    if (music->multitrack_channels > 0 && music->multitrack_tracks > 0) {
        music->multitrack = SDL_TRUE;
        if (music->multitrack_channels * music->multitrack_tracks > music->info.channels) {
            Mix_SetError("XQOA: Invalid multitrack setup: product of channels and tracks must not be bigger than actual channels number at this file.");
            QOA_CleanUp(src, music);
            return NULL;
        }
    }

    if (music->multitrack) {
        in_channels = (Uint8)music->multitrack_channels;
    } else {
        in_channels = (Uint8)music->info.channels;
    }

    music->stream = SDL_NewAudioStream(AUDIO_S16SYS, in_channels, music->computed_src_rate,
                                       music_spec.format, music_spec.channels, music_spec.freq);
    if (!music->stream) {
        Mix_SetError("QOA: Can't initialize stream.");
        QOA_CleanUp(src, music);
        return NULL;
    }

    if ((music->loop_end > 0) && (music->loop_end <= music->info.samples) &&
        (music->loop_start < music->loop_end)) {
        music->loop = 1;
    }

    music->freesrc = freesrc;
    return music;
}

static void *QOA_CreateFromRW(struct SDL_RWops *src, int freesrc)
{
    return QOA_CreateFromRWex(src, freesrc, NULL);
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
    SDL_RWseek(music->src, music->first_frame_pos, RW_SEEK_SET);
    music->sample_pos = 0;
    music->sample_data_len = 0;
    music->sample_data_pos = 0;
    music->at_end = SDL_FALSE;
    return 0;
}

/* Clean-up the output buffer */
static void QOA_Stop(void *context)
{
    QOA_Music *music = (QOA_Music *)context;
    SDL_AudioStreamClear(music->stream);
}

static unsigned int _QOA_DecodeFrame(QOA_Music *music)
{
    unsigned int samples_decoded = 0;
    size_t got_bytes, encoded_frame_size;

    encoded_frame_size = qoa_max_frame_size(&music->info);

    got_bytes = SDL_RWread(music->src, music->decode_buffer, 1, encoded_frame_size);
    if (got_bytes == 0) {
        return 0;
    }

    qoa_decode_frame(music->decode_buffer, got_bytes, &music->info, music->sample_data, &samples_decoded);

    /* required a multiple of 4 */
    samples_decoded &= ~3;

    music->sample_data_pos = 0;
    music->sample_data_len = samples_decoded;

    return samples_decoded;
}

static int _QOA_ReadSamples(QOA_Music *music, Sint16 *out_samples, int num_samples)
{
    int src_index = music->sample_data_pos * music->info.channels;
    int frame_size = (sizeof(Sint16) * music->num_channels);
    int samples_written = 0;
    int i, to_copy, samples_left;

    for (i = 0; i < num_samples; ) {
        /* Do we have to decode more samples? */
        if (music->sample_data_len - music->sample_data_pos == 0) {
            if (music->at_end) {
                break;
            }
            if (!_QOA_DecodeFrame(music)) {
                music->sample_pos += 1;
                music->at_end = SDL_TRUE;
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

static int QOA_SeekToSample(QOA_Music *music, Uint32 dst_pos)
{
    int dst_frame;
    Sint64 offset;

    if (dst_pos > music->info.samples) {
        music->sample_pos = music->info.samples + 1;
        music->at_end = SDL_TRUE;
        return -1;
    }

    dst_frame = dst_pos / QOA_FRAME_LEN;

    music->sample_pos = dst_frame * QOA_FRAME_LEN;
    music->sample_data_len = 0;
    music->sample_data_pos = 0;

    offset = music->first_frame_pos + dst_frame * qoa_max_frame_size(&music->info);

    SDL_RWseek(music->src, offset, RW_SEEK_SET);

    music->skip_samples = dst_pos - (dst_frame * QOA_FRAME_LEN);
    music->at_end = SDL_FALSE;

    return 0;
}

/* Play some of a stream previously started with xmp_play() */
static int QOA_GetSome(void *context, void *data, int bytes, SDL_bool *done)
{
    QOA_Music *music = (QOA_Music *)context;
    SDL_bool looped = SDL_FALSE, retry_get = SDL_FALSE;
    int filled, amount, channels, result, amount_samples, div_chans, i;
    int frame_size = (sizeof(Sint16) * music->num_channels);
    Uint32 pcmPos, j, k;
    Sint16 buf_mid[8];
    Sint16 *buf_in, *buf_out;

try_get:
    filled = SDL_AudioStreamGet(music->stream, data, bytes);
    if (filled != 0) {
        return filled;
    }

    if (!music->play_count) {
        /* All done */
        *done = SDL_TRUE;
        return 0;
    }

    if (music->computed_src_rate < 0) {
        result = QOA_UpdateSpeed(music);
        if (result < 0) {
            return -1; /* Error has occured */
        }
        retry_get = SDL_TRUE;
    }

    amount = _QOA_ReadSamples(music, (Sint16*)music->buffer, music->buffer_size / frame_size);
    amount *= frame_size;

    channels = music->info.channels;
    pcmPos = music->sample_pos;

    if (music->multitrack && amount > 0) { /* Mix channels into desired output */
        amount_samples = amount / frame_size;
        amount = music->multitrack_channels * amount_samples * sizeof(Sint16);
        channels = music->multitrack_channels;
        div_chans = (music->info.channels / music->multitrack_channels);
        buf_in = (Sint16*)music->buffer;
        buf_out = (Sint16*)music->buffer;

        for (i = 0; i < amount_samples; ++i) {
            for (k = 0; k < music->multitrack_channels; ++k) {
                buf_mid[k] = 0;
            }

            for (j = 0; j < music->multitrack_tracks; ++j) {
                if (music->multitrack_mute[j]) {
                    continue;
                }

                for (k = 0; k < music->multitrack_channels; ++k) {
                    buf_mid[k] += buf_in[(j * music->multitrack_channels) + k] / div_chans;
                }
            }

            for (k = 0; k < music->multitrack_channels; ++k) {
                buf_out[k] = buf_mid[k];
            }

            buf_in += music->info.channels;
            buf_out += music->multitrack_channels;
        }

    }

    if (music->loop && (music->play_count != 1) && (pcmPos >= music->loop_end)) {
        amount -= (int)((pcmPos - music->loop_end) * channels) * (int)sizeof(Sint16);
        result = QOA_SeekToSample(music, music->loop_start);
        if (result < 0) {
            return Mix_SetError("XQOA: Failed to seek via qoa_seek_to_sample");
        } else {
            int play_count = -1;
            if (music->play_count > 0) {
                play_count = (music->play_count - 1);
            }
            music->play_count = play_count;
        }
        looped = SDL_TRUE;
    }

    if (amount > 0) {
        if (SDL_AudioStreamPut(music->stream, music->buffer, amount) < 0) {
            return -1;
        }
    } else if (!looped) {
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

    if (retry_get) {
        goto try_get; /* Prevent the false-positive "too many zero loop cycles" break condition match */
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
    Uint32 dst_pos;

    if (pos < 0.0) {
        pos = 0.0;
    }

    dst_pos = (int)(pos * music->info.samplerate);

    return QOA_SeekToSample(music, dst_pos);
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

static int QOA_SetSpeed(void *context, double speed)
{
    QOA_Music *music = (QOA_Music *)context;
    if (speed <= 0.01) {
        speed = 0.01;
    }
    music->speed = speed;
    music->computed_src_rate = -1;
    return 0;
}

static double QOA_GetSpeed(void *context)
{
    QOA_Music *music = (QOA_Music *)context;
    return music->speed;
}

static double QOA_LoopStart(void *music_p)
{
    QOA_Music *music = (QOA_Music *)music_p;
    if (music->loop > 0) {
        return (double)music->loop_start / music->info.samplerate;
    }
    return -1.0;
}

static double QOA_LoopEnd(void *music_p)
{
    QOA_Music *music = (QOA_Music *)music_p;
    if (music->loop > 0) {
        return (double)music->loop_end / music->info.samplerate;
    }
    return -1.0;
}

static double QOA_LoopLength(void *music_p)
{
    QOA_Music *music = (QOA_Music *)music_p;
    if (music->loop > 0) {
        return (double)music->loop_len / music->info.samplerate;
    }
    return -1.0;
}

static int QOA_GetTracksCount(void *music_p)
{
    QOA_Music *music = (QOA_Music *)music_p;
    if (music->multitrack) {
        return music->multitrack_tracks;
    }
    return -1;
}

static int QOA_SetTrackMute(void *music_p, int track, int mute)
{
    QOA_Music *music = (QOA_Music *)music_p;
    if (music->multitrack && track >= 0 && (Uint32)track < music->multitrack_tracks) {
        music->multitrack_mute[track] = mute;
        return 0;
    }
    return -1;
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

    SDL_RWseek(src, 0, RW_SEEK_SET);
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
    QOA_CreateFromRWex,
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
    QOA_SetSpeed,
    QOA_GetSpeed,
    NULL,   /* SetPitch [MIXER-X] */
    NULL,   /* GetPitch [MIXER-X] */
    QOA_GetTracksCount,
    QOA_SetTrackMute,
    QOA_LoopStart,
    QOA_LoopEnd,
    QOA_LoopLength,
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
