/*
  SDL_mixer:  An audio mixer library based on the SDL library
  Copyright (C) 1997-2022 Sam Lantinga <slouken@libsdl.org>

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
#ifdef MUSIC_FFMPEG

#include "music_ffmpeg.h"

#include "SDL_log.h"
#include "SDL_assert.h"

#define inline SDL_INLINE
#define av_always_inline SDL_INLINE
#define __STDC_CONSTANT_MACROS

#include <libavutil/frame.h>
#include <libavutil/mem.h>

#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/avutil.h>
#include <libavutil/error.h>
#include <libavutil/samplefmt.h>
#include <libavutil/opt.h>
#include <libswresample/swresample.h>


#define AUDIO_INBUF_SIZE 4096

#if LIBAVCODEC_VERSION_INT >= AV_VERSION_INT(59, 24, 100)
#define AVCODEC_NEW_CHANNEL_LAYOUT
#endif

typedef struct ffmpeg_loader {
    int loaded;
    void *handle_avcodec;
    void *handle_avformat;
    void *handle_avutil;
    void *handle_swresample;


    unsigned (*avformat_version)(void);
    AVFormatContext *(*avformat_alloc_context)(void);
    AVIOContext *(*avio_alloc_context)(
                      unsigned char *buffer,
                      int buffer_size,
                      int write_flag,
                      void *opaque,
                      int (*read_packet)(void *opaque, uint8_t *buf, int buf_size),
                      int (*write_packet)(void *opaque, uint8_t *buf, int buf_size),
                      int64_t (*seek)(void *opaque, int64_t offset, int whence));
    int (*avformat_open_input)(AVFormatContext **ps, const char *url,
                               const AVInputFormat *fmt, AVDictionary **options);
    int (*avformat_find_stream_info)(AVFormatContext *ic, AVDictionary **options);
    int (*av_find_best_stream)(AVFormatContext *ic,
                              enum AVMediaType type,
                              int wanted_stream_nb,
                              int related_stream,
                              const AVCodec **decoder_ret,
                              int flags);
    void (*av_dump_format)(AVFormatContext *ic,
                           int index,
                           const char *url,
                           int is_output);
    int (*av_seek_frame)(AVFormatContext *s, int stream_index, int64_t timestamp, int flags);
    int (*av_read_frame)(AVFormatContext *s, AVPacket *pkt);
    int (*avformat_seek_file)(AVFormatContext *s, int stream_index, int64_t min_ts, int64_t ts, int64_t max_ts, int flags);
    void (*avformat_close_input)(AVFormatContext **s);


    unsigned (*avcodec_version)(void);
    const AVCodec *(*avcodec_find_decoder)(enum AVCodecID id);
    AVCodecContext *(*avcodec_alloc_context3)(const AVCodec *codec);
    int (*avcodec_parameters_to_context)(AVCodecContext *codec, const AVCodecParameters *par);
    int (*avcodec_open2)(AVCodecContext *avctx, const AVCodec *codec, AVDictionary **options);
    AVPacket *(*av_packet_alloc)(void);
    int (*avcodec_send_packet)(AVCodecContext *avctx, const AVPacket *avpkt);
    int (*avcodec_receive_frame)(AVCodecContext *avctx, AVFrame *frame);
    void (*av_packet_unref)(AVPacket *pkt);
    void (*av_packet_free)(AVPacket **pkt);
    void (*avcodec_flush_buffers)(AVCodecContext *avctx);
    void (*avcodec_free_context)(AVCodecContext **avctx);


    unsigned (*avutil_version)(void);
    int (*av_opt_set_int)(void *obj, const char *name, int64_t val, int search_flags);
    int (*av_opt_set_sample_fmt)(void *obj, const char *name, enum AVSampleFormat fmt, int search_flags);
    void *(*av_malloc)(size_t size);
    int (*av_strerror)(int errnum, char *errbuf, size_t errbuf_size);
    AVFrame *(*av_frame_alloc)(void);
    int64_t (*av_rescale)(int64_t a, int64_t b, int64_t c);
    void (*av_frame_free)(AVFrame **frame);


    unsigned (*swresample_version)(void);
    struct SwrContext *(*swr_alloc)(void);
    int (*swr_init)(struct SwrContext *s);
    int (*av_get_bytes_per_sample)(enum AVSampleFormat sample_fmt);
    int (*swr_convert)(struct SwrContext *s, uint8_t **out, int out_count,
                                       const uint8_t **in , int in_count);
    void (*swr_free)(struct SwrContext **s);

} ffmpeg_loader;

static ffmpeg_loader ffmpeg;

static inline char *mix_av_make_error_string(char *errbuf, size_t errbuf_size, int errnum)
{
    ffmpeg.av_strerror(errnum, errbuf, errbuf_size);
    return errbuf;
}

#define mix_av_err2str(errnum) \
    mix_av_make_error_string((char[AV_ERROR_MAX_STRING_SIZE]){0}, AV_ERROR_MAX_STRING_SIZE, errnum)


static int FFMPEG_Load(void)
{
    unsigned ver_avcodec, ver_avformat, ver_avutil, ver_swresample;

    if (ffmpeg.loaded == 0) {

        /* AVFormat */
        ffmpeg.avformat_version = avformat_version;
        ffmpeg.avformat_alloc_context = avformat_alloc_context;
        ffmpeg.avio_alloc_context = avio_alloc_context;
        ffmpeg.avformat_open_input = avformat_open_input;
        ffmpeg.avformat_find_stream_info = avformat_find_stream_info;
        ffmpeg.av_find_best_stream = av_find_best_stream;
        ffmpeg.av_dump_format = av_dump_format;
        ffmpeg.av_seek_frame = av_seek_frame;
        ffmpeg.avformat_seek_file = avformat_seek_file;
        ffmpeg.av_read_frame = av_read_frame;
        ffmpeg.avformat_close_input = avformat_close_input;

        /* AVCodec */
        ffmpeg.avcodec_version = avcodec_version;
        ffmpeg.avcodec_find_decoder = avcodec_find_decoder;
        ffmpeg.avcodec_alloc_context3 = avcodec_alloc_context3;
        ffmpeg.avcodec_parameters_to_context = avcodec_parameters_to_context;
        ffmpeg.avcodec_open2 = avcodec_open2;
        ffmpeg.av_packet_alloc = av_packet_alloc;
        ffmpeg.avcodec_send_packet = avcodec_send_packet;
        ffmpeg.avcodec_receive_frame = avcodec_receive_frame;
        ffmpeg.av_packet_unref = av_packet_unref;
        ffmpeg.av_packet_free = av_packet_free;
        ffmpeg.avcodec_flush_buffers = avcodec_flush_buffers;
        ffmpeg.avcodec_free_context = avcodec_free_context;

        /* AVUtil */
        ffmpeg.avutil_version = avutil_version;
        ffmpeg.av_opt_set_int = av_opt_set_int;
        ffmpeg.av_opt_set_sample_fmt = av_opt_set_sample_fmt;
        ffmpeg.av_malloc = av_malloc;
        ffmpeg.av_strerror = av_strerror;
        ffmpeg.av_frame_alloc = av_frame_alloc;
        ffmpeg.av_rescale = av_rescale;
        ffmpeg.av_frame_free = av_frame_free;

        /* SWResample */
        ffmpeg.swresample_version = swresample_version;
        ffmpeg.swr_alloc = swr_alloc;
        ffmpeg.swr_init = swr_init;
        ffmpeg.av_get_bytes_per_sample = av_get_bytes_per_sample;
        ffmpeg.swr_convert = swr_convert;
        ffmpeg.swr_free = swr_free;

        ver_avcodec = ffmpeg.avcodec_version();
        ver_avformat = ffmpeg.avformat_version();
        ver_avutil = ffmpeg.avutil_version();
        ver_swresample = ffmpeg.swresample_version();

        if (AV_VERSION_MAJOR(ver_avcodec) != LIBAVCODEC_VERSION_MAJOR) {
            Mix_SetError("Linked FFMPEG %s version %u is INCOMPATIBLE, the major version %u is required",
                         "avcodec", AV_VERSION_MAJOR(ver_avcodec), LIBAVCODEC_VERSION_MAJOR);
            return -1;
        }

        if (AV_VERSION_MAJOR(ver_avformat) != LIBAVFORMAT_VERSION_MAJOR) {
            Mix_SetError("Linked FFMPEG %s version %u is INCOMPATIBLE, the major version %u is required",
                         "avformat", AV_VERSION_MAJOR(ver_avformat), LIBAVFORMAT_VERSION_MAJOR);
            return -1;
        }

        if (AV_VERSION_MAJOR(ver_avutil) != LIBAVUTIL_VERSION_MAJOR) {
            Mix_SetError("Linked FFMPEG %s version %u is INCOMPATIBLE, the major version %u is required",
                         "avutil", AV_VERSION_MAJOR(ver_avutil), LIBAVUTIL_VERSION_MAJOR);
            return -1;
        }

        if (AV_VERSION_MAJOR(ver_swresample) != LIBSWRESAMPLE_VERSION_MAJOR) {
            Mix_SetError("Linked FFMPEG %s version %u is INCOMPATIBLE, the major version %u is required",
                         "swresample", AV_VERSION_MAJOR(ver_swresample), LIBSWRESAMPLE_VERSION_MAJOR);
            return -1;
        }

        if (ver_avcodec == LIBAVCODEC_VERSION_INT) {
            SDL_Log("Linked FFMPEG avcodec version %u", ver_avcodec);
        } else {
            SDL_Log("Linked FFMPEG avcodec version %u is NOT MATCHING to API version %u", ver_avcodec, LIBAVCODEC_VERSION_INT);
        }

        if (ver_avformat == LIBAVFORMAT_VERSION_INT) {
            SDL_Log("Linked FFMPEG avformat version %u", ver_avformat);
        } else {
            SDL_Log("Linked FFMPEG avformat version %u is NOT MATCHING to API version %u", ver_avformat, LIBAVFORMAT_VERSION_INT);
        }

        if (ver_avutil == LIBAVUTIL_VERSION_INT) {
            SDL_Log("Linked FFMPEG avutil version %u", ver_avutil);
        } else {
            SDL_Log("Linked FFMPEG avutil version %u is NOT MATCHING to API version %u", ver_avutil, LIBAVUTIL_VERSION_INT);
        }

        if (ver_swresample == LIBSWRESAMPLE_VERSION_INT) {
            SDL_Log("Linked FFMPEG swresample version %u", ver_swresample);
        } else {
            SDL_Log("Linked FFMPEG swresample version %u is NOT MATCHING to API version %u", ver_swresample, LIBSWRESAMPLE_VERSION_INT);
        }
    }
    ++ffmpeg.loaded;

    return 0;
}

static void FFMPEG_Unload(void)
{
    if (ffmpeg.loaded == 0) {
        return;
    }
    if (ffmpeg.loaded == 1) {
#ifdef FFMPEG_DYNAMIC
        SDL_UnloadObject(ffmpeg.handle_avcodec);
        SDL_UnloadObject(ffmpeg.handle_avformat);
        SDL_UnloadObject(ffmpeg.handle_avutil);
        SDL_UnloadObject(ffmpeg.handle_swresample);
#endif
    }
    --ffmpeg.loaded;
}



/* This file supports Game Music Emulator music streams */
typedef struct
{
    SDL_RWops *src;
    Sint64 src_start;
    int freesrc;
    AVFormatContext *fmt_ctx;
    AVIOContext     *avio_in;
    const AVCodec *codec;
    AVCodecContext *audio_dec_ctx;
    AVStream *audio_stream;
    AVCodecParserContext *parser;
    AVPacket *pkt;
    AVFrame *decoded_frame;
    enum AVSampleFormat sfmt;
    SDL_bool planar;
    enum AVSampleFormat dst_sample_fmt;
    SwrContext *swr_ctx;
    Uint8 *merge_buffer;
    size_t merge_buffer_size;
    int srate;
    int schannels;
    int stream_index;
    int play_count;
    int volume;
    double time_position;
    double time_duration;
    SDL_AudioStream *stream;
    Uint8 *in_buffer;
    size_t in_buffer_size;
    void *buffer;
    size_t buffer_size;
} FFMPEG_Music;


static int FFMPEG_UpdateStream(FFMPEG_Music *music)
{
    SDL_assert(music->audio_stream->codecpar);
    enum AVSampleFormat sfmt = music->audio_stream->codecpar->format;
    int srate = music->audio_stream->codecpar->sample_rate;
#if defined(AVCODEC_NEW_CHANNEL_LAYOUT)
    int channels = music->audio_stream->codecpar->ch_layout.nb_channels;
#else
    int channels = music->audio_stream->codecpar->channels;
#endif
    int fmt = 0;
    int layout;

    if (srate == 0 || channels == 0) {
        return -1;
    }

    if (sfmt != music->sfmt || srate != music->srate || channels != music->schannels || !music->stream) {
        music->planar = SDL_FALSE;

        switch(sfmt)
        {
        case AV_SAMPLE_FMT_U8P:
            music->planar = SDL_TRUE;
            music->dst_sample_fmt = AV_SAMPLE_FMT_U8;
            /*fallthrough*/
        case AV_SAMPLE_FMT_U8:
            fmt = AUDIO_U8;
            break;

        case AV_SAMPLE_FMT_S16P:
            music->planar = SDL_TRUE;
            music->dst_sample_fmt = AV_SAMPLE_FMT_S16;
            /*fallthrough*/
        case AV_SAMPLE_FMT_S16:
            fmt = AUDIO_S16SYS;
            break;

        case AV_SAMPLE_FMT_S32P:
            music->planar = SDL_TRUE;
            music->dst_sample_fmt = AV_SAMPLE_FMT_S32;
            /*fallthrough*/
        case AV_SAMPLE_FMT_S32:
            fmt = AUDIO_S32SYS;
            break;

        case AV_SAMPLE_FMT_FLTP:
            music->planar = SDL_TRUE;
            music->dst_sample_fmt = AV_SAMPLE_FMT_FLT;
            /*fallthrough*/
        case AV_SAMPLE_FMT_FLT:
            fmt = AUDIO_F32SYS;
            break;

        default:
            return -1; /* Unsupported audio format */
        }

        if (music->stream) {
            SDL_FreeAudioStream(music->stream);
            music->stream = NULL;
        }

        if (music->merge_buffer) {
            SDL_free(music->merge_buffer);
            music->merge_buffer = NULL;
            music->merge_buffer_size = 0;
        }
        if (music->swr_ctx) {
            swr_free(&music->swr_ctx);
            music->swr_ctx = NULL;
        }

        music->stream = SDL_NewAudioStream(fmt, (Uint8)channels, srate,
                                           music_spec.format, music_spec.channels, music_spec.freq);
        if (!music->stream) {
            return -2;
        }

        if (music->planar) {
            music->swr_ctx = ffmpeg.swr_alloc();
#if defined(AVCODEC_NEW_CHANNEL_LAYOUT)
            layout = music->audio_stream->codecpar->ch_layout.u.mask;
#else
            layout = music->audio_stream->codecpar->channel_layout;
#endif
            if (layout == 0) {
                if(channels > 2) {
                    layout = AV_CH_LAYOUT_SURROUND;
                } else if(channels == 2) {
                    layout = AV_CH_LAYOUT_STEREO;
                } else if(channels == 1) {
                    layout = AV_CH_LAYOUT_MONO;
                }
            }
            ffmpeg.av_opt_set_int(music->swr_ctx, "in_channel_layout",  layout, 0);
            ffmpeg.av_opt_set_int(music->swr_ctx, "out_channel_layout", layout, 0);
            ffmpeg.av_opt_set_int(music->swr_ctx, "in_sample_rate",     srate, 0);
            ffmpeg.av_opt_set_int(music->swr_ctx, "out_sample_rate",    srate, 0);
            ffmpeg.av_opt_set_sample_fmt(music->swr_ctx, "in_sample_fmt",  sfmt, 0);
            ffmpeg.av_opt_set_sample_fmt(music->swr_ctx, "out_sample_fmt", music->dst_sample_fmt,  0);
            ffmpeg.swr_init(music->swr_ctx);

            music->merge_buffer_size = channels * ffmpeg.av_get_bytes_per_sample(sfmt) * 4096;
            music->merge_buffer = (Uint8*)SDL_calloc(1, music->merge_buffer_size);
            if (!music->merge_buffer) {
                music->planar = SDL_FALSE;
                return -1;
            }
        }

        music->sfmt = sfmt;
        music->srate = srate;
        music->schannels = channels;
    }

    return 0;
}

static int _rw_read_buffer(void *opaque, uint8_t *buf, int buf_size)
{
    FFMPEG_Music *music = (FFMPEG_Music *)opaque;
    size_t ret = SDL_RWread(music->src, buf, 1, buf_size);

    if (ret == 0) {
        return AVERROR_EOF;
    }

    return ret;
}

static int64_t _rw_seek(void *opaque, int64_t offset, int whence)
{
    FFMPEG_Music *music = (FFMPEG_Music *)opaque;
    int rw_whence;

    switch(whence)
    {
    default:
    case SEEK_SET:
        rw_whence = RW_SEEK_SET;
        offset += music->src_start;
        break;
    case SEEK_CUR:
        rw_whence = RW_SEEK_CUR;
        break;
    case SEEK_END:
        rw_whence = RW_SEEK_END;
        break;
    case AVSEEK_SIZE:
        return SDL_RWsize(music->src);
    }

    return SDL_RWseek(music->src, offset, rw_whence);
}

static void FFMPEG_Delete(void *context);

static void *FFMPEG_NewRW(struct SDL_RWops *src, int freesrc)
{
    FFMPEG_Music *music = NULL;
    int ret;

    music = (FFMPEG_Music *)SDL_calloc(1, sizeof *music);
    if (!music) {
        SDL_OutOfMemory();
        return NULL;
    }

    music->in_buffer = (uint8_t *)ffmpeg.av_malloc(AUDIO_INBUF_SIZE);
    music->in_buffer_size = AUDIO_INBUF_SIZE;
    if (!music->in_buffer) {
        SDL_OutOfMemory();
        FFMPEG_Delete(music);
        return NULL;
    }

    music->src = src;
    music->src_start = SDL_RWtell(src);
    music->volume = MIX_MAX_VOLUME;

    music->fmt_ctx = ffmpeg.avformat_alloc_context();
    if (!music->fmt_ctx) {
        FFMPEG_Delete(music);
        Mix_SetError("FFMPEG: Failed to allocate format context");
        return NULL;
    }

    music->avio_in = ffmpeg.avio_alloc_context(music->in_buffer,
                                               music->in_buffer_size,
                                               0,
                                               music,
                                               _rw_read_buffer,
                                               NULL,
                                               _rw_seek);
    if(!music->avio_in) {
        FFMPEG_Delete(music);
        Mix_SetError("FFMPEG: Unhandled file format");
        return NULL;
    }

    music->fmt_ctx->pb = music->avio_in;

    ret = ffmpeg.avformat_open_input(&music->fmt_ctx, NULL, NULL, NULL);
    if (ret < 0) {
        Mix_SetError("FFMPEG: Failed to open the input: %s", mix_av_err2str(ret));
        FFMPEG_Delete(music);
        return NULL;
    }

    ret = ffmpeg.avformat_find_stream_info(music->fmt_ctx, NULL);
    if (ret < 0) {
        Mix_SetError("FFMPEG: Could not find stream information: %s", mix_av_err2str(ret));
        FFMPEG_Delete(music);
        return NULL;
    }

    ret = ffmpeg.av_find_best_stream(music->fmt_ctx, AVMEDIA_TYPE_AUDIO, -1, -1, &music->codec, 0);
    if (ret < 0) {
        Mix_SetError("FFMPEG: Could not find audio stream in input file: %s", mix_av_err2str(ret));
        FFMPEG_Delete(music);
        return NULL;
    }

    music->stream_index = ret;
    music->audio_stream = music->fmt_ctx->streams[music->stream_index];

    if (!music->codec) {
        music->codec = ffmpeg.avcodec_find_decoder(music->audio_stream->codecpar->codec_id);
        if (!music->codec) {
            Mix_SetError("FFMPEG: Failed to find audio codec");
            FFMPEG_Delete(music);
            return NULL;
        }
    }

    music->audio_dec_ctx = ffmpeg.avcodec_alloc_context3(music->codec);
    if (!music->audio_dec_ctx) {
        Mix_SetError("FFMPEG: Failed allocate the codec context");
        FFMPEG_Delete(music);
        return NULL;
    }

    if (!music->audio_stream->codecpar) {
        Mix_SetError("FFMPEG: codec parameters aren't recognised");
        FFMPEG_Delete(music);
        return NULL;
    }

    if (ffmpeg.avcodec_parameters_to_context(music->audio_dec_ctx, music->audio_stream->codecpar) < 0) {
        Mix_SetError("FFMPEG: Failed to copy codec parameters to context");
        FFMPEG_Delete(music);
        return NULL;
    }

    ret = ffmpeg.avcodec_open2(music->audio_dec_ctx, music->codec, NULL);
    if (ret < 0) {
        Mix_SetError("FFMPEG: Failed to initialise the decoder: %s", mix_av_err2str(ret));
        FFMPEG_Delete(music);
        return NULL;
    }

    music->decoded_frame = ffmpeg.av_frame_alloc();
    if (!music->decoded_frame) {
        Mix_SetError("FFMPEG: Failed to allocate the frame");
        FFMPEG_Delete(music);
        return NULL;
    }

    music->pkt = ffmpeg.av_packet_alloc();
    if (!music->pkt) {
        Mix_SetError("FFMPEG: Failed to allocate the packet");
        FFMPEG_Delete(music);
        return NULL;
    }

    if (FFMPEG_UpdateStream(music) < 0) {
        Mix_SetError("FFMPEG: Failed to initialise the stream");
        FFMPEG_Delete(music);
        return NULL;
    }

    if (music->fmt_ctx->duration != AV_NOPTS_VALUE && music->fmt_ctx->duration != 0) {
        music->time_duration = (double)music->fmt_ctx->duration / AV_TIME_BASE;
    } else if (music->audio_stream->nb_frames > 0) {
        music->time_duration = (double)music->audio_stream->nb_frames / music->audio_stream->codecpar->sample_rate;
    }

    ffmpeg.av_dump_format(music->fmt_ctx, music->stream_index, "<SDL_RWops context 1>", 0);

    music->freesrc = freesrc;

    return music;
}

/* Set the volume for a GME stream */
static void FFMPEG_SetVolume(void *music_p, int volume)
{
    FFMPEG_Music *music = (FFMPEG_Music*)music_p;
    music->volume = volume;
}

/* Get the volume for a GME stream */
static int FFMPEG_GetVolume(void *music_p)
{
    FFMPEG_Music *music = (FFMPEG_Music*)music_p;
    return music->volume;
}

/* Start playback of a given Game Music Emulators stream */
static int FFMPEG_Play(void *music_p, int play_count)
{
    FFMPEG_Music *music = (FFMPEG_Music*)music_p;
    if (music) {
        SDL_AudioStreamClear(music->stream);
        ffmpeg.av_seek_frame(music->fmt_ctx, music->stream_index, 0, AVSEEK_FLAG_ANY);
        music-> time_position = 0.0;
        music->play_count = play_count;
    }
    return 0;
}

static void bump_merge_buffer(FFMPEG_Music *music, size_t new_size)
{
    music->merge_buffer = (Uint8*)SDL_realloc(music->merge_buffer, new_size);
    music->merge_buffer_size = new_size;
}

static int decode_packet(FFMPEG_Music *music, const AVPacket *pkt, SDL_bool *got_some)
{
    int ret = 0;
    size_t unpadded_linesize;
    size_t sample_size;

    *got_some = SDL_FALSE;

    ret = ffmpeg.avcodec_send_packet(music->audio_dec_ctx, pkt);
    if (ret < 0) {
        if (ret == AVERROR_EOF) {
            return ret;
        }

        Mix_SetError("ERROR: Error submitting a packet for decoding (%s)", mix_av_err2str(ret));
        SDL_Log("FFMPEG: %s", Mix_GetError());
        return ret;
    }

    /* get all the available frames from the decoder */
    while (ret >= 0) {
        ret = ffmpeg.avcodec_receive_frame(music->audio_dec_ctx, music->decoded_frame);
        if (ret < 0) {
            /* those two return values are special and mean there is no output */
            /* frame available, but there were no errors during decoding */
            if (ret == AVERROR_EOF || ret == AVERROR(EAGAIN)) {
                return 0;
            }

            Mix_SetError("FFMPEG: Error during decoding (%s)", mix_av_err2str(ret));
            return ret;
        }

        FFMPEG_UpdateStream(music);

        if (music->planar) {
            sample_size = ffmpeg.av_get_bytes_per_sample(music->decoded_frame->format);
            unpadded_linesize = sample_size * music->decoded_frame->nb_samples * music->schannels;

            if (unpadded_linesize > music->merge_buffer_size) {
                bump_merge_buffer(music, unpadded_linesize);
            }

            ffmpeg.swr_convert(music->swr_ctx,
                               &music->merge_buffer, music->decoded_frame->nb_samples,
                               (const Uint8**)music->decoded_frame->extended_data, music->decoded_frame->nb_samples);

            music-> time_position += (double)music->decoded_frame->nb_samples / music->srate;

            if (SDL_AudioStreamPut(music->stream, music->merge_buffer, unpadded_linesize) < 0) {
                Mix_SetError("FFMPEG: Failed to put audio stream");
                return -1;
            }
        } else {
            unpadded_linesize = music->decoded_frame->nb_samples * ffmpeg.av_get_bytes_per_sample(music->decoded_frame->format);
            if (SDL_AudioStreamPut(music->stream, music->decoded_frame->extended_data[0], unpadded_linesize) < 0) {
                Mix_SetError("FFMPEG: Failed to put audio stream");
                return -1;
            }
        }

        av_frame_unref(music->decoded_frame);

        *got_some = SDL_TRUE;

        if (ret < 0)
            return ret;
    }

    return 0;
}

static int FFMPEG_GetSome(void *context, void *data, int bytes, SDL_bool *done)
{
    FFMPEG_Music *music = (FFMPEG_Music *)context;
    int filled;
    int ret = 0;
    SDL_bool got_some;

    filled = SDL_AudioStreamGet(music->stream, data, bytes);
    if (filled != 0) {
        return filled;
    }

    if (!music->play_count) {
        /* All done */
        *done = SDL_TRUE;
        return 0;
    }

    got_some = SDL_FALSE;

    /* read frames from the file */
    while (ffmpeg.av_read_frame(music->fmt_ctx, music->pkt) >= 0) {
        /* check if the packet belongs to a stream we are interested in, otherwise */
        /* skip it */
        if (music->pkt->stream_index == music->stream_index) {
            ret = decode_packet(music, music->pkt, &got_some);
        }
        ffmpeg.av_packet_unref(music->pkt);

        if (ret < 0 || got_some)
            break;
    }

    if (!got_some || ret == AVERROR_EOF) {
        if (music->play_count == 1) {
            music->play_count = 0;
            SDL_AudioStreamFlush(music->stream);
        } else {
            int play_count = -1;
            if (music->play_count > 0) {
                play_count = (music->play_count - 1);
            }
            if (FFMPEG_Play(music, play_count) < 0) {
                return -1;
            }
        }
        return 0;
    }

    return 0;
}


/* Play some of a stream previously started with GME_play() */
static int FFMPEG_PlayAudio(void *music_p, void *data, int bytes)
{
    FFMPEG_Music *music = (FFMPEG_Music*)music_p;
    return music_pcm_getaudio(music_p, data, bytes, music->volume, FFMPEG_GetSome);
}

static int FFMPEG_Seek(void *music_p, double time)
{
    FFMPEG_Music *music = (FFMPEG_Music*)music_p;
    AVRational timebase = music->audio_stream->time_base;
    int64_t ts;
    int err;

    ts = ffmpeg.av_rescale(time * 1000, timebase.den, timebase.num);
    ts /=1000;
    err = ffmpeg.avformat_seek_file(music->fmt_ctx, music->stream_index, 0, ts, ts, AVSEEK_FLAG_ANY);

    if (err >= 0) {
        music->time_position = time;
        ffmpeg.avcodec_flush_buffers(music->audio_dec_ctx);
    } else {
        SDL_Log("FFMPEG: Seek failed: %s", mix_av_err2str(err));
        return -1;
    }

    return 0;
}

static double FFMPEG_Tell(void *context)
{
    FFMPEG_Music *music = (FFMPEG_Music *)context;
    return (double)music->time_position;
}

static double FFMPEG_Duration(void *context)
{
    FFMPEG_Music *music = (FFMPEG_Music *)context;
    return (double)music->time_duration;
}


/* Close the given Game Music Emulators stream */
static void FFMPEG_Delete(void *context)
{
    FFMPEG_Music *music = (FFMPEG_Music*)context;
    if (music) {
        if (music->audio_dec_ctx) {
            ffmpeg.avcodec_free_context(&music->audio_dec_ctx);
        }
        if (music->fmt_ctx) {
            ffmpeg.avformat_close_input(&music->fmt_ctx);
        }
/*
        if (music->avio_in) {
            avio_closep(&music->avio_in);
        }
*/
        if (music->pkt) {
            ffmpeg.av_packet_free(&music->pkt);
        }
        if (music->decoded_frame) {
            ffmpeg.av_frame_free(&music->decoded_frame);
        }
        if (music->swr_ctx) {
            ffmpeg.swr_free(&music->swr_ctx);
        }

        music->in_buffer = NULL; /* This buffer is already freed by FFMPEG side*/
        music->in_buffer_size = 0;

        if (music->merge_buffer) {
            SDL_free(music->merge_buffer);
        }

        if (music->stream) {
            SDL_FreeAudioStream(music->stream);
        }
        if (music->buffer) {
            SDL_free(music->buffer);
        }
        if (music->freesrc) {
            SDL_RWclose(music->src);
        }
        SDL_free(music);
    }
}


Mix_MusicInterface Mix_MusicInterface_FFMPEG =
{
    "FFMPEG",
    MIX_MUSIC_FFMPEG,
    MUS_FFMPEG,
    SDL_FALSE,
    SDL_FALSE,

    FFMPEG_Load,   /* Load */
    NULL,   /* Open */
    FFMPEG_NewRW,
    NULL,   /* CreateFromRWex [MIXER-X]*/
    NULL,   /* CreateFromFile */
    NULL,   /* CreateFromFileEx [MIXER-X]*/
    FFMPEG_SetVolume,
    FFMPEG_GetVolume,   /* GetVolume [MIXER-X]*/
    FFMPEG_Play,
    NULL,   /* IsPlaying */
    FFMPEG_PlayAudio,
    NULL,   /* Jump */
    FFMPEG_Seek, /* Seek */
    FFMPEG_Tell, /* Tell [MIXER-X]*/
    FFMPEG_Duration, /* Duration */
    NULL,   /* SetTempo [MIXER-X] */
    NULL,   /* GetTempo [MIXER-X] */
    NULL,   /* SetSpeed [MIXER-X] */
    NULL,   /* GetSpeed [MIXER-X] */
    NULL,   /* SetPitch [MIXER-X] */
    NULL,   /* GetPitch [MIXER-X] */
    NULL,   /* GetTracksCount */
    NULL,   /* SetTrackMuted */
    NULL,   /* LoopStart [MIXER-X]*/
    NULL,   /* LoopEnd [MIXER-X]*/
    NULL,   /* LoopLength [MIXER-X]*/
    NULL,   /* GetMetaTag [MIXER-X]*/
    NULL,   /* Pause */
    NULL,   /* Resume */
    NULL,   /* Stop */
    FFMPEG_Delete,
    NULL,   /* Close */
    FFMPEG_Unload    /* Unload */
};


#endif /* MUSIC_FFMPEG */
