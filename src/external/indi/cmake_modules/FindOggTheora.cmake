#
# Find the native Ogg/Theora includes and libraries
#
# This module defines
# OGGTHEORA_INCLUDE_DIR, where to find ogg/ogg.h and theora/theora.h
# OGGTHEORA_LIBRARIES, the libraries to link against to use Ogg/Theora.
# OGGTHEORA_FOUND, If false, do not try to use Ogg/Theora.

FIND_PATH(OGGTHEORA_ogg_INCLUDE_DIR ogg/ogg.h)

FIND_PATH(OGGTHEORA_theora_INCLUDE_DIR theora/theora.h)

FIND_LIBRARY(OGGTHEORA_ogg_LIBRARY ogg)

FIND_LIBRARY(OGGTHEORA_theoraenc_LIBRARY theoraenc)

FIND_LIBRARY(OGGTHEORA_theoradec_LIBRARY theoradec)

SET(OGGTHEORA_INCLUDE_DIRS
  ${OGGTHEORA_ogg_INCLUDE_DIR}
  ${OGGTHEORA_theora_INCLUDE_DIR}
  )
#HACK multiple directories
SET(OGGTHEORA_INCLUDE_DIR ${OGGTHEORA_INCLUDE_DIRS})

SET(OGGTHEORA_LIBRARIES
  ${OGGTHEORA_theoraenc_LIBRARY}
  ${OGGTHEORA_theoradec_LIBRARY}
  ${OGGTHEORA_ogg_LIBRARY}
  )
#HACK multiple libraries
SET(OGGTHEORA_LIBRARY ${OGGTHEORA_LIBRARIES})

INCLUDE(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(OGGTHEORA
  "Could NOT find the ogg and theora libraries"
  OGGTHEORA_ogg_LIBRARY
  OGGTHEORA_theoraenc_LIBRARY
  OGGTHEORA_theoradec_LIBRARY
  OGGTHEORA_ogg_INCLUDE_DIR
  OGGTHEORA_theora_INCLUDE_DIR
  )

MARK_AS_ADVANCED(OGGTHEORA_ogg_INCLUDE_DIR OGGTHEORA_theora_INCLUDE_DIR
  OGGTHEORA_ogg_LIBRARY OGGTHEORA_theoraenc_LIBRARY
  OGGTHEORA_theoradec_LIBRARY
  )
