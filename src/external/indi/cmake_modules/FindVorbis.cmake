# - Find vorbis
# Find the native vorbis includes and libraries
#
#  VORBIS_INCLUDE_DIR - where to find vorbis.h, etc.
#  VORBIS_LIBRARIES   - List of libraries when using vorbis(file).
#  VORBIS_FOUND       - True if vorbis found.

if(VORBIS_INCLUDE_DIR)
    # Already in cache, be silent
    set(VORBIS_FIND_QUIETLY TRUE)
endif(VORBIS_INCLUDE_DIR)

find_path(VORBIS_INCLUDE_DIR vorbis/vorbisfile.h)

find_library(OGG_LIBRARY NAMES ogg)
find_library(VORBIS_LIBRARY NAMES vorbis)
find_library(VORBISFILE_LIBRARY NAMES vorbisfile)

# Handle the QUIETLY and REQUIRED arguments and set VORBIS_FOUND to TRUE if
# all listed variables are TRUE.
include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(VORBIS DEFAULT_MSG
    VORBIS_INCLUDE_DIR OGG_LIBRARY VORBIS_LIBRARY VORBIS_LIBRARY)

if(VORBIS_FOUND)
  set(VORBIS_LIBRARIES ${VORBISFILE_LIBRARY} ${VORBIS_LIBRARY} ${OGG_LIBRARY})
else(VORBIS_FOUND)
  set(VORBIS_LIBRARIES)
endif(VORBIS_FOUND)

mark_as_advanced(VORBIS_INCLUDE_DIR)
mark_as_advanced(OGG_LIBRARY VORBIS_LIBRARY VORBISFILE_LIBRARY)
