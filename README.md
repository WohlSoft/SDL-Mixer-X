# SDL Mixer X

An audio mixer library, a fork of [SDL_mixer](http://www.libsdl.org/projects/SDL_mixer/) library

# Description
SDL_Mixer is a sample multi-channel audio mixer library.
It supports any number of simultaneously playing channels of 16 bit stereo audio,
plus a single channel of music, mixed by the popular FLAC, OPUS, XMP, ModPlug,
MikMod MOD, Timidity MIDI, FluidSynth, Ogg Vorbis, FLAC, libMAD, and MPG123 MP3 libraries.

SDL Mixer X - is an extended fork made by Vitaly Novichkov "Wohlstand" in
13 January 2015. SDL_mixer is a quick and stable high-quality audio library,
however, it has own bunch of deffects such as broken resampling, incorrect
playback of WAV files, inability to choose MIDI backend in runtime,
inability to customize Timidty patches path, No support for cross-fade
and parallel streams playing (has only one musical stream. Using of very
long Chunks is ineffectively), etc. The goal of this fork is resolving those
issues, providing more extended functionality than was originally,
and providing support for more supported audio formats.

* [More detailed information](docs/index.md)
* [Read original ReadMe](README.txt)

