# - Try to find libModPlug
# Once done this will define
#  ModPlug_FOUND - System has libModPlug
#  ModPlug_INCLUDE_DIRS - The libModPlug include directories
#  ModPlug_LIBRARIES - The libraries needed to use libModPlug

find_path(ModPlug_INCLUDE_DIR "modplug.h"
          PATH_SUFFIXES libmodplug)
find_library(ModPlug_LIBRARY NAMES modplug)

if(ModPlug_INCLUDE_DIR AND ModPlug_LIBRARY)
    set(ModPlug_FOUND 1)
endif()

mark_as_advanced(ModPlug_INCLUDE_DIR ModPlug_LIBRARY)

set(ModPlug_LIBRARIES ${ModPlug_LIBRARY} )
set(ModPlug_INCLUDE_DIRS ${ModPlug_INCLUDE_DIR} )

