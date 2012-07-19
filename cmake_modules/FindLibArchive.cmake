# Copyright 2012 The FLWOR Foundation.
# 
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
# 
# http://www.apache.org/licenses/LICENSE-2.0
# 
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

IF (LIBARCHIVE_INCLUDE_DIR)
  SET (LIBARCHIVE_FIND_QUIETLY TRUE)
ENDIF (LIBARCHIVE_INCLUDE_DIR)

FIND_PATH (
  LIBARCHIVE_INCLUDE_DIR
  archive.h
  PATHS ${LIBACHIVE_INCLUDE_DIR} /usr/include/ /usr/local/include /opt/local/include )
MARK_AS_ADVANCED (LIBARCHIVE_INCLUDE_DIR)

FIND_LIBRARY (
  LIBARCHIVE_LIBRARY
  NAMES archive
  PATHS ${LIBACHIVE_LIBRARY_DIR} /usr/lib /usr/local/lib /opt/local/lib)
MARK_AS_ADVANCED (LIBARCHIVE_LIBRARY)

IF (LIBARCHIVE_INCLUDE_DIR AND LIBARCHIVE_LIBRARY)
  SET (LIBARCHIVE_FOUND 1)
  SET (LIBARCHIVE_LIBRARIES ${LIBARCHIVE_LIBRARY})
  SET (LIBARCHIVE_INCLUDE_DIRS ${LIBARCHIVE_INCLUDE_DIR})
  IF (NOT LIBARCHIVE_FIND_QUIETLY)
    MESSAGE (STATUS "Found libarchive library: " ${LIBARCHIVE_LIBRARY})
    MESSAGE (STATUS "Found libarchive include path : " ${LIBARCHIVE_INCLUDE_DIR})
  ENDIF (NOT LIBARCHIVE_FIND_QUIETLY)

  SET(CMAKE_REQUIRED_INCLUDES "${LIBARCHIVE_INCLUDE_DIR}")
  SET(CMAKE_REQUIRED_LIBRARIES "${LIBARCHIVE_LIBRARY}")
  INCLUDE(CheckFunctionExists)
  CHECK_FUNCTION_EXISTS(archive_write_zip_set_compression_deflate ZORBA_LIBARCHIVE_HAVE_SET_COMPRESSION)
ELSE (LIBARCHIVE_INCLUDE_DIR AND LIBARCHIVE_LIBRARY)
  SET (LIBARCHIVE_FOUND 0)
  SET (LIBARCHIVE_LIBRARIES)
  SET (LIBARCHIVE_INCLUDE_DIRS)
ENDIF (LIBARCHIVE_INCLUDE_DIR AND LIBARCHIVE_LIBRARY)  
