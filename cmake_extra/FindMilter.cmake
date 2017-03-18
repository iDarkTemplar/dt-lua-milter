# Locate Milter library
# This module defines
#  MILTER_FOUND, if false, do not try to link to Milter
#  MILTER_LIBRARIES
#  MILTER_INCLUDE_DIR, where to find libmilter/mfapi.h and libmilter/mfdef.h
#

#=============================================================================
# Copyright 2007-2009 Kitware, Inc.
# Copyright 2017-2017 i.Dark_Templar <darktemplar@dark-templar-archives.net>
#
# Distributed under the OSI-approved BSD License (the "License");
# see accompanying file Copyright.txt for details.
#
# This software is distributed WITHOUT ANY WARRANTY; without even the
# implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
# See the License for more information.
#=============================================================================
# (To distribute this file outside of CMake, substitute the full
#  License text for the above reference.)

FIND_PATH(MILTER_INCLUDE_DIR libmilter/mfapi.h
  HINTS
  $ENV{MILTER_DIR}
  PATH_SUFFIXES include
  PATHS
  ~/Library/Frameworks
  /Library/Frameworks
  /usr/local
  /usr
  /sw # Fink
  /opt/local # DarwinPorts
  /opt/csw # Blastwave
  /opt
)

FIND_LIBRARY(MILTER_LIBRARY 
  NAMES milter
  HINTS
  $ENV{MILTER_DIR}
  PATH_SUFFIXES lib64 lib
  PATHS
  ~/Library/Frameworks
  /Library/Frameworks
  /usr/local
  /usr
  /sw
  /opt/local
  /opt/csw
  /opt
)

IF(MILTER_LIBRARY)
  SET( MILTER_LIBRARIES "${MILTER_LIBRARY}" CACHE STRING "Milter Libraries")
ENDIF(MILTER_LIBRARY)

INCLUDE(FindPackageHandleStandardArgs)
# handle the QUIETLY and REQUIRED arguments and set LUA_FOUND to TRUE if 
# all listed variables are TRUE
FIND_PACKAGE_HANDLE_STANDARD_ARGS(MILTER
                                  REQUIRED_VARS MILTER_LIBRARIES MILTER_INCLUDE_DIR)

MARK_AS_ADVANCED(MILTER_INCLUDE_DIR MILTER_LIBRARIES MILTER_LIBRARY)

