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

#ifdef MUSIC_VGM

#include <cstdio>
#include <cstdlib>
#include <cstring>

#include <vgm/player/playerbase.hpp>
#include <vgm/player/vgmplayer.hpp>
#include <vgm/player/s98player.hpp>
#include <vgm/player/droplayer.hpp>
#include <vgm/player/gymplayer.hpp>
#include <vgm/player/playera.hpp>
#include <vgm/utils/DataLoader.h>
#include <vgm/utils/MemoryLoader.h>
#include <vgm/emu/SoundDevs.h>
#include <vgm/emu/EmuCores.h>
#include <vgm/emu/SoundEmu.h>

extern "C" {
    #include "music_vgm.h"
}


static int VGM_Load(void)
{
    return 0;
}

static void VGM_Unload(void)
{}

/* Global flags which are applying on initializing of GME player with a file */
typedef struct {
    int track_number;
    double tempo;
    double gain;
} VGM_Setup;

static VGM_Setup gme_setup = {
    0, 1.0, 1.0
};

static void VGM_SetDefault(VGM_Setup *setup)
{
    setup->track_number = 0;
    setup->tempo = 1.0;
    setup->gain = 1.0;
}


/* This file supports Game Music Emulator music streams */
typedef struct
{
    SDL_RWops *src;
    int freesrc;
    Uint8 *mem;
    size_t mem_size;
    int play_count;
    PlayerA *player;
    DATA_LOADER *loader;
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
} VGM_Music;

static void VGM_delete(void *context);

/* Set the volume for a GME stream */
void VGM_setvolume(void *music_p, int volume)
{
    VGM_Music *music = (VGM_Music*)music_p;
    double v = SDL_floor(((double)(volume) * music->gain) + 0.5);
    music->volume = (int)v;
}

/* Get the volume for a GME stream */
int VGM_getvolume(void *music_p)
{
    VGM_Music *music = (VGM_Music*)music_p;
    double v = SDL_floor(((double)(music->volume) / music->gain) + 0.5);
    return (int)v;
}

static void process_args(const char *args, VGM_Setup *setup)
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
    VGM_SetDefault(setup);

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

struct RW_LOADER
{
    SDL_RWops *src;
    size_t lastRead;
};

static UINT8 RWOpsLoader_dopen(void *context)
{
    RW_LOADER *loader = (RW_LOADER*)context;
    SDL_RWseek(loader->src, 0, SEEK_SET);
    loader->lastRead = 1;
    return 0x00;
}

static UINT32 RWOpsLoader_dread(void *context, UINT8 *buffer, UINT32 numBytes)
{
    RW_LOADER *loader = (RW_LOADER*)context;
    loader->lastRead = SDL_RWread(loader->src, buffer, 1, numBytes);
    return loader->lastRead;
}

static UINT8 RWOpsLoader_dseek(void *context, UINT32 offset, UINT8 whence)
{
    RW_LOADER *loader = (RW_LOADER*)context;
    return (UINT32)SDL_RWseek(loader->src, offset, whence);
}

static INT32 RWOpsLoader_dtell(void *context)
{
    RW_LOADER *loader = (RW_LOADER*)context;
    return (UINT32)SDL_RWtell(loader->src);
}

static UINT8 RWOpsLoader_dclose(void *context)
{
    RW_LOADER *loader = (RW_LOADER*)context;
    SDL_RWseek(loader->src, 0, SEEK_SET);
    return 0x00;
}

static UINT32 RWOpsLoader_dlength(void *context)
{
    RW_LOADER *loader = (RW_LOADER*)context;
    return SDL_RWsize(loader->src);
}

static UINT8 RWOpsLoader_deof(void *context)
{
    RW_LOADER *loader = (RW_LOADER*)context;
    return loader->lastRead == 0;
}

const DATA_LOADER_CALLBACKS rwOpsLoader =
{
    0x52576F70,		// "RWop"
    "SDLRWops Loader",
    RWOpsLoader_dopen,
    RWOpsLoader_dread,
    RWOpsLoader_dseek,
    RWOpsLoader_dclose,
    RWOpsLoader_dtell,
    RWOpsLoader_dlength,
    RWOpsLoader_deof,
    NULL,
};

DATA_LOADER *RWOpsLoader_Init(SDL_RWops *src)
{
    DATA_LOADER *dLoader;
    RW_LOADER *mLoader;

    dLoader = (DATA_LOADER *)calloc(1, sizeof(DATA_LOADER));
    if (dLoader == NULL) {
        return NULL;
    }

    mLoader = (RW_LOADER*)calloc(1, sizeof(RW_LOADER));
    if (mLoader == NULL) {
        free(dLoader);
        return NULL;
    }

    mLoader->src = src;

    DataLoader_Setup(dLoader, &rwOpsLoader, mLoader);

    return dLoader;
}


VGM_Music *VGM_LoadSongRW(SDL_RWops *src, int freesrc, const char *args)
{
//    void *mem = 0;
//    size_t size;
    PlayerBase* plrEngine;
    VGM_Music *music;
    VGM_Setup setup = gme_setup;
//    const char *err;
    SDL_bool has_loop_length = SDL_TRUE;

    if (src == NULL) {
        Mix_SetError("VGM: Empty source given");
        return NULL;
    }

    process_args(args, &setup);

    music = (VGM_Music *)SDL_calloc(1, sizeof(VGM_Music));

    music->src = src;

    music->tempo = setup.tempo;
    music->gain = setup.gain;

    music->stream = SDL_NewAudioStream(AUDIO_S16SYS, music_spec.channels, music_spec.freq,
                                       music_spec.format, music_spec.channels, music_spec.freq);
    if (!music->stream) {
        VGM_delete(music);
        return NULL;
    }

    music->player = new PlayerA();
    if (!music->player) {
        SDL_OutOfMemory();
        VGM_delete(music);
        return NULL;
    }

    music->player->SetSampleRate(music_spec.freq);

    /* Register all player engines.
     * libvgm will automatically choose the correct one depending on the file format. */
    music->player->RegisterPlayerEngine(new VGMPlayer);
    music->player->RegisterPlayerEngine(new S98Player);
    music->player->RegisterPlayerEngine(new DROPlayer);
    music->player->RegisterPlayerEngine(new GYMPlayer);

    if(music->player->SetOutputSettings(music_spec.freq, music_spec.channels, 16, music_spec.size)) {
        VGM_delete(music);
        Mix_SetError("VGM: unsupported audio output settings");
        return NULL;
    }

    SDL_RWseek(src, 0, RW_SEEK_SET);
    music->mem = (Uint8*)SDL_LoadFile_RW(src, &music->mem_size, SDL_FALSE);

    music->loader = MemoryLoader_Init(music->mem, music->mem_size);
//    music->loader = RWOpsLoader_Init(src);
    if (!music->loader) {
        SDL_OutOfMemory();
        VGM_delete(music);
        return NULL;
    }

    if (DataLoader_Load(music->loader)) {
        DataLoader_CancelLoading(music->loader);
        VGM_delete(music);
        Mix_SetError("VGM: failed to load DataLoader");
        return NULL;
    }

    music->buffer_size = music_spec.size;
    music->buffer = SDL_malloc(music->buffer_size);
    if (!music->buffer) {
        SDL_OutOfMemory();
        VGM_delete(music);
        return NULL;
    }

//    SDL_RWseek(src, 0, RW_SEEK_SET);
//    mem = SDL_LoadFile_RW(src, &size, SDL_FALSE);
    UINT8 ret = music->player->LoadFile(music->loader);
    if(ret)
    {
        VGM_delete(music);
        Mix_SetError("VGM: failed to load file");
        return NULL;
    }

    plrEngine = music->player->GetPlayer();

    /*
    if (plrEngine->GetPlayerType() == FCC_VGM) {
        VGMPlayer* vgmplay = dynamic_cast<VGMPlayer*>(plrEngine);
        music->player->SetLoopCount(vgmplay->GetModifiedLoopCount(loops));
    }*/

    // gme.gme_set_tempo(music->game_emu, music->tempo);

    music->volume = MIX_MAX_VOLUME;
    meta_tags_init(&music->tags);

    /*
    tags = plrEngine->GetTags();
    while(*tags) {
        fprintf(stderr,"%s: %s\n",tags[0],tags[1]);
        tags += 2;
    }*/

    music->track_length = music->player->GetTotalTime(1);
    music->intro_length = 0;
    music->loop_length = music->player->GetLoopTime();

    music->has_track_length = SDL_TRUE;
    if (music->track_length <= 0 ) {
        music->has_track_length = SDL_FALSE;
    }

    if (music->intro_length < 0 ) {
        music->intro_length = 0;
    }
    if (music->loop_length <= 0 ) {
        if (music->track_length > 0) {
            music->loop_length = music->track_length;
        }
        has_loop_length = SDL_FALSE;
    }

    if (!music->has_track_length && has_loop_length) {
        music->track_length = music->intro_length + music->loop_length;
        music->has_track_length = SDL_TRUE;
    }

//    meta_tags_set(&music->tags, MIX_META_TITLE, musInfo->song);
//    meta_tags_set(&music->tags, MIX_META_ARTIST, musInfo->author);
//    meta_tags_set(&music->tags, MIX_META_ALBUM, musInfo->game);
//    meta_tags_set(&music->tags, MIX_META_COPYRIGHT, musInfo->copyright);
//    gme.gme_free_info(musInfo);

    music->freesrc = freesrc;

    return music;
}

/* Load a Game Music Emulators stream from an SDL_RWops object */
static void *VGM_new_RWEx(struct SDL_RWops *src, int freesrc, const char *extraSettings)
{
    VGM_Music *gmeMusic;

    gmeMusic = VGM_LoadSongRW(src, freesrc, extraSettings);
    if (!gmeMusic) {
        return NULL;
    }
    return gmeMusic;
}

static void *VGM_new_RW(struct SDL_RWops *src, int freesrc)
{
    return VGM_new_RWEx(src, freesrc, "0");
}

/* Start playback of a given Game Music Emulators stream */
static int VGM_play(void *music_p, int play_count)
{
    VGM_Music *music = (VGM_Music*)music_p;
//    int fade_start;
    if (music) {
        SDL_AudioStreamClear(music->stream);
        music->play_count = play_count;
        music->player->SetLoopCount(play_count);
        music->player->Start();
//        fade_start = play_count > 0 ? music->intro_length + (music->loop_length * play_count) : -1;
//#if VGM_VERSION >= 0x000700
//        gme.gme_set_fade(music->game_emu, fade_start, 8000);
//#else
//        gme.gme_set_fade(music->game_emu, fade_start);
//#endif
//        gme.gme_seek(music->game_emu, 0);
    }
    return 0;
}

static int VGM_GetSome(void *context, void *data, int bytes, SDL_bool *done)
{
    VGM_Music *music = (VGM_Music*)context;
    int filled;
//    const char *err = NULL;

    filled = SDL_AudioStreamGet(music->stream, data, bytes);
    if (filled != 0) {
        return filled;
    }

//    if (music->player->GetState()) {
//        /* All done */
//        *done = SDL_TRUE;
//        return 0;
//    }

    music->player->Render(music->buffer_size, music->buffer);

//    if (err != NULL) {
//        Mix_SetError("GME: %s", err);
//        return 0;
//    }

    if (SDL_AudioStreamPut(music->stream, music->buffer, music->buffer_size) < 0) {
        return -1;
    }
    return 0;
}

/* Play some of a stream previously started with VGM_play() */
static int VGM_playAudio(void *music_p, void *data, int bytes)
{
    VGM_Music *music = (VGM_Music*)music_p;
    return music_pcm_getaudio(music_p, data, bytes, music->volume, VGM_GetSome);
}

/* Close the given Game Music Emulators stream */
static void VGM_delete(void *context)
{
    VGM_Music *music = (VGM_Music*)context;
    if (music) {
        meta_tags_clear(&music->tags);
        if (music->player) {
            music->player->Stop();
            music->player->UnloadFile();
            music->player->UnregisterAllPlayers();
            delete music->player;
            music->player = NULL;
        }
        if (music->loader) {
            DataLoader_Deinit(music->loader);
            music->loader = NULL;
        }
        if (music->stream) {
            SDL_FreeAudioStream(music->stream);
        }
        if (music->buffer) {
            SDL_free(music->buffer);
        }
        if (music->mem) {
            SDL_free(music->mem);
            music->mem = NULL;
        }
        if (music->freesrc) {
            SDL_RWclose(music->src);
        }
        SDL_free(music);
    }
}

static const char* VGM_GetMetaTag(void *context, Mix_MusicMetaTag tag_type)
{
    VGM_Music *music = (VGM_Music *)context;
    return meta_tags_get(&music->tags, tag_type);
}

/* Jump (seek) to a given position (time is in seconds) */
static int VGM_Seek(void *music_p, double time)
{
    VGM_Music *music = (VGM_Music*)music_p;
    return 0;
}

static double VGM_Tell(void *music_p)
{
    VGM_Music *music = (VGM_Music*)music_p;
    return music->player->GetCurTime(0);
}

static double VGM_Duration(void *music_p)
{
    VGM_Music *music = (VGM_Music*)music_p;
    return music->player->GetTotalTime(0);
}

static int VGM_SetTempo(void *music_p, double tempo)
{
    VGM_Music *music = (VGM_Music *)music_p;
    if (music && (tempo > 0.0)) {
        music->tempo = tempo;
        music->player->SetPlaybackSpeed(tempo);
        PlayerBase *plrEngine = music->player->GetPlayer();
        if (plrEngine->GetPlayerType() == FCC_VGM) {
            VGMPlayer* vgmplay = dynamic_cast<VGMPlayer*>(plrEngine);
            vgmplay->SetPlaybackSpeed(tempo);
        }
        return 0;
    }
    return -1;
}

static double VGM_GetTempo(void *music_p)
{
    VGM_Music *music = (VGM_Music *)music_p;
    if (music) {
        return music->player->GetPlaybackSpeed();
    }
    return -1.0;
}

static int VGM_GetTracksCount(void *music_p)
{
    VGM_Music *music = (VGM_Music *)music_p;
    if (music) {
        return -1;
    }
    return -1;
}

static int VGM_SetTrackMute(void *music_p, int track, int mute)
{
    VGM_Music *music = (VGM_Music *)music_p;
    if (music) {
        return 0;
    }
    return -1;
}


Mix_MusicInterface Mix_MusicInterface_VGM =
{
    "VGM",
    MIX_MUSIC_VGM,
    MUS_VGM,
    SDL_FALSE,
    SDL_FALSE,

    VGM_Load,
    NULL,   /* Open */
    VGM_new_RW,
    VGM_new_RWEx,   /* CreateFromRWex [MIXER-X]*/
    NULL,   /* CreateFromFile */
    NULL,   /* CreateFromFileEx [MIXER-X]*/
    VGM_setvolume,
    VGM_getvolume,   /* GetVolume [MIXER-X]*/
    VGM_play,
    NULL,   /* IsPlaying */
    VGM_playAudio,
    NULL,       /* Jump */
    VGM_Seek,   /* Seek */
    VGM_Tell,   /* Tell [MIXER-X]*/
    VGM_Duration,
    VGM_SetTempo,   /* [MIXER-X] */
    VGM_GetTempo,   /* [MIXER-X] */
    VGM_GetTracksCount,
    VGM_SetTrackMute,
    NULL,   /* LoopStart [MIXER-X]*/
    NULL,   /* LoopEnd [MIXER-X]*/
    NULL,   /* LoopLength [MIXER-X]*/
    VGM_GetMetaTag,/* GetMetaTag [MIXER-X]*/
    NULL,   /* Pause */
    NULL,   /* Resume */
    NULL,   /* Stop */
    VGM_delete,
    NULL,   /* Close */
    VGM_Unload
};

#endif /* MUSIC_GME */
