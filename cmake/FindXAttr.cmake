# This file is part of Strigi Desktop Search
#
# Copyright (C) 2007 Jos van den Oever <jos@vandenoever.info>
#
# This library is free software; you can redistribute it and/or
# modify it under the terms of the GNU Library General Public
# License as published by the Free Software Foundation; either
# version 2 of the License, or (at your option) any later version.
#
# This library is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# Library General Public License for more details.
#
# You should have received a copy of the GNU Library General Public License
# along with this library; see the file COPYING.LIB.  If not, write to
# the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
# Boston, MA 02110-1301, USA.

# - Try to find the xattr header
# Once done this will define 
# 
#  XATTR_FOUND - system has xattr.h 
#  XATTR_INCLUDE_DIR - the xattr include directory 
# 
 
FIND_PATH(XATTR_INCLUDE_DIR attr/xattr.h) 
 
IF(XATTR_INCLUDE_DIR) 
   SET(XATTR_FOUND TRUE) 
ENDIF(XATTR_INCLUDE_DIR) 
 
IF(XATTR_FOUND) 
  IF(NOT XAttr_FIND_QUIETLY) 
    MESSAGE(STATUS "Found xattr.h") 
  ENDIF(NOT XAttr_FIND_QUIETLY) 
ELSE(XATTR_FOUND) 
  IF(XAttr_FIND_REQUIRED) 
    MESSAGE(FATAL_ERROR "Could not find xattr.h") 
  ENDIF(XAttr_FIND_REQUIRED) 
ENDIF(XATTR_FOUND) 

