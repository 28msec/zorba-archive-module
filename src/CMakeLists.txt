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

IF (ZORBA_SUPPRESS_LIBARCHIVE)
  MESSAGE (STATUS "ZORBA_SUPRESS_LIBARCHIVE is true - not searching for LibArchive.")
ELSE (ZORBA_SUPRESS_LINARCHIVE)
  
  MESSAGE (STATUS "Looking for LibArchive")
  FIND_PACKAGE (LibArchive)
  
  IF (LIBARCHIVE_FOUND)
    MESSAGE (STATUS "Found LibArchive --" ${LIBARCHIVE_LIBRARIES})
    INCLUDE_DIRECTORIES (${LIBARCHIVE_INCLUDE_DIR})  

    DECLARE_ZORBA_MODULE (
      URI "http://www.expath.org/ns/archive"
      VERSION 1.0
      FILE "archive_module.xq"
      LINK_LIBRARIES "${LIBARCHIVE_LIBRARIES}")

    DECLARE_ZORBA_SCHEMA (
      URI "http://www.expath.org/ns/archive"
      FILE "archive.xsd")

  ELSE (LIBARCHIVE_FOUND)
    MESSAGE (STATUS "LibArchive library not found == if you want to use the archive functionality please set LIBARCHIVE_INCLUDE_DIR and LIBARCHIVE_LIBRARIES cmake parameters accordingly.")
  ENDIF (LIBARCHIVE_FOUND)
  
ENDIF (ZORBA_SUPPRESS_LIBARCHIVE)
