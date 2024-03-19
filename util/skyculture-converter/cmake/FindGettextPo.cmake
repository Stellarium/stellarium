# - Try to find the GETTEXT-PO libraries
# Once done this will define
#
#  GETTEXTPO_FOUND - system has GETTEXT-PO
#  GETTEXTPO_INCLUDE_DIR - the GETTEXT-PO include directory
#  GETTEXTPO_LIBRARIES - GETTEXT-PO library
#
# Copyright (c) 2012 Yichao Yu <yyc1992@gmail.com>
# Copyright (c) 2015 Weng Xuetian <wengxt@gmail.com>
#
# Redistribution and use is allowed according to the terms of the BSD license.
# For details see the accompanying COPYING-CMAKE-SCRIPTS file.

find_path(GETTEXTPO_INCLUDE_DIR NAMES gettext-po.h)
find_library(GETTEXTPO_LIBRARY NAMES gettextpo)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(GettextPo
    FOUND_VAR
        GETTEXTPO_FOUND
    REQUIRED_VARS
        GETTEXTPO_LIBRARY
        GETTEXTPO_INCLUDE_DIR
)

if(GETTEXTPO_FOUND AND NOT TARGET GettextPo::GettextPo)
    add_library(GettextPo::GettextPo UNKNOWN IMPORTED)
    set_target_properties(GettextPo::GettextPo PROPERTIES
        IMPORTED_LOCATION "${GETTEXTPO_LIBRARY}"
        INTERFACE_INCLUDE_DIRECTORIES "${GETTEXTPO_INCLUDE_DIR}"
    )
endif()

mark_as_advanced(GETTEXTPO_INCLUDE_DIR GETTEXTPO_LIBRARY)
