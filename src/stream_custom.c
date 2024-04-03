/*
  SDL_mixer:  An audio mixer library based on the SDL library
  Copyright (C) 1997-2024 Sam Lantinga <slouken@libsdl.org>

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

#define STREAM_CUSTOM_INTERNAL
#include "stream_custom.h"
#include "SDL_audio.h"
#include "SDL_assert.h"


typedef struct _Mix_AudioStream
{
    SDL_bool resampler_needed;
    Uint8* local_buffer;
    size_t local_buffer_len;
    size_t local_buffer_stored;

    Uint8 src_channels;
    SDL_AudioFormat src_format;

    int src_rate;
    int dst_rate;

    double ratio;
    double resample_pos;
    double resample_offset;

    void (*filter)(Mix_AudioStream *res, const Uint8 *in, int len, size_t sample_size);
    size_t sample_size;

    SDL_AudioStream *stream;
} Mix_AudioStream;


static int s_reallocBuffer(Mix_AudioStream *stream, int len)
{
    if (stream->ratio > 0.0) {
        stream->local_buffer_len = (size_t)SDL_ceil(len * stream->ratio) + 32;
    } else {
        stream->local_buffer_len = (size_t)len + 32;
    }

    if (stream->local_buffer) {
        stream->local_buffer = SDL_realloc(stream->local_buffer, stream->local_buffer_len);
    } else {
        stream->local_buffer = SDL_malloc(stream->local_buffer_len);
    }

    if (!stream->local_buffer) {
        SDL_OutOfMemory();
        return 0;
    }

    return 1;
}


static void s_upSampleAny(Mix_AudioStream *stream, const Uint8 *in, int len, size_t sample_size)
{
    const Uint8 *src = in;
    const Uint8 *src_end = in + len;
    Uint8 *dst = stream->local_buffer;
    Uint8 *dst_end = stream->local_buffer + stream->local_buffer_len;
    size_t offset = stream->src_channels * sample_size;

    stream->local_buffer_stored = 0;

    while (src < src_end && dst < dst_end) {
        SDL_memcpy(dst, src, offset);
        dst += offset;
        stream->local_buffer_stored += offset;

        stream->resample_pos += stream->resample_offset;

        if (stream->resample_pos >= 1.0) {
            src += offset;
            stream->resample_pos -= 1.0;
        }
    }
}


static void s_downSampleAny(Mix_AudioStream *stream, const Uint8 *in, int len, size_t sample_size)
{
    const Uint8 *src = in;
    const Uint8 *src_end = in + len;
    Uint8 *dst = stream->local_buffer;
    Uint8 *dst_end = stream->local_buffer + stream->local_buffer_len;
    size_t offset = stream->src_channels * sample_size;

    stream->local_buffer_stored = 0;

    do {
        stream->resample_pos += stream->resample_offset;

        if (stream->resample_pos >= 1.0) {
            SDL_memcpy(dst, src, offset);
            dst += offset;
            stream->resample_pos -= 1.0;
            stream->local_buffer_stored += offset;
        }

        src += offset;
    } while (src < src_end && dst < dst_end);
}


#define F_RESAMPLE_BY_BYTE(numByte, type) \
static void s_upSample##numByte##byte(Mix_AudioStream *stream, const Uint8 *in, int len, size_t sample_size)\
{\
    const Uint8 *src = in;\
    const Uint8 *src_end = in + len;\
    Uint8 *dst = stream->local_buffer;\
    Uint8 *dst_end = stream->local_buffer + stream->local_buffer_len;\
    size_t offset = stream->src_channels * sample_size;\
\
    stream->local_buffer_stored = 0;\
\
    while (src < src_end && dst < dst_end) {\
        *(type*)dst = *(type*)src;\
        dst += offset;\
        stream->local_buffer_stored += offset;\
\
        stream->resample_pos += stream->resample_offset;\
\
        if (stream->resample_pos >= 1.0) {\
            src += offset;\
            stream->resample_pos -= 1.0;\
        }\
    }\
}\
\
static void s_downSample##numByte##byte(Mix_AudioStream *stream, const Uint8 *in, int len, size_t sample_size)\
{\
    const Uint8 *src = in;\
    const Uint8 *src_end = in + len;\
    Uint8 *dst = stream->local_buffer;\
    Uint8 *dst_end = stream->local_buffer + stream->local_buffer_len;\
    size_t offset = stream->src_channels * sample_size;\
\
    stream->local_buffer_stored = 0;\
\
    do {\
        stream->resample_pos += stream->resample_offset;\
\
        if (stream->resample_pos >= 1.0) {\
            *(type*)dst = *(type*)src;\
            dst += offset;\
            stream->resample_pos -= 1.0;\
            stream->local_buffer_stored += offset;\
        }\
\
        src += offset;\
    } while (src < src_end && dst < dst_end);\
}

F_RESAMPLE_BY_BYTE(1, Uint8)
F_RESAMPLE_BY_BYTE(2, Uint16)
F_RESAMPLE_BY_BYTE(4, Uint32)
F_RESAMPLE_BY_BYTE(8, Uint64)


Mix_AudioStream *Mix_NewAudioStream(const SDL_AudioFormat src_format,
                                    const Uint8 src_channels,
                                    const int src_rate,
                                    const SDL_AudioFormat dst_format,
                                    const Uint8 dst_channels,
                                    const int dst_rate)
{
    Mix_AudioStream *stream = SDL_calloc(1, sizeof(Mix_AudioStream));

    if (src_rate != dst_rate) {
        stream->src_channels = src_channels;
        stream->src_format = src_format;
        stream->src_rate = src_rate;
        stream->dst_rate = dst_rate;
        stream->ratio = (double)dst_rate / (double)src_rate;
        stream->resample_pos = src_rate < dst_rate ? 0.0 : 1.0;
        stream->resample_offset   = src_rate < dst_rate ? 1.0 / stream->ratio : stream->ratio;
        stream->resampler_needed = SDL_TRUE;
        stream->sample_size = SDL_AUDIO_BITSIZE(stream->src_format) / 8;

        switch(stream->sample_size * stream->src_channels)
        {
        case 1:
            stream->filter = src_rate < dst_rate ? s_upSample1byte : s_downSample1byte;
            break;

        case 2:
            stream->filter = src_rate < dst_rate ? s_upSample2byte : s_downSample2byte;
            break;

        case 4:
            stream->filter = src_rate < dst_rate ? s_upSample4byte : s_downSample4byte;
            break;

        case 8:
            stream->filter = src_rate < dst_rate ? s_upSample8byte : s_downSample8byte;
            break;

        default:
            stream->filter = src_rate < dst_rate ? s_upSampleAny : s_downSampleAny;
            break;
        }
    }

    stream->stream = SDL_NewAudioStream(src_format, src_channels, dst_rate,
                                        dst_format, dst_channels, dst_rate);
    if (!stream->stream) {
        Mix_FreeAudioStream(stream);
        return NULL;
    }

    return stream;
}

int Mix_AudioStreamPut(Mix_AudioStream *stream, const void *buf, int len)
{
    Uint8 *out = (Uint8 *)buf;
    int out_len = len;

    if (stream->resampler_needed) {
        if (!stream->local_buffer || stream->local_buffer_len < (size_t)len) {
            if (!s_reallocBuffer(stream, len)) {
                return -1;
            }
        }

        stream->filter(stream, buf, len, stream->sample_size);
        out = stream->local_buffer;
        out_len = stream->local_buffer_stored;
    }

    return SDL_AudioStreamPut(stream->stream, out, out_len);
}

int Mix_AudioStreamGet(Mix_AudioStream *stream, void *buf, int len)
{
    return SDL_AudioStreamGet(stream->stream, buf, len);
}

int Mix_AudioStreamAvailable(Mix_AudioStream *stream)
{
    return SDL_AudioStreamAvailable(stream->stream);
}

int Mix_AudioStreamFlush(Mix_AudioStream *stream)
{
    return SDL_AudioStreamFlush(stream->stream);
}

void Mix_AudioStreamClear(Mix_AudioStream *stream)
{
    SDL_memset(stream->local_buffer, 0, stream->local_buffer_len);
    SDL_AudioStreamClear(stream->stream);
}

void Mix_FreeAudioStream(Mix_AudioStream *stream)
{
    if (stream->local_buffer) {
        SDL_free(stream->local_buffer);
    }

    if (stream->stream) {
        SDL_FreeAudioStream(stream->stream);
    }

    SDL_free(stream);
}
