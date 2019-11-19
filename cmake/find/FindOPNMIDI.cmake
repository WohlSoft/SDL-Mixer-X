# - Try to find libOPNMIDI
# Once done this will define
#  OPNMIDI_FOUND - System has libOPNMIDI
#  OPNMIDI_INCLUDE_DIRS - The libOPNMIDI include directories
#  OPNMIDI_LIBRARIES - The libraries needed to use libOPNMIDI

find_path(OPNMIDI_INCLUDE_DIR "opnmidi.h")
find_library(OPNMIDI_LIBRARY NAMES OPNMIDI)

if(OPNMIDI_INCLUDE_DIR AND OPNMIDI_LIBRARY)
    set(OPNMIDI_FOUND 1)
endif()

mark_as_advanced(OPNMIDI_INCLUDE_DIR OPNMIDI_LIBRARY)

set(OPNMIDI_LIBRARIES ${OPNMIDI_LIBRARY} )
set(OPNMIDI_INCLUDE_DIRS ${OPNMIDI_INCLUDE_DIR} )

