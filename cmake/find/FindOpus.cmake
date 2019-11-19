# - Try to find Opus
# Once done this will define
#  Opus_FOUND - System has Opus
#  Opus_INCLUDE_DIRS - The Opus include directories
#  Opus_LIBRARIES - The libraries needed to use Opus

find_path(Opus_INCLUDE_DIR "opusfile.h" PATH_SUFFIXES opus)
find_path(Ogg_INCLUDE_DIR "ogg.h" PATH_SUFFIXES ogg)

find_library(Opus_LIBRARY NAMES opus)
find_library(OpusFile_LIBRARY NAMES opusfile)

if(Opus_INCLUDE_DIR AND Opus_LIBRARY)
    set(Opus_FOUND 1)
    if(APPLE)
        find_library(OpusFile_DYNAMIC_LIBRARY NAMES "opusfile"  PATH_SUFFIXES ".dylib")
    elseif(WIN32)
        find_library(OpusFile_DYNAMIC_LIBRARY NAMES "opusfile" PATH_SUFFIXES ".dll")
    else()
        find_library(OpusFile_DYNAMIC_LIBRARY NAMES "opusfile" PATH_SUFFIXES ".so")
    endif()
endif()

mark_as_advanced(Opus_INCLUDE_DIR Ogg_INCLUDE_DIR Opus_LIBRARY)

set(Opus_LIBRARIES ${OpusFile_LIBRARY} ${Opus_LIBRARY})
set(Opus_INCLUDE_DIRS ${Ogg_INCLUDE_DIR} ${Opus_INCLUDE_DIR})

