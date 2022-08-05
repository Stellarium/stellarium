# - Try to find Inno Setup compiler
# Once done this will define
#
#  ISS_COMPILER_FOUND - system has Inno Setup compiler
#  ISS_COMPILER_EXECUTABLE: the full path to the Inno Setup compiler.
#
# =====================================================================
# Copyright 2022 Alexander Wolf
#
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions
# are met:
#
# * Redistributions of source code must retain the above copyright
#   notice, this list of conditions and the following disclaimer.
#
# * Redistributions in binary form must reproduce the above copyright
#   notice, this list of conditions and the following disclaimer in the
#   documentation and/or other materials provided with the distribution.
#
# * Neither the names of Kitware, Inc., the Insight Software Consortium,
#   nor the names of their contributors may be used to endorse or promote
#   products derived from this software without specific prior written
#   permission.

# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
# "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
# LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
# A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
# HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
# SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
# LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
# DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
# THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
# OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
#

IF (ISS_COMPILER_EXECUTABLE)
  # Already in cache, be silent
  SET (ISS_COMPILER_FIND_QUIETLY TRUE)
ENDIF (ISS_COMPILER_EXECUTABLE)

FIND_PROGRAM(ISS_COMPILER_EXECUTABLE NAMES ISCC)
IF (ISS_COMPILER_EXECUTABLE)
  SET(ISS_COMPILER_FOUND TRUE)
  IF (NOT ISS_COMPILER_FIND_QUIETLY)
    MESSAGE(STATUS "Found Inno Setup (.iss) compiler: ${ISS_COMPILER_EXECUTABLE}")
  ENDIF (NOT ISS_COMPILER_FIND_QUIETLY)
ELSE (ISS_COMPILER_EXECUTABLE)
  SET(ISS_COMPILER_FOUND FALSE)
  IF (ISS_COMPILER_REQUIRED)
    MESSAGE(FATAL_ERROR "Inno Setup compiler not found")
  ENDIF (ISS_COMPILER_REQUIRED)
ENDIF (ISS_COMPILER_EXECUTABLE)

