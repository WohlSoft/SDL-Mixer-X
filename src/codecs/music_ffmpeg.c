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

/* This file supports Game Music Emulator music streams */
typedef struct
{
    SDL_RWops *src;
    Sint64 src_start;
    int freesrc;
    struct AVFormatContext *fmt_ctx;
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
    struct SwrContext *swr_ctx;
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


static int FFMPEG_Load(void)
{
    return 0;
}

static void FFMPEG_Unload(void)
{

}


static int FFMPEG_UpdateStream(FFMPEG_Music *music)
{
    SDL_assert(music->audio_stream->codecpar);
    enum AVSampleFormat sfmt = music->audio_stream->codecpar->format;
    int srate = music->audio_stream->codecpar->sample_rate;
    int channels = music->audio_stream->codecpar->channels;
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
            music->swr_ctx = swr_alloc();
            layout = music->audio_stream->codecpar->channel_layout;
            if (layout == 0) {
                if(channels > 2) {
                    layout = AV_CH_LAYOUT_SURROUND;
                } else if(channels == 2) {
                    layout = AV_CH_LAYOUT_STEREO;
                } else if(channels == 1) {
                    layout = AV_CH_LAYOUT_MONO;
                }
            }
            av_opt_set_int(music->swr_ctx, "in_channel_layout",  layout, 0);
            av_opt_set_int(music->swr_ctx, "out_channel_layout", layout, 0);
            av_opt_set_int(music->swr_ctx, "in_sample_rate",     srate, 0);
            av_opt_set_int(music->swr_ctx, "out_sample_rate",    srate, 0);
            av_opt_set_sample_fmt(music->swr_ctx, "in_sample_fmt",  sfmt, 0);
            av_opt_set_sample_fmt(music->swr_ctx, "out_sample_fmt", music->dst_sample_fmt,  0);
            swr_init(music->swr_ctx);

            music->merge_buffer_size = channels * av_get_bytes_per_sample(sfmt) * 4096;
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

    music->in_buffer = (uint8_t *)av_malloc(AUDIO_INBUF_SIZE);
    music->in_buffer_size = AUDIO_INBUF_SIZE;
    if (!music->in_buffer) {
        SDL_OutOfMemory();
        FFMPEG_Delete(music);
        return NULL;
    }

    music->src = src;
    music->src_start = SDL_RWtell(src);
    music->volume = MIX_MAX_VOLUME;

    music->fmt_ctx = avformat_alloc_context();
    if (!music->fmt_ctx) {
        FFMPEG_Delete(music);
        Mix_SetError("FFMPEG: Failed to allocate format context");
        return NULL;
    }

    music->avio_in = avio_alloc_context(music->in_buffer,
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

    ret = avformat_open_input(&music->fmt_ctx, "/home/vitaly/Yandex.Disk/Музыка (Phone)/ff6-fanatics.mid.m4a", NULL, NULL);
    if (ret < 0) {
        Mix_SetError("FFMPEG: Failed to open the input: %s", av_err2str(ret));
        FFMPEG_Delete(music);
        return NULL;
    }

    ret = avformat_find_stream_info(music->fmt_ctx, NULL);
    if (ret < 0) {
        Mix_SetError("FFMPEG: Could not find stream information: %s", av_err2str(ret));
        FFMPEG_Delete(music);
        return NULL;
    }

    ret = av_find_best_stream(music->fmt_ctx, AVMEDIA_TYPE_AUDIO, -1, -1, &music->codec, 0);
    if (ret < 0) {
        Mix_SetError("FFMPEG: Could not find audio stream in input file: %s", av_err2str(ret));
        FFMPEG_Delete(music);
        return NULL;
    }

    music->stream_index = ret;
    music->audio_stream = music->fmt_ctx->streams[music->stream_index];

    av_dump_format(music->fmt_ctx, music->stream_index, "<SDL_RWops context 1>", 0);

    if (music->audio_stream->duration != AV_NOPTS_VALUE) {
        music->time_duration = music->fmt_ctx->duration / AV_TIME_BASE;
    }

    if (!music->codec) {
        music->codec = avcodec_find_decoder(music->audio_stream->codecpar->codec_id);
        if (!music->codec) {
            Mix_SetError("FFMPEG: Failed to find audio codec");
            FFMPEG_Delete(music);
            return NULL;
        }
    }

    music->audio_dec_ctx = avcodec_alloc_context3(music->codec);
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

    if (avcodec_parameters_to_context(music->audio_dec_ctx, music->audio_stream->codecpar) < 0) {
        Mix_SetError("FFMPEG: Failed to copy codec parameters to context");
        FFMPEG_Delete(music);
        return NULL;
    }

    ret = avcodec_open2(music->audio_dec_ctx, music->codec, NULL);
    if (ret < 0) {
        Mix_SetError("FFMPEG: Failed to initialise the decoder: %s", av_err2str(ret));
        FFMPEG_Delete(music);
        return NULL;
    }

    music->decoded_frame = av_frame_alloc();
    if (!music->decoded_frame) {
        Mix_SetError("FFMPEG: Failed to allocate the frame");
        FFMPEG_Delete(music);
        return NULL;
    }

    music->pkt = av_packet_alloc();
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
        av_seek_frame(music->fmt_ctx, music->stream_index, 0, AVSEEK_FLAG_ANY);
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

    ret = avcodec_send_packet(music->audio_dec_ctx, pkt);
    if (ret < 0) {
        if (ret == AVERROR_EOF) {
            return ret;
        }

        Mix_SetError("ERROR: Error submitting a packet for decoding (%s)", av_err2str(ret));
        SDL_Log("FFMPEG: %s", Mix_GetError());
        return ret;
    }

    /* get all the available frames from the decoder */
    while (ret >= 0) {
        ret = avcodec_receive_frame(music->audio_dec_ctx, music->decoded_frame);
        if (ret < 0) {
            /* those two return values are special and mean there is no output */
            /* frame available, but there were no errors during decoding */
            if (ret == AVERROR_EOF || ret == AVERROR(EAGAIN)) {
                return 0;
            }

            Mix_SetError("FFMPEG: Error during decoding (%s)", av_err2str(ret));
            return ret;
        }

        FFMPEG_UpdateStream(music);

        if (music->planar) {
            sample_size = av_get_bytes_per_sample(music->decoded_frame->format);
            unpadded_linesize = sample_size * music->decoded_frame->nb_samples * music->schannels;

            if (unpadded_linesize > music->merge_buffer_size) {
                bump_merge_buffer(music, unpadded_linesize);
            }

            swr_convert(music->swr_ctx,
                        &music->merge_buffer, music->decoded_frame->nb_samples,
                        (const Uint8**)music->decoded_frame->extended_data, music->decoded_frame->nb_samples);

            music-> time_position += (double)music->decoded_frame->nb_samples / music->srate;

            if (SDL_AudioStreamPut(music->stream, music->merge_buffer, unpadded_linesize) < 0) {
                Mix_SetError("FFMPEG: Failed to put audio stream");
                return -1;
            }
        } else {
            unpadded_linesize = music->decoded_frame->nb_samples * av_get_bytes_per_sample(music->decoded_frame->format);
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
    while (av_read_frame(music->fmt_ctx, music->pkt) >= 0) {
        /* check if the packet belongs to a stream we are interested in, otherwise */
        /* skip it */
        if (music->pkt->stream_index == music->stream_index) {
            ret = decode_packet(music, music->pkt, &got_some);
        }
        av_packet_unref(music->pkt);

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
    AVRational timebase = music->audio_dec_ctx->time_base;
    int64_t ts = av_rescale(time, timebase.den, timebase.num);
    int err = avformat_seek_file(music->fmt_ctx, music->stream_index, 0, ts, ts, AVSEEK_FLAG_ANY);
    music-> time_position = time;

    if (err < 0) {
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
            avcodec_free_context(&music->audio_dec_ctx);
        }
        if (music->fmt_ctx) {
            avformat_close_input(&music->fmt_ctx);
        }
        if (music->pkt) {
            av_packet_free(&music->pkt);
        }
        if (music->decoded_frame) {
            av_frame_free(&music->decoded_frame);
        }
        if (music->swr_ctx) {
            swr_free(&music->swr_ctx);
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
