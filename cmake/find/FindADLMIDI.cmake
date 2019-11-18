# - Try to find libADLMIDI
# Once done this will define
#  ADLMIDI_FOUND - System has libADLMIDI
#  ADLMIDI_INCLUDE_DIRS - The libADLMIDI include directories
#  ADLMIDI_LIBRARIES - The libraries needed to use libADLMIDI

find_path(ADLMIDI_INCLUDE_DIR "adlmidi.h")
find_library(ADLMIDI_LIBRARY NAMES ADLMIDI)

if(ADLMIDI_INCLUDE_DIR AND ADLMIDI_LIBRARY)
    set(ADLMIDI_FOUND 1)
endif()

mark_as_advanced(MPG123_INCLUDE_DIR ADLMIDI_LIBRARY)

set(ADLMIDI_LIBRARIES ${ADLMIDI_LIBRARY} )
set(ADLMIDI_INCLUDE_DIRS ${ADLMIDI_INCLUDE_DIR} )

