#!/bin/bash
cp ../build/libSDL_mixer_ext.so .
g++ -O0 -g -o playwave -DTEST_MIX_DISTANCE -Wl,-rpath='$ORIGIN' -I../include/SDL_mixer_ext playwave.c -L. -L../build -lSDL2 -lSDL_mixer_ext
g++ -O0 -g -o playmus -Wl,-rpath='$ORIGIN' -I../include/SDL_mixer_ext playmus.c -L. -L../build -lSDL2 -lSDL_mixer_ext

