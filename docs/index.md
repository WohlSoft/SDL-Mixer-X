# SDL Mixer X
A fork of [SDL_mixer](http://www.libsdl.org/projects/SDL_mixer/) library

# Description
SDL_Mixer is a sample multi-channel audio mixer library.
It supports any number of simultaneously playing channels of 16 bit stereo audio, 
plus a single channel of music, mixed by the popular FLAC, MikMod MOD, ModPlug,
Timidity MIDI, FluidSynth, Ogg Vorbis, libMAD, and SMPEG MP3 libraries.

SDL Mixer X - is an extended fork made by Vitaly Novichkov "Wohlstand" in
13 January 2015. SDL_mixer is a quick and stable high-quality audio library,
however, it has own bunch of deffects such as broken resampling, incorrect
playback of WAV files, inability to choose MIDI backend in runtime,
inability to customize Timidty patches path, No support for cross-fade
and parallel streams playing (has only one musical stream. Using of very
long Chunks is ineffectively), etc. The goal of this fork is resolving those
issues, providing more extended functionality than was originally, 
and providing support for more supported audio formats.

## New features of SDL Mixer X in comparison to original SDL_mixer
* Added much more music formats (Such a game music emulators, XMI, MUS, and IMF playing via [ADLMIDI](https://github.com/Wohlstand/libADLMIDI) library)
* Added support of the loop points in the OGG files (via <u>LOOPSTART</u> and <u>LOOPEND</u> (or <u>LOOPLENGHT</u>) meta-tags)
* In the Modplug module enabled internal loops (tracker musics with internal loops are will be looped rightly)
* Better MIDI support:
  * Added ability to choose any of available MIDI backends in runtime
  * Added ability to append custom config path for Timidity synthesizer, then no more limit to default patches set
  * Forked version now has [https://github.com/Wohlstand/libADLMIDI ADLMIDI] midi sequences together with Native MIDI, Timidity and Fluidsynth. ADLMIDI is [[FM Synthesis|OPL-Synth]] Emulation based MIDI player.
  * Also the experimental [OPNMIDI](https://github.com/Wohlstand/libOPNMIDI) was added which an MIDI player through emulator of YM2612 chip which was widely used on Sega Megadrive/Genesis game console.
* Added new API functions
  * Ability to redefine Timidity patches path. So, patches folders are can be stored in any place!
  * Added functions to retrieve some meta-tags: Title, Artist, Album, Copyright
  * Added ADLMIDI Extra functions: Change bank ID, enable/disable high-level tremolo, enable/disable high-level vibrato, enable/disable scalable modulation
  * Added OPNMIDI Extra function: Set path to custom bank file (You can use [this editor](https://github.com/Wohlstand/OPN2BankEditor) to create or edit them)
* Own re-sampling implementation which a workaround to glitches caused with inaccurate re-sampler implementation from SDL2 (anyway, recent versions of SDL2 now has much better resampler than was before).
* Added support of [extra arguments](http://wohlsoft.ru/pgewiki/SDL_Mixer_X#Path_arguments) in the tail of the file path, passed into Mix_LoadMUS function.
* Added ability to build shared library in the <u>stdcall</u> mode with static linking of libSDL on Windows to use it as independent audio library with other languages like VB6 or .NET.
* QMake and CMake building systems in use

## Requirements
* Fresh [SDL2 library](https://hg.libsdl.org)
* Complete set of audio codecs from [this repository](https://github.com/WohlSoft/AudioCodecs)
* Building system on your taste:
  * QMake from Qt 5.x
  * CMake >= 2.8

# Documentation
* [Full documentation](SDL_mixer_ext.html)
* [PGE-Wiki description page](http://wohlsoft.ru/pgewiki/SDL_Mixer_X)
