FIND_PACKAGE_WIN32 (NAME LibArchive FOUND_VAR LIBARCHIVE_FOUND SEARCH_NAMES libarchive)

IF (LIBARCHIVE_FOUND)
  FIND_PACKAGE_DLLS_WIN32 (${FOUND_LOCATION} archive.dll)
ENDIF (LIBARCHIVE_FOUND)  