# - Try to find OGG Container
# Once done this will define
#  OGG_FOUND - System has OGG Container
#  OGG_INCLUDE_DIRS - The OGG Container include directories
#  OGG_LIBRARIES - The libraries needed to use OGG Container

find_path(OGG_INCLUDE_DIR "ogg.h" PATH_SUFFIXES ogg)

find_library(OGG_LIBRARY NAMES ogg)

if(OGG_INCLUDE_DIR AND OGG_LIBRARY)
    set(OGG_FOUND 1)
endif()

mark_as_advanced(OGG_INCLUDE_DIR OGG_LIBRARY)

set(OGG_LIBRARIES ${OGG_LIBRARY})
set(OGG_INCLUDE_DIRS ${OGG_INCLUDE_DIR})
