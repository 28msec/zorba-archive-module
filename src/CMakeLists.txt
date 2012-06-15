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
