# - Try to find OGG Vorbis
# Once done this will define
#  Vorbis_FOUND - System has OGG Vorbis
#  Vorbis_INCLUDE_DIRS - The OGG Vorbis include directories
#  Vorbis_LIBRARIES - The libraries needed to use OGG Vorbis

find_path(Vorbis_INCLUDE_DIR "codec.h" PATH_SUFFIXES vorbis)
find_path(Ogg_INCLUDE_DIR "ogg.h" PATH_SUFFIXES ogg)

find_library(Vorbis_LIBRARY NAMES vorbis)
find_library(VorbisFile_LIBRARY NAMES vorbisfile)

if(APPLE)
    find_library(VorbisDec_DYNAMIC_LIBRARY NAMES "vorbisfile"  PATH_SUFFIXES ".dylib")
elseif(WIN32)
    find_library(VorbisDec_DYNAMIC_LIBRARY NAMES "vorbisfile" PATH_SUFFIXES ".dll")
else()
    find_library(VorbisDec_DYNAMIC_LIBRARY NAMES "vorbisfile" PATH_SUFFIXES ".so")
endif()

if(Vorbis_INCLUDE_DIR AND Vorbis_LIBRARY)
    set(Vorbis_FOUND 1)
endif()

mark_as_advanced(Vorbis_INCLUDE_DIR Ogg_INCLUDE_DIR Vorbis_LIBRARY)

set(Vorbis_LIBRARIES ${VorbisFile_LIBRARY} ${Vorbis_LIBRARY})
set(Vorbis_INCLUDE_DIRS ${Ogg_INCLUDE_DIR} ${Vorbis_INCLUDE_DIR})
