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

#ifndef STREAM_CUSTOM_H
#define STREAM_CUSTOM_H

#include "SDL_stdinc.h"

/*
    This is a wrapper over the SDL AudioStream that provides the
    custom and extremely simple resampler for the very low end hardware
 */

#ifndef SDL_audio_h_
typedef Uint16 SDL_AudioFormat;
#endif

struct _Mix_AudioStream;
typedef struct _Mix_AudioStream Mix_AudioStream;

Mix_AudioStream *Mix_NewAudioStream(const SDL_AudioFormat src_format,
                                    const Uint8 src_channels,
                                    const int src_rate,
                                    const SDL_AudioFormat dst_format,
                                    const Uint8 dst_channels,
                                    const int dst_rate);

int Mix_AudioStreamPut(Mix_AudioStream *stream, const void *buf, int len);
int Mix_AudioStreamGet(Mix_AudioStream *stream, void *buf, int len);
int Mix_AudioStreamAvailable(Mix_AudioStream *stream);
int Mix_AudioStreamFlush(Mix_AudioStream *stream);
void Mix_AudioStreamClear(Mix_AudioStream *stream);
void Mix_FreeAudioStream(Mix_AudioStream *stream);

#ifndef STREAM_CUSTOM_INTERNAL
#define SDL_NewAudioStream          Mix_NewAudioStream
#define SDL_AudioStreamPut          Mix_AudioStreamPut
#define SDL_AudioStreamGet          Mix_AudioStreamGet
#define SDL_AudioStreamAvailable    Mix_AudioStreamAvailable
#define SDL_AudioStreamFlush        Mix_AudioStreamFlush
#define SDL_AudioStreamClear        Mix_AudioStreamClear
#define SDL_FreeAudioStream         Mix_FreeAudioStream
#define SDL_AudioStream             Mix_AudioStream
#endif

#endif /* STREAM_CUSTOM_H */
