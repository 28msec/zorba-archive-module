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

FIND_PACKAGE_WIN32 (NAME LibArchive FOUND_VAR LIBARCHIVE_FOUND SEARCH_NAMES libarchive)

IF (LIBARCHIVE_FOUND)
  FIND_PACKAGE_DLLS_WIN32 (${FOUND_LOCATION} archive.dll)
  SET(CMAKE_REQUIRED_INCLUDES "${LIBARCHIVE_INCLUDE_DIR}")
  SET(CMAKE_REQUIRED_LIBRARIES "${LIBARCHIVE_LIBRARY}")
  INCLUDE(CheckFunctionExists)
  CHECK_FUNCTION_EXISTS(archive_write_zip_set_compression_deflate LIBARCHIVE_HAVE_SET_COMPRESSION)
ENDIF (LIBARCHIVE_FOUND)  
