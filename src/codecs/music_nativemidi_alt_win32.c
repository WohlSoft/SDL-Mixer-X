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

#ifdef MUSIC_MID_NATIVE

/* This file supports playing MIDI files with OS APIs */

#include "SDL_timer.h"
#include "SDL_atomic.h"
#include "SDL_mutex.h"
#include "utils.h"
#include "music_nativemidi.h"
#include "midi_seq/win32_seq.h"
#include <windows.h>

typedef struct _NativeMidiSong
{
    void *song;
    SDL_atomic_t running;
    SDL_mutex *lock;
    SDL_mutex *pause;
    SDL_Thread* thread;
    SDL_bool paused;
    double tempo;
    int loops;
    int volume;
    Mix_MusicMetaTags tags;
} NativeMidiSong;


int native_midi_detect(void)
{
    HMIDIOUT out;
    MMRESULT err = midiOutOpen(&out, 0, 0, 0, CALLBACK_NULL);

    if (err == MMSYSERR_NOERROR) {
        return -1;
    }

    midiOutClose(out);

    return 1;
}

static int NativeMidiThread(void *context)
{
    NativeMidiSong *music = (NativeMidiSong *)context;
    HANDLE hTimer;
    LARGE_INTEGER liDueTime;
    int start, end;
    double t = 0.0001, w;

    liDueTime.QuadPart = -10000000LL;

    /* Create an unnamed waitable timer.*/
    hTimer = CreateWaitableTimerW(NULL, TRUE, NULL);
    if (NULL == hTimer) {
        Mix_SetError("Native MIDI Win32-Alt: CreateWaitableTimer failed (%lu)\n", (unsigned long)GetLastError());
        return 1;
    }

    win32_seq_init_midi_out(music->song);

    win32_seq_set_loop_enabled(music->song, music->loops < 0 ? 1 : 0);
    win32_seq_set_tempo_multiplier(music->song, music->tempo);

    SDL_AtomicSet(&music->running, 1);

    while (SDL_AtomicGet(&music->running)) {
        start = SDL_GetTicks();
        SDL_LockMutex(music->lock);
        if (win32_seq_at_end(music->song)) {
            SDL_UnlockMutex(music->lock);
            break;
        }
        t = win32_seq_tick(music->song, t, 0.0001);
        SDL_UnlockMutex(music->lock);
        end = SDL_GetTicks();

        w = (double)(end - start) / 1000;

        if ((t - w) > 0.0) {
            liDueTime.QuadPart = (LONGLONG)-((t - w) * 10000000.0);
            if (!SetWaitableTimer(hTimer, &liDueTime, 0, NULL, NULL, 0)) {
                Mix_SetError("Native MIDI Win32-Alt: SetWaitableTimer failed (%lu)\n", (unsigned long)GetLastError());
                return 2;
            }

            if (WaitForSingleObject(hTimer, INFINITE) != WAIT_OBJECT_0) {
                Mix_SetError("Native MIDI Win32-Alt: WaitForSingleObject failed (%lu)\n", (unsigned long)GetLastError());
            }
        }

        if (music->paused) { /* Hacky "pause" done by a mutex */
            SDL_LockMutex(music->pause);
            SDL_UnlockMutex(music->pause);
        }
    }

    SDL_AtomicSet(&music->running, 0);

    win32_seq_close_midi_out(music->song);
    CloseHandle(hTimer);

    return 0;
}

static NativeMidiSong *NATIVEMIDI_Create()
{
    NativeMidiSong *s = (NativeMidiSong*)SDL_calloc(1, sizeof(NativeMidiSong));
    if (s) {
        s->lock = SDL_CreateMutex();
        s->pause = SDL_CreateMutex();
        SDL_AtomicSet(&s->running, 0);
        s->loops = 0;
        s->tempo = 1.0;
        s->volume = MIX_MAX_VOLUME;
    }
    return s;
}

static void NATIVEMIDI_Destroy(NativeMidiSong *music)
{
    if (music) {
        meta_tags_clear(&music->tags);
        SDL_DestroyMutex(music->lock);
        SDL_DestroyMutex(music->pause);
        SDL_free(music);
    }
}

static void *NATIVEMIDI_CreateFromRW(SDL_RWops *src, int freesrc)
{
    void *bytes = 0;
    long filesize = 0;
    unsigned char byte[1];
    Sint64 length = 0;
    size_t bytes_l;
    NativeMidiSong *music = NATIVEMIDI_Create();

    if (!music) {
        SDL_OutOfMemory();
        return NULL;
    }

    length = SDL_RWseek(src, 0, RW_SEEK_END);
    if (length < 0) {
        Mix_SetError("NativeMIDI: wrong file\n");
        NATIVEMIDI_Destroy(music);
        return NULL;
    }

    SDL_RWseek(src, 0, RW_SEEK_SET);
    bytes = SDL_malloc((size_t)length);
    if (!bytes) {
        SDL_OutOfMemory();
        NATIVEMIDI_Destroy(music);
        return NULL;
    }

    filesize = 0;
    while ((bytes_l = SDL_RWread(src, &byte, sizeof(Uint8), 1)) != 0) {
        ((Uint8 *)bytes)[filesize] = byte[0];
        filesize++;
    }

    if (filesize == 0) {
        SDL_free(bytes);
        NATIVEMIDI_Destroy(music);
        Mix_SetError("NativeMIDI: wrong file\n");
        return NULL;
    }

    music->song = win32_seq_init_interface();
    if (!music->song) {
        SDL_OutOfMemory();
        NATIVEMIDI_Destroy(music);
        return NULL;
    }


    if (win32_seq_openData(music->song, bytes, filesize) < 0) {
        Mix_SetError("NativeMIDI: %s", win32_seq_get_error(music->song));
        NATIVEMIDI_Destroy(music);
        return NULL;
    }

    if (freesrc) {
        SDL_RWclose(src);
    }

    meta_tags_init(&music->tags);
    meta_tags_set_from_midi(&music->tags, MIX_META_TITLE, win32_seq_meta_title(music->song));
    meta_tags_set_from_midi(&music->tags, MIX_META_COPYRIGHT, win32_seq_meta_copyright(music->song));
    return music;
}

static int NATIVEMIDI_Play(void *context, int play_count)
{
    NativeMidiSong *music = (NativeMidiSong *)context;
    int loops = play_count;
    if (loops > 0) {
        --loops;
    }

    if (!SDL_AtomicGet(&music->running) && !music->thread) {
        music->loops = loops;
        SDL_AtomicSet(&music->running, 1);
        music->thread = SDL_CreateThread(NativeMidiThread, "NativeMidiLoop", (void *)music);
    }

    return 0;
}

static void NATIVEMIDI_SetVolume(void *context, int volume)
{
    NativeMidiSong *music = (NativeMidiSong *)context;
    double vf = (double)volume / MIX_MAX_VOLUME;
    music->volume = volume;
    SDL_LockMutex(music->lock);
    win32_seq_set_volume(music->song, vf);
    SDL_UnlockMutex(music->lock);
}

static int NATIVEMIDI_GetVolume(void *context)
{
    NativeMidiSong *music = (NativeMidiSong *)context;
    return music->volume;
}

static SDL_bool NATIVEMIDI_IsPlaying(void *context)
{
    NativeMidiSong *music = (NativeMidiSong *)context;
    return SDL_AtomicGet(&music->running) ? SDL_TRUE : SDL_FALSE;
}

static void NATIVEMIDI_Pause(void *context)
{
    NativeMidiSong *music = (NativeMidiSong *)context;
    if (!music->paused) {
        SDL_LockMutex(music->pause);
        music->paused = SDL_TRUE;
        win32_seq_all_notes_off(music->song);
    }
}

static void NATIVEMIDI_Resume(void *context)
{
    NativeMidiSong *music = (NativeMidiSong *)context;
    if (music->paused) {
        music->paused = SDL_FALSE;
        SDL_UnlockMutex(music->pause);
    }
}

static void NATIVEMIDI_Stop(void *context)
{
    NativeMidiSong *music = (NativeMidiSong *)context;

    if (music->paused) { /* Unpause if paused */
        music->paused = SDL_FALSE;
        SDL_UnlockMutex(music->pause);
    }

    if (music && NATIVEMIDI_IsPlaying(context)) {
        SDL_AtomicSet(&music->running, 0);
        SDL_WaitThread(music->thread, NULL);
    }
}

static void NATIVEMIDI_Delete(void *context)
{
    NativeMidiSong *music = (NativeMidiSong *)context;
    NATIVEMIDI_Stop(music);
    if (music->song) {
        win32_seq_free(music->song);
    }
    NATIVEMIDI_Destroy(music);
}

static const char* NATIVEMIDI_GetMetaTag(void *context, Mix_MusicMetaTag tag_type)
{
    NativeMidiSong *music = (NativeMidiSong *)context;
    return meta_tags_get(&music->tags, tag_type);
}

/* Jump (seek) to a given position (time is in seconds) */
static int NATIVEMIDI_Seek(void *context, double time)
{
    NativeMidiSong *music = (NativeMidiSong *)context;
    SDL_LockMutex(music->lock);
    win32_seq_seek(music->song, time);
    SDL_UnlockMutex(music->lock);
    return 0;
}

static double NATIVEMIDI_Tell(void *context)
{
    NativeMidiSong *music = (NativeMidiSong *)context;
    double ret;
    if (music) {
        SDL_LockMutex(music->lock);
        ret = win32_seq_tell(music->song);
        SDL_UnlockMutex(music->lock);
        return ret;
    }
    return -1;
}

static double NATIVEMIDI_Duration(void *context)
{
    double ret;
    NativeMidiSong *music = (NativeMidiSong *)context;
    if (music) {
        SDL_LockMutex(music->lock);
        ret = win32_seq_length(music->song);
        SDL_UnlockMutex(music->lock);
        return ret;
    }
    return -1;
}

static int NATIVEMIDI_SetTempo(void *context, double tempo)
{
    NativeMidiSong *music = (NativeMidiSong *)context;
    if (music && (tempo > 0.0)) {
        SDL_LockMutex(music->lock);
        win32_seq_set_tempo_multiplier(music->song, tempo);
        SDL_UnlockMutex(music->lock);
        music->tempo = tempo;
        return 0;
    }
    return -1;
}

static double NATIVEMIDI_GetTempo(void *context)
{
    NativeMidiSong *music = (NativeMidiSong *)context;
    if (music) {
        return music->tempo;
    }
    return -1.0;
}

static double NATIVEMIDI_LoopStart(void *context)
{
    NativeMidiSong *music = (NativeMidiSong *)context;
    double ret;

    if (music) {
        SDL_LockMutex(music->lock);
        ret = win32_seq_loop_start(music->song);
        SDL_UnlockMutex(music->lock);
        return ret;
    }
    return -1;
}

static double NATIVEMIDI_LoopEnd(void *context)
{
    NativeMidiSong *music = (NativeMidiSong *)context;
    double ret;

    if (music) {
        SDL_LockMutex(music->lock);
        ret = win32_seq_loop_end(music->song);
        SDL_UnlockMutex(music->lock);
        return ret;
    }
    return -1;
}

static double NATIVEMIDI_LoopLength(void *context)
{
    NativeMidiSong *music = (NativeMidiSong *)context;
    double start, end;
    if (music) {
        SDL_LockMutex(music->lock);
        start = win32_seq_loop_start(music->song);
        end = win32_seq_loop_end(music->song);
        SDL_UnlockMutex(music->lock);
        if (start >= 0 && end >= 0) {
            return (end - start);
        }
    }
    return -1;
}

Mix_MusicInterface Mix_MusicInterface_NATIVEMIDI =
{
    "NATIVEMIDI",
    MIX_MUSIC_NATIVEMIDI,
    MUS_MID,
    SDL_FALSE,
    SDL_FALSE,

    NULL,   /* Load */
    NULL,   /* Open */
    NATIVEMIDI_CreateFromRW,
    NULL,   /* CreateFromRWex [MIXER-X]*/
    NULL,   /* CreateFromFile */
    NULL,   /* CreateFromFileEx [MIXER-X]*/
    NATIVEMIDI_SetVolume,
    NATIVEMIDI_GetVolume,   /* GetVolume [MIXER-X]*/
    NATIVEMIDI_Play,
    NATIVEMIDI_IsPlaying,
    NULL,   /* GetAudio */
    NATIVEMIDI_Seek,   /* Seek */
    NATIVEMIDI_Tell,   /* Tell [MIXER-X]*/
    NATIVEMIDI_Duration,   /* Duration */
    NATIVEMIDI_SetTempo,   /* Set Tempo multiplier [MIXER-X] */
    NATIVEMIDI_GetTempo,   /* Get Tempo multiplier [MIXER-X] */
    NATIVEMIDI_LoopStart,   /* LoopStart [MIXER-X]*/
    NATIVEMIDI_LoopEnd,   /* LoopEnd [MIXER-X]*/
    NATIVEMIDI_LoopLength,   /* LoopLength [MIXER-X]*/
    NATIVEMIDI_GetMetaTag,   /* GetMetaTag [MIXER-X]*/
    NATIVEMIDI_Pause,
    NATIVEMIDI_Resume,
    NATIVEMIDI_Stop,
    NATIVEMIDI_Delete,
    NULL,   /* Close */
    NULL    /* Unload */
};

#endif /* MUSIC_MID_NATIVE */

/* vi: set ts=4 sw=4 expandtab: */
