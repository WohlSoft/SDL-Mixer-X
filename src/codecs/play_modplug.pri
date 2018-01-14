LIBS    += -lmodplug
DEFINES += MODPLUG_STATIC

DEFINES += MODPLUG_HEADER=\"<modplug/modplug.h>\"

HEADERS += \
    $$PWD/music_modplug.h

SOURCES += \
    $$PWD/music_modplug.c

