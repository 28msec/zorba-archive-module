IF (ZORBA_SUPPRESS_LIBARCHIVE)
  MESSAGE(STATUS "ZORBA_SUPPRESS_LIBARCHIVE is true - not searching for libarchive")
ELSE(ZORBA_SUPPRESS_LIBARCHIVE)
  
  MESSAGE (STATUS "Looking for libarchive")
  FIND_PACKAGE (LibArchive)

  IF(LIBARCHIVE_FOUND)
    MESSAGE (STATUS "Found libarchive library -- " ${LIBARCHIVE_LIBRARIES})
    SET (ZIP_LINK_LIBRARIES ${LIBARCHIVE_LIBRARIES})
    
    INCLUDE_DIRECTORIES(${LIBARCHIVE_INCLUDE_DIR})
    DECLARE_ZORBA_MODULE (URI "http://www.expath.org/ns/zip2" VERSION 1.0 FILE "zip.xq" LINK_LIBRARIES "${ZIP_LINK_LIBRARIES}")
  
  ELSE (LIBARCHIVE_FOUND)
    MESSAGE(STATUS "libarchive Library Not Found. You need to install libarchive Library in order to compile the Zip Module.")
  ENDIF (LIBARCHIVE_FOUND)
  MESSAGE (STATUS "")
ENDIF (ZORBA_SUPPRESS_LIBARCHIVE)