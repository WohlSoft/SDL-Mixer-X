# MixerX - the extended SDL_mixer
A fork of [SDL_mixer](http://www.libsdl.org/projects/SDL_mixer/) library

# Description
SDL_Mixer is a sample multi-channel audio mixer library.
It supports any number of simultaneously playing channels of 16 bit stereo audio,
plus a single channel of music, mixed by the popular FLAC, OPUS, XMP, ModPlug,
MikMod MOD, Timidity MIDI, FluidSynth, Ogg Vorbis, FLAC, libMAD, and MPG123 MP3 libraries.

SDL Mixer X (or shorty MixerX) - It’s an extended fork of the SDL_mixer library
made by Vitaly Novichkov "Wohlstand" in 13 January 2015. SDL_mixer is a quick and
stable high-quality audio library, however, it has own bunch of deffects and lack
of certain abilities such as inability to choose the MIDI backend in runtime,
No support for cross-fade and parallel streams playing (has only one musical stream.
Using of very long Chunks is ineffectively), etc. The goal of this fork is resolving
those issues, providing more extended functionality than was originally, and
providing support for more supported audio formats.

## New features of MixerX in comparison to original SDL_mixer
* Added much more music formats (Such a game music emulators liks NSF, VGM, HES, GBM, etc. playing via GME library, also XMI, MUS, and IMF playing via [ADLMIDI](https://github.com/Wohlstand/libADLMIDI) library)
* Support for multiple parallel music (or atmpsphere) streams using new-added API functions including the cross-fade support
* Better MIDI support:
  * Added ability to choose any of available MIDI backends in runtime
  * Added ability to append custom config path for Timidity synthesizer, then no more limit to default patches set
  * Forked version now has [ADLMIDI](https://github.com/Wohlstand/libADLMIDI) midi sequences together with Native MIDI, Timidity and Fluidsynth. ADLMIDI is [OPL-Synth](http://wohlsoft.ru/pgewiki/FM_Synthesis) Emulation based MIDI player. Unlike to other MIDI synthesizers are using in SDL Mixer X (except of Native MIDI), ADLMIDI is completely standalone software synthesizer which never requires any external sound fonts or banks to work.
  * Also, the [OPNMIDI](https://github.com/Wohlstand/libOPNMIDI) was added which a MIDI player through emulator of YM2612 chip which was widely used on Sega Megadrive/Genesis game console. Also is fully standalone like ADLMIDI.
* Added new API functions
  * Added ADLMIDI Extra functions: Change bank ID, enable/disable high-level tremolo, enable/disable high-level vibrato, enable/disable scalable modulation, Set path to custom bank file (You can use [this editor](https://github.com/Wohlstand/OPL3BankEditor) to create or edit them)
  * Added OPNMIDI Extra function: Set path to custom bank file (You can use [this editor](https://github.com/Wohlstand/OPN2BankEditor) to create or edit them)
  * Music tempo change function: you can change speed of playing music by giving the factor value while music is playing. Works for MIDI (libADLMIDI and libOPNMIDI), GME, and XMP.
  * Added music functions with the `Stream` suffix which can alter individual properties of every opened music and even play them concurrently.
* Added an ability to set individual effects per every `Mix_Music` instance.
* Built-in effects (such as 3D-position, distance, panning, and reverse stereo) now supported per every `Mix_Music` instance
* Added support of [extra arguments](http://wohlsoft.ru/pgewiki/SDL_Mixer_X#Path_arguments) in the tail of the file path, passed into Mix_LoadMUS function.
* Added ability to build shared library in the <u>stdcall</u> mode with static linking of libSDL on Windows to use it as independent audio library with other languages like VB6 or .NET.
* CMake building systems in use
  * Can be used with libraries currently installed in the system.
  * Optionally, you can set the `-DDOWNLOAD_AUDIO_CODECS_DEPENDENCY=ON` option to automatically download sources of all dependencies and build them in a place.

### Features introduced at MixerX later added into the mainstream SDL_mixer
* Added support of the loop points in the OGG and OPUS files (via <u>LOOPSTART</u> and <u>LOOPEND</u> (or <u>LOOPLENGHT</u>) meta-tags). Backported into SDL_mixer since 12'th of Novenber, 2019.
* In the Modplug module enabled internal loops (tracker musics with internal loops are will be looped correctly). Fixed at SDL_mixer since 31'th of January.
* Added new API functions
  * Ability to redefine Timidity patches path. So, patches folders are can be stored in any place!
  * Added functions to retrieve some meta-tags: Title, Artist, Album, Copyright

### Features introduced at MixerX later removed because of unnecessarity
* Own re-sampling implementation which a workaround to glitches caused with inaccurate re-sampler implementation from SDL2. Recent versions of SDL2 now has much better resampler than was before.

## Requirements
* Fresh [SDL2 library](https://hg.libsdl.org/SDL/)
* Optionally, complete set of audio codecs from [this repository](https://github.com/WohlSoft/AudioCodecs) can be built in one run with a small effort.
  * OGG Vorbis or Tremor:
    * [libogg, libvorbis, libvorbisfile (or libvorbisidec)](https://www.xiph.org/downloads/)
  * OPUS
    * [liboups, libopusfle](http://opus-codec.org/downloads/)
  * [FLAC](https://www.xiph.org/flac/)
  * one of these MP3 libraries on your choice:
    * [libmpg123](https://www.mpg123.de/)
    * libmad (working build with using of CMake is [here](https://github.com/WohlSoft/AudioCodecs/tree/master/libmad), original automake build is too old broken and unmodified since 2004'th year)
  * Tracker music modules
    * [XMP](https://github.com/libxmp/libxmp)
    * [ModPlug](https://github.com/WohlSoft/AudioCodecs/tree/master/libmodplug)
    * [MikMod](https://github.com/WohlSoft/AudioCodecs/tree/master/libmikmod)
  * MIDI on your choice
    * modified [Timidity-SDL library](https://github.com/WohlSoft/AudioCodecs/tree/master/libtimidity-sdl) (in original SDL Mixer it's source code is an embedded part)
    * FluidSynth (supported both 1.x and 2.x, will be identified on a compile time, dynamically will not work)
    * [libADLMIDI](https://github.com/Wohlstand/libADLMIDI)
    * [libOPNMIDI](https://github.com/Wohlstand/libOPNMIDI)

* [CMake >= 2.8](https://cmake.org/download/) building system
* Compiler, compatible with C90 (C99 is required when building with OPUS library) or higher and C++98 (to build C++-written parts and libraries) or higher. Build tested and works on next compilers:
  * GCC 4.8 and higher
  * CLang 3.x and higher
  * MSVC 2013, 2015, 2017

# How to build

## With CMake
With this way all dependencies will be automatically downloaded and compiled
```bash
mkdir build
cd build
cmake -DCMAKE_INSTALL_PREFIX=/usr/local/ -DDOWNLOAD_AUDIO_CODECS_DEPENDENCY=ON -DDOWNLOAD_SDL2_DEPENDENCY=ON ..
make -j 4 #where 4 - set number of CPU cores you have
make install
```

# How to use library

## Include
The API is fully compatible with original SDL Mixer and can be used as a drop-in replacement of original SDL Mixer with exception of a different header name
```cpp
#include <SDL2/SDL_mixer_ext.h>
```

## Linking (Linux)

### Dynamically
```bash
gcc playmus.c -o playmus -I/usr/local/include -L/usr/local/lib -lSDL2_mixer_ext -lSDL2 -lstdc++
```

### Statically
To get it linked you must also link dependencies of SDL Mixer X library itself and also dependencies of SDL2 too
```
gcc playmus.c -o playmus -I/usr/local/include -L/usr/local/lib -Wl,-Bstatic -lSDL2_mixer_ext -lFLAC -lopusfile -lopus -lvorbisfile -lvorbis -logg -lmad -lid3tag -lmodplug -lADLMIDI -lOPNMIDI -ltimidity -lgme -lzlib -lSDL2 -Wl,-Bdynamic -lpthread -lm -ldl -static-libgcc -lstdc++
```

# Documentation
* [Full documentation](https://wohlsoft.github.io/SDL-Mixer-X/SDL_mixer_ext.html)
* [PGE-Wiki description page](http://wohlsoft.ru/pgewiki/SDL_Mixer_X)
