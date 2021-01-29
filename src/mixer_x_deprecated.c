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

#include "SDL_mixer.h"

/*
    Deprecated SDL Mixer X's functions.
    There are kept to don't break ABI with existing apps.
*/

void SDLCALLCC MIX_Timidity_addToPathList(const char *path)
{
    Mix_SetTimidityCfg(path);
}

void SDLCALLCC Mix_Timidity_addToPathList(const char *path)
{
    Mix_SetTimidityCfg(path);
}

const char *const *SDLCALLCC  MIX_ADLMIDI_getBankNames()
{
#ifdef MUSIC_MID_ADLMIDI
    return Mix_ADLMIDI_getBankNames();
#else
    return NULL;
#endif
}

int SDLCALLCC MIX_ADLMIDI_getBankID()
{
#ifdef MUSIC_MID_ADLMIDI
    return Mix_ADLMIDI_getBankID();
#else
    return 0;
#endif
}

void SDLCALLCC MIX_ADLMIDI_setBankID(int bnk)
{
#ifdef MUSIC_MID_ADLMIDI
    Mix_ADLMIDI_setBankID(bnk);
#else
    (void)bnk;
#endif
}

int SDLCALLCC MIX_ADLMIDI_getTremolo()
{
#ifdef MUSIC_MID_ADLMIDI
    return Mix_ADLMIDI_getTremolo();
#else
    return -1;
#endif
}

void SDLCALLCC MIX_ADLMIDI_setTremolo(int tr)
{
#ifdef MUSIC_MID_ADLMIDI
    Mix_ADLMIDI_setTremolo(tr);
#else
    (void)tr;
#endif
}

int SDLCALLCC MIX_ADLMIDI_getVibrato()
{
#ifdef MUSIC_MID_ADLMIDI
    return Mix_ADLMIDI_getVibrato();
#else
    return -1;
#endif
}

void SDLCALLCC MIX_ADLMIDI_setVibrato(int vib)
{
#ifdef MUSIC_MID_ADLMIDI
    Mix_ADLMIDI_setVibrato(vib);
#else
    (void)vib;
#endif
}

int SDLCALLCC MIX_ADLMIDI_getScaleMod()
{
#ifdef MUSIC_MID_ADLMIDI
    return Mix_ADLMIDI_getScaleMod();
#else
    return -1;
#endif
}

void SDLCALLCC MIX_ADLMIDI_setScaleMod(int sc)
{
#ifdef MUSIC_MID_ADLMIDI
    Mix_ADLMIDI_setScaleMod(sc);
#else
    (void)sc;
#endif
}

int SDLCALLCC MIX_ADLMIDI_getAdLibMode()
{
    return -1;
}

void SDLCALLCC MIX_ADLMIDI_setAdLibMode(int sc)
{
    (void)sc;
}

void SDLCALLCC MIX_ADLMIDI_setSetDefaults()
{
#ifdef MUSIC_MID_ADLMIDI
    Mix_ADLMIDI_setSetDefaults();
#endif
}

int SDLCALLCC MIX_ADLMIDI_getLogarithmicVolumes()
{
#ifdef MUSIC_MID_ADLMIDI
    return Mix_ADLMIDI_getLogarithmicVolumes();
#else
    return -1;
#endif
}

void SDLCALLCC MIX_ADLMIDI_setLogarithmicVolumes(int lv)
{
#ifdef MUSIC_MID_ADLMIDI
    Mix_ADLMIDI_setLogarithmicVolumes(lv);
#else
    (void)lv;
#endif
}

int SDLCALLCC MIX_ADLMIDI_getVolumeModel()
{
#ifdef MUSIC_MID_ADLMIDI
    return Mix_ADLMIDI_getVolumeModel();
#else
    return -1;
#endif
}

void SDLCALLCC MIX_ADLMIDI_setVolumeModel(int vm)
{
#ifdef MUSIC_MID_ADLMIDI
    Mix_ADLMIDI_setVolumeModel(vm);
#else
    (void)vm;
#endif
}


void SDLCALLCC MIX_OPNMIDI_setCustomBankFile(const char *bank_wonp_path)
{
#ifdef MUSIC_MID_ADLMIDI
    Mix_OPNMIDI_setCustomBankFile(bank_wonp_path);
#else
    (void)bank_wonp_path;
#endif
}


int SDLCALLCC MIX_SetMidiDevice(int device)
{
    return Mix_SetMidiPlayer(device);
}

void SDLCALLCC MIX_SetLockMIDIArgs(int lock_midiargs)
{
    Mix_SetLockMIDIArgs(lock_midiargs);
}

int SDLCALLCC Mix_GetMidiDevice()
{
    return Mix_GetMidiPlayer();
}

int SDLCALLCC Mix_GetNextMidiDevice()
{
    return Mix_GetNextMidiPlayer();
}

int SDLCALLCC Mix_SetMidiDevice(int player)
{
    return Mix_SetMidiPlayer(player);
}

int SDLCALLCC Mix_ADLMIDI_getAdLibMode()
{
    return -1;
}

void SDLCALLCC Mix_ADLMIDI_setAdLibMode(int ald)
{
    (void)ald;
}
