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

#define inline SDL_INLINE
#define av_always_inline SDL_INLINE
#define __STDC_CONSTANT_MACROS

#include <libavutil/frame.h>
#include <libavutil/mem.h>

#include <libavcodec/avcodec.h>


/* This file supports Game Music Emulator music streams */
typedef struct
{
    SDL_RWops *src;
    int freesrc;
    const AVCodec *codec;
    AVCodecContext *c;
    AVCodecParserContext *parser;
    AVPacket *pkt;
    AVFrame *decoded_frame;
    enum AVSampleFormat sfmt;
    int play_count;
    int volume;
    SDL_AudioStream *stream;
    void *buffer;
    size_t buffer_size;
} FFMPEG_Music;


static void FFMPEG_Delete(void *context);

static void *FFMPEG_NewRW(struct SDL_RWops *src, int freesrc)
{
    FFMPEG_Music *music = NULL;

    music = (FFMPEG_Music *)SDL_calloc(1, sizeof *music);
    if (!music) {
        SDL_OutOfMemory();
        return NULL;
    }

    music->src = src;
    music->volume = MIX_MAX_VOLUME;

    music->pkt = av_packet_alloc();

    music->codec = avcodec_find_decoder(AV_CODEC_ID_AAC);
    if (!music->codec) {
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
        music->play_count = play_count;
    }
    return 0;
}

static int FFMPEG_GetSome(void *context, void *data, int bytes, SDL_bool *done)
{

}


/* Play some of a stream previously started with GME_play() */
static int FFMPEG_PlayAudio(void *music_p, void *data, int bytes)
{
    FFMPEG_Music *music = (FFMPEG_Music*)music_p;
    return music_pcm_getaudio(music_p, data, bytes, music->volume, FFMPEG_GetSome);
}

/* Close the given Game Music Emulators stream */
static void FFMPEG_Delete(void *context)
{
    FFMPEG_Music *music = (FFMPEG_Music*)context;
    if (music) {
        avcodec_free_context(&music->c);
        av_parser_close(music->parser);
        av_frame_free(&music->decoded_frame);
        av_packet_free(&music->pkt);
        if (music->stream) {
            SDL_FreeAudioStream(music->stream);
        }
        if (music->buffer) {
            SDL_free(music->buffer);
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

    NULL,   /* Load */
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
    NULL,   /* Seek */
    NULL,   /* Tell [MIXER-X]*/
    NULL,   /* Duration */
    NULL,   /* [MIXER-X] */
    NULL,   /* [MIXER-X] */
    NULL,
    NULL,
    NULL,   /* LoopStart [MIXER-X]*/
    NULL,   /* LoopEnd [MIXER-X]*/
    NULL,   /* LoopLength [MIXER-X]*/
    NULL,/* GetMetaTag [MIXER-X]*/
    NULL,   /* Pause */
    NULL,   /* Resume */
    NULL,   /* Stop */
    FFMPEG_Delete,
    NULL,   /* Close */
    NULL,   /* Unload */
};


#endif /* MUSIC_FFMPEG */
