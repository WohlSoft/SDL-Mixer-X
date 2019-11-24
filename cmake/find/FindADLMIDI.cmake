# - Try to find libADLMIDI
# Once done this will define
#  ADLMIDI_FOUND - System has libADLMIDI
#  ADLMIDI_INCLUDE_DIRS - The libADLMIDI include directories
#  ADLMIDI_LIBRARIES - The libraries needed to use libADLMIDI

find_path(ADLMIDI_INCLUDE_DIR "adlmidi.h")
find_library(ADLMIDI_LIBRARY NAMES ADLMIDI)

include(FindPackageHandleStandardArgs)
# handle the QUIETLY and REQUIRED arguments and set ADLMIDI_FOUND to TRUE
# if all listed variables are TRUE
find_package_handle_standard_args(ADLMIDI  DEFAULT_MSG
                                  ADLMIDI_LIBRARY ADLMIDI_INCLUDE_DIR)

mark_as_advanced(ADLMIDI_INCLUDE_DIR ADLMIDI_LIBRARY)

set(ADLMIDI_LIBRARIES ${ADLMIDI_LIBRARY})
set(ADLMIDI_INCLUDE_DIRS ${ADLMIDI_INCLUDE_DIR})

