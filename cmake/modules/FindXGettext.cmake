# - Try to find xgettext
# Once done this will define
#
#  XGETTEXT_FOUND - system has Iconv
#  GETTEXT_XGETTEXT_EXECUTABLE: the full path to the xgettext tool.
#
# This program is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License
# as published by the Free Software Foundation; either version 2
# of the License, or (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 51 Franklin Street, Suite 500, Boston, MA  02110-1335, USA.

IF (GETTEXT_XGETTEXT_EXECUTABLE)
  # Already in cache, be silent
  SET (XGETTEXT_FIND_QUIETLY TRUE)
ENDIF (GETTEXT_XGETTEXT_EXECUTABLE)

FIND_PROGRAM(GETTEXT_XGETTEXT_EXECUTABLE xgettext)
IF (GETTEXT_XGETTEXT_EXECUTABLE)
  SET(XGETTEXT_FOUND TRUE)
  IF (NOT XGETTEXT_FIND_QUIETLY)
    MESSAGE(STATUS "Found xgettext: ${GETTEXT_XGETTEXT_EXECUTABLE}")
  ENDIF (NOT XGETTEXT_FIND_QUIETLY)
ELSE (GETTEXT_XGETTEXT_EXECUTABLE)
  SET(XGETTEXT_FOUND FALSE)
  IF (XGettext_REQUIRED)
    MESSAGE(FATAL_ERROR "xgettext not found")
  ENDIF (XGettext_REQUIRED)
ENDIF (GETTEXT_XGETTEXT_EXECUTABLE)

