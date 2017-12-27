TEMPLATE = lib
CONFIG -= app_bundle
CONFIG -= qt
static-sdlmixer:{
    CONFIG -= dll
    CONFIG += static
} else {
    CONFIG += dll
    CONFIG -= static
}

exists($$PWD/../../_common/lib_destdir.pri):{
    # As part of PGE Project
    include($$PWD/../../_common/lib_destdir.pri)
} else {
    DESTDIR=$$PWD/build/lib/
}
include($$PWD/cflags.pri)

static-sdlmixer:{
    TARGET = SDL_Mixer_Xstatic
} else {
    TARGET = SDL2_mixer_ext
    win32:enable-stdcalls:{ #Required for building a VB6-compatible version
        TARGET = SDL2MixerVB
    }
}

# Dependent audio codecs
include($$PWD/../AudioCodecs/audio_codec_includes.pri)

# Codecs
DEFAULT_CODECS += \
    WAV_MUSIC \
    MID_MUSIC \
    USE_TIMIDITY_MIDI \
    #USE_FLUIDSYNTH_MIDI \
    #USE_FLUIDLIGHT \
    USE_ADL_MIDI \
    USE_OPN2_MIDI \
    OGG_MUSIC \
    FLAC_MUSIC \
    #MP3_MUSIC \
    MP3_MAD_MUSIC \
    GME_MUSIC \
    #MOD_MUSIC \
    MODPLUG_MUSIC \
    #CMD_MUSIC

COPY=cp
win32:{
    DEFAULT_CODECS += USE_NATIVE_MIDI
    enable-stdcalls:{ #Useful for VB6 usage
        TARGET = SDL2MixerVB
        QMAKE_LFLAGS += -Wl,--add-stdcall-alias
        DEFINES += FORCE_STDCALLS
    }
    RC_FILE = version.rc
    COPY=copy
}

# Build properties
exists($$PWD/../../_common/build_props.pri):{
    # As part of PGE Project
    include($$PWD/../../_common/build_props.pri)
    DESTDIR = $$PWD/../_builds/$$TARGETOS/lib
    LIBS += -L$$PWD/../_builds/$$TARGETOS/lib
    INCLUDEPATH += $$PWD/../_builds/$$TARGETOS/include
    QMAKE_POST_LINK = \
        $$COPY $$shell_path($$PWD/include/SDL_mixer_ext/SDL_mixer_ext.h) \
        $$shell_path($$PWD/../_builds/$$TARGETOS/include/SDL2)

    SDL2MixerH.path =  $$PWD/../_builds/$$TARGETOS/include/SDL2
    SDL2MixerH.files += $$PWD/include/SDL_mixer_ext/SDL_mixer_ext.h
    INSTALLS += SDL2MixerH

    win32: {
        SDL2MixerSO.path = $$PWD/../_builds/win32/bin
        SDL2MixerSO.files += $$PWD/../_builds/win32/lib/*.dll
        INSTALLS += SDL2MixerSO
    }
} else {
    # As standalone library
    exists($$PWD/../AudioCodecs/build/lib/):{
        LIBS += -L$$PWD/../AudioCodecs/build/lib/
    }
    LIBS += -L$$PWD/build/lib/
    INCLUDEPATH += -L$$PWD/build/include/
    include($$PWD/_common/build_props.pri)
}


INCLUDEPATH += $$PWD/include/

linux-g++||unix:!macx:!android:{
    DEFINES += HAVE_INTTYPES_H HAVE_SETENV HAVE_SINF HAVE_FORK
    CONFIG -= dll
    CONFIG -= static
    CONFIG += static_and_shared
    CONFIG += unversioned_libname
    CONFIG += skip_target_version_ext
}
android:{
    DEFINES += HAVE_INTTYPES_H HAVE_SETENV HAVE_SINF
}
macx:{
    DEFINES += HAVE_INTTYPES_H HAVE_SETENV HAVE_SINF
    # Build as static library
    CONFIG -= dll
    CONFIG += static
    CONFIG += staticlib
}

android:{
    LIBS += -lvorbisfile -lvorbis -logg #-lvorbisidec
} else {
    win32:{
        DEFINES -= UNICODE _UNICODE
        enable-stdcalls: {
            LIBS += -static -l:libSDL2.a -static-libgcc -static-libstdc++ -static -lpthread -luuid
            include($$PWD/src/vb6_binding/vb6_binding.pri)
        } else {
            !win*-msvc*:{
                LIBS += -lSDL2main -lSDL2.dll
            } else {
                LIBS += -lSDL2main -lSDL2
            }
        }
        LIBS += -lwinmm -lole32 -limm32 -lversion -loleaut32 -luser32 -lgdi32
    } else {
        LIBS += -lSDL2
    }
}

!win*-msvc*:{
    LIBS += -lm
}


# Let user disable or enable component by adding extra CONFIG+= values
enable-flac:  DEFAULT_CODECS += FLAC_MUSIC
disable-flac: DEFAULT_CODECS -= FLAC_MUSIC

enable-midi:  DEFAULT_CODECS += MID_MUSIC
disable-midi: DEFAULT_CODECS -= MID_MUSIC

enable-midi-adlmidi:  DEFAULT_CODECS += USE_ADL_MIDI
disable-midi-adlmidi: DEFAULT_CODECS -= USE_ADL_MIDI

enable-midi-timidity:  DEFAULT_CODECS += USE_TIMIDITY_MIDI
disable-midi-timidity: DEFAULT_CODECS -= USE_TIMIDITY_MIDI

enable-midi-native:  DEFAULT_CODECS += USE_NATIVE_MIDI
disable-midi-native: DEFAULT_CODECS -= USE_NATIVE_MIDI

enable-midi-opnmidi:  DEFAULT_CODECS += USE_OPN2_MIDI
disable-midi-opnmidi: DEFAULT_CODECS -= USE_OPN2_MIDI

enable-midi-fluidsynth:  DEFAULT_CODECS += USE_FLUIDSYNTH_MIDI
disable-midi-fluidsynth: DEFAULT_CODECS -= USE_FLUIDSYNTH_MIDI

enable-mp3:  DEFAULT_CODECS += MP3_MAD_MUSIC
disable-mp3: DEFAULT_CODECS -= MP3_MAD_MUSIC

enable-mp3-smpeg:  DEFAULT_CODECS += MP3_MUSIC
disable-mp3-smpeg: DEFAULT_CODECS -= MP3_MUSIC

enable-ogg:  DEFAULT_CODECS += OGG_MUSIC
disable-ogg: DEFAULT_CODECS -= OGG_MUSIC

enable-gme:  DEFAULT_CODECS += GME_MUSIC
disable-gme: DEFAULT_CODECS -= GME_MUSIC

enable-mikmod:  DEFAULT_CODECS += MOD_MUSIC
disable-mikmod: DEFAULT_CODECS -= MOD_MUSIC

enable-modplug:  DEFAULT_CODECS += MODPLUG_MUSIC
disable-modplug: DEFAULT_CODECS -= MODPLUG_MUSIC

enable-cmd:  DEFAULT_CODECS += CMD_MUSIC
disable-cmd: DEFAULT_CODECS -= CMD_MUSIC

android: {
DEFINES += \
    HAVE_STRCASECMP \
    HAVE_STRNCASECMP \
    MID_MUSIC \
    USE_ADL_MIDI \
    GME_MUSIC \
    NO_OLDNAMES \
    SPC_MORE_ACCURACY
    #OGG_USE_TREMOR
    DEFAULT_CODECS -= FLAC_MUSIC #temopary with no FLAC, because I wasn't built it because compilation bug
}

DEFINES += \
    $$DEFAULT_CODECS \
    main=SDL_main \
    PIC \
    _REENTRANT \
    _USE_MATH_DEFINES \

include($$PWD/src/codecs/play_wave.pri)
contains(DEFINES, USE_ADL_MIDI):        include($$PWD/src/codecs/play_midi_adl.pri)
contains(DEFINES, USE_OPN2_MIDI):       include($$PWD/src/codecs/play_midi_opn.pri)
contains(DEFINES, USE_TIMIDITY_MIDI):   include($$PWD/src/codecs/play_midi_timidity.pri)
contains(DEFINES, USE_NATIVE_MIDI):     include($$PWD/src/codecs/play_midi_native.pri)
contains(DEFINES, USE_FLUIDSYNTH_MIDI): include($$PWD/src/codecs/play_midi_fluid.pri)
contains(DEFINES, OGG_MUSIC):           include($$PWD/src/codecs/play_ogg.pri)
contains(DEFINES, FLAC_MUSIC):          include($$PWD/src/codecs/play_flac.pri)
contains(DEFINES, MOD_MUSIC):           include($$PWD/src/codecs/play_mikmod.pri)
contains(DEFINES, MODPLUG_MUSIC):       include($$PWD/src/codecs/play_modplug.pri)
contains(DEFINES, MP3_MAD_MUSIC):       include($$PWD/src/codecs/play_mp3.pri)
contains(DEFINES, GME_MUSIC):           include($$PWD/src/codecs/play_gme.pri)
contains(DEFINES, CMD_MUSIC):           include($$PWD/src/codecs/play_cmdmusic.pri)

HEADERS += \
    include/SDL_mixer_ext/SDL_mixer_ext.h \
    include/SDL_mixer_ext/begin_code.h \
    include/SDL_mixer_ext/close_code.h \
    src/audio_codec.h \
    src/effects_internal.h \
    src/resample/my_resample.h \
    src/mixer.h \

SOURCES += \
    src/audio_codec.c \
    src/effect_position.c \
    src/effect_stereoreverse.c \
    src/effects_internal.c \
    src/mixer.c \
    src/music.c \
    src/resample/my_resample.c
