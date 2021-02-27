# SDL Mixer X

An audio mixer library, a fork of [SDL_mixer](http://www.libsdl.org/projects/SDL_mixer/) library

# Description
SDL_Mixer is a sample multi-channel audio mixer library.
It supports any number of simultaneously playing channels of 16 bit stereo audio,
plus a single channel of music, mixed by the popular FLAC, OPUS, XMP, ModPlug,
MikMod MOD, Timidity MIDI, FluidSynth, Ogg Vorbis, FLAC, libMAD, and MPG123 MP3 libraries.

SDL Mixer X - is an extended fork made by Vitaly Novichkov "Wohlstand" in
13 January 2015. SDL_mixer is a quick and stable high-quality audio library,
however, it has own bunch of defects such as incorrect playback of some WAV files,
inability to choose MIDI backend in runtime, inability to customize Timidity patches path,
No support for cross-fade and parallel streams playing (has only one musical stream.
Using of very long Chunks is ineffective), etc. The goal of this fork is to resolve those
issues, providing more extended functionality than was originally,
and providing support for more audio formats.

* [More detailed information](docs/index.md)
* [Read original ReadMe](README.txt)


# License
The library does use many dependencies licensed differently, and the final license
depends on which libraries you'll use and how (statically or dynamically).

The license of the library itself is ZLib as the mainstream SDL_mixer.

## List of libraries
There are libraries that keeps MixerX library under ZLib:
(Optionally, you can automatically filter any zlib-incompatible libraries by using
the `-DSDL_MIXER_CLEAR_FOR_ZLIB_LICENSE=ON` flag):

### Static linking
(BSD, ZLib, and "Artistic" licenses are allows usage in closed-source projects)
* libFLAC
* libModPlug
* libOGG
* libOpenMPT
* libOpus
* libSDL2
* libTimidity-SDL
* libVorbis
* libZlib

### LGPL-license libraries
LGPL allows usage in closed-source projects when LGPL-licensed components are linked dynamically.
Note, once you link those libraries statically, the MixerX library gets LGPLv2.1+ license.
* libFluidLite
* libGME
* libMikMod
* libMPG123
* libXMP

### GPL-license libraries
There are libraries making the MixerX being licensed as GPLv2+ or GPLv3+ with no matter
how you will link them. You can avoid linking them by using of the `-DSDL_MIXER_CLEAR_FOR_LGPL_LICENSE=ON`
CMake flag to get LGPL license, or use the `-DSDL_MIXER_CLEAR_FOR_ZLIB_LICENSE=ON` flag
to get the ZLib license.
* libMAD
* libADLMIDI
* libOPNMIDI
