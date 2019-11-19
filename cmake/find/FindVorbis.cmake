# - Try to find OGG Vorbis
# Once done this will define
#  Vorbis_FOUND - System has OGG Vorbis
#  Vorbis_INCLUDE_DIRS - The OGG Vorbis include directories
#  Vorbis_LIBRARIES - The libraries needed to use OGG Vorbis

find_path(Vorbis_INCLUDE_DIR "codec.h" PATH_SUFFIXES vorbis)
find_path(Tremor_INCLUDE_DIR "ivorbisfile.h" PATH_SUFFIXES tremor)
find_path(Ogg_INCLUDE_DIR "ogg.h" PATH_SUFFIXES ogg)

find_library(Vorbis_LIBRARY NAMES vorbis)
find_library(VorbisFile_LIBRARY NAMES vorbisfile)
find_library(VorbisDec_LIBRARY NAMES vorbisdec)

if(Vorbis_INCLUDE_DIR AND Vorbis_LIBRARY)
    set(Vorbis_FOUND 1)
    if(APPLE)
        find_library(VorbisFile_DYNAMIC_LIBRARY NAMES "vorbisfile"  PATH_SUFFIXES ".dylib")
    elseif(WIN32)
        find_library(VorbisFile_DYNAMIC_LIBRARY NAMES "vorbisfile" PATH_SUFFIXES ".dll")
    else()
        find_library(VorbisFile_DYNAMIC_LIBRARY NAMES "vorbisfile" PATH_SUFFIXES ".so")
    endif()
endif()

if(Tremor_INCLUDE_DIR AND VorbisDec_LIBRARY)
    set(Tremor_FOUND 1)
    if(APPLE)
        find_library(VorbisDec_DYNAMIC_LIBRARY NAMES "vorbisdec"  PATH_SUFFIXES ".dylib")
    elseif(WIN32)
        find_library(VorbisDec_DYNAMIC_LIBRARY NAMES "vorbisdec" PATH_SUFFIXES ".dll")
    else()
        find_library(VorbisDec_DYNAMIC_LIBRARY NAMES "vorbisdec" PATH_SUFFIXES ".so")
    endif()
endif()

mark_as_advanced(Vorbis_INCLUDE_DIR Ogg_INCLUDE_DIR Vorbis_LIBRARY Tremor_INCLUDE_DIR VorbisDec_LIBRARY)

set(Tremor_LIBRARIES ${VorbisDec_LIBRARY})
set(Tremor_INCLUDE_DIRS ${Ogg_INCLUDE_DIR} ${Tremor_INCLUDE_DIR})

set(Vorbis_LIBRARIES ${VorbisFile_LIBRARY} ${Vorbis_LIBRARY})
set(Vorbis_INCLUDE_DIRS ${Ogg_INCLUDE_DIR} ${Vorbis_INCLUDE_DIR})

