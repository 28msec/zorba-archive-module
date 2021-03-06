/*
 * Copyright 2012 The FLWOR Foundation.
 * 
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 * 
 * http://www.apache.org/licenses/LICENSE-2.0
 * 
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <algorithm>
#include <cassert>
#include <cstdio>
#include <fstream>
#include <iostream>
#include <string>
#include <memory>
#include <sys/types.h>

#include <sys/timeb.h>
#ifdef UNIX
# include <sys/time.h>
#endif
#ifdef WIN32
# include "strptime.h"
#ifndef _WIN64
# include <Windows.h>
# include <MMSystem.h>
#endif
#endif

#include <zorba/diagnostic_list.h>
#include <zorba/empty_sequence.h>
#include <zorba/item_factory.h>
#include <zorba/singleton_item_sequence.h>
#include <zorba/user_exception.h>
#include <zorba/util/base64_util.h>
#include <zorba/util/base64_stream.h>
#include <zorba/util/transcode_stream.h>

#include "archive.h"
#include "archive_entry.h"
#include "archive_module.h"
#include "config.h"

#define ERROR_ENTRY_COUNT_MISMATCH "ENTRY-COUNT"
#define ERROR_INVALID_OPTIONS "INVALID-OPTIONS"
#define ERROR_INVALID_ENTRY_VALS "INVALID-ENTRY-VALS"
#define ERROR_INVALID_ENCODING "INVALID-ENCODING"
#define ERROR_CORRUPTED_ARCHIVE "CORRUPTED-ARCHIVE"
#define ERROR_DIFFERENT_COMPRESSIONS_NOT_SUPPORTED "DIFFERENT-COMPRESSIONS-NOT-SUPPORTED"

namespace zorba { namespace archive {

// Allocating global keys
zorba::Item ArchiveModule::globalFormatKey;
zorba::Item ArchiveModule::globalCompressionKey;
zorba::Item ArchiveModule::globalNameKey;
zorba::Item ArchiveModule::globalTypeKey;
zorba::Item ArchiveModule::globalSizeKey;
zorba::Item ArchiveModule::globalLastModifiedKey;
zorba::Item ArchiveModule::globalEncodingKey;

/*******************************************************************************
 ******************************************************************************/
  zorba::ExternalFunction*
    ArchiveModule::getExternalFunction(const zorba::String& localName)
  {
    FuncMap_t::iterator lIte = theFunctions.find(localName);

    ExternalFunction*& lFunc = theFunctions[localName];

    if (lIte == theFunctions.end())
    {
      if (localName == "create")
      {
        lFunc = new CreateFunction(this);
      }
      else if (localName == "entries")
      {
        lFunc = new EntriesFunction(this);
      }
      else if (localName == "extract-text")
      {
        lFunc = new ExtractTextFunction(this);
      }
      else if (localName == "extract-binary")
      {
        lFunc = new ExtractBinaryFunction(this);
      }
      else if (localName == "delete")
      {
        lFunc = new DeleteFunction(this);
      }
      else if (localName == "update")
      {
        lFunc = new UpdateFunction(this);
      }
      else if (localName == "options")
      {
        lFunc = new OptionsFunction(this);
      }
    }

    return lFunc;
  }

  void ArchiveModule::destroy()
  {
    delete this;
  }

  ArchiveModule::~ArchiveModule()
  {
    for (FuncMap_t::const_iterator lIter = theFunctions.begin();
      lIter != theFunctions.end(); ++lIter)
    {
      delete lIter->second;
    }
    theFunctions.clear();
  }

  zorba::Item
  ArchiveModule::createDateTimeItem(time_t& aTime)
  {
    // create a datetime item with UTC as timezone
    // this seems to be what mtime from libarchive returns
    struct ::tm gmtm;
#ifdef WIN32
    gmtime_s(&gmtm, &aTime);
#else
    gmtime_r(&aTime, &gmtm);
#endif

    // create a datetime item without timezone because
    // this is what the entry tells us (at least for zip)
    Item lModifiedItem = getItemFactory()->createDateTime(
        static_cast<short>(gmtm.tm_year + 1900),
        static_cast<short>(gmtm.tm_mon + 1),
        static_cast<short>(gmtm.tm_mday),
        static_cast<short>(gmtm.tm_hour),
        static_cast<short>(gmtm.tm_min),
        gmtm.tm_sec,
        0);
    return lModifiedItem;
  }

  void
  ArchiveModule::parseDateTimeItem(const zorba::Item& i, time_t& t)
  {
    const char* lTime = i.getStringValue().c_str();

    struct tm tm;
    memset(&tm, 0, sizeof(struct tm));

    char* lTmp = strptime(lTime, "%Y-%m-%dT%T", &tm);
    if (lTmp != lTime + 19)
    {
      std::ostringstream lMsg;
      lMsg << i.getStringValue()
        << ": invalid value for last-modified attribute ";
      ArchiveFunction::throwError(ERROR_INVALID_ENTRY_VALS, lMsg.str().c_str());
    }
  }

  zorba::Item
  ArchiveModule::getGlobalItems(enum ArchiveModule::GLOBAL_ITEMS g)
  {
      switch(g)
      {
      case FORMAT: return globalFormatKey;
      case COMPRESSION: return globalCompressionKey;
      case NAME: return globalNameKey;
      case TYPE: return globalTypeKey;
      case SIZE: return globalSizeKey;
      case LAST_MODIFIED: return globalLastModifiedKey;
      case ENCODING: return globalEncodingKey;
      // we should never touch the default clause but ...
      default: return globalFormatKey;
      }
  }

/*******************************************************************************
 ****************************** ArchiveFunction ********************************
 ******************************************************************************/
  ArchiveFunction::ArchiveFunction(const ArchiveModule* aModule)
    : theModule(aModule)
  {
  }

  ArchiveFunction::~ArchiveFunction()
  {
  }


  /******************
  ** Archive Entry **
  ******************/

  ArchiveFunction::ArchiveEntry::ArchiveEntry()
    : theEncoding("UTF-8"),
      theEntryType(regular)
  {
    // use current time as a default for each entry
#if defined (WIN32)
    struct _timeb timebuffer;
    _ftime_s( &timebuffer );
#else
    struct timeb timebuffer;
    ftime( &timebuffer );
#endif
    theLastModified = timebuffer.time;
  }

  void
  ArchiveFunction::ArchiveEntry::setValues(struct archive_entry* aEntry)
  {
    theEntryPath = archive_entry_pathname(aEntry);

    if (archive_entry_size_is_set(aEntry))
    {
      theSize = (int)archive_entry_size(aEntry);
    }

    if (archive_entry_mtime_is_set(aEntry))
    {
      theLastModified = archive_entry_mtime(aEntry);
    }
    //check if it is encoded

    switch(archive_entry_filetype(aEntry))
    {
      case AE_IFDIR: theEntryType = directory; break;
      default: theEntryType = regular; break;
    }
  }

  void
  ArchiveFunction::ArchiveEntry::setValues(zorba::Item& aEntry)
  {
    if (aEntry.isJSONItem())
    {
      Item lKey;

      Iterator_t lKeyIter = aEntry.getObjectKeys();
      lKeyIter->open();
      while (lKeyIter->next(lKey))
      {
        Item lKeyValue;
        lKeyValue = aEntry.getObjectValue(lKey.getStringValue());

        if(lKey.getStringValue() == ArchiveModule::getGlobalItems(ArchiveModule::NAME).getStringValue())
        {
          theEntryPath = lKeyValue.getStringValue();
        }
        else if(lKey.getStringValue() == ArchiveModule::getGlobalItems(ArchiveModule::TYPE).getStringValue())
        {
          String filetype = lKeyValue.getStringValue();
          if(filetype == "directory")
          {
            theEntryType = directory;
          }
        }
        else if (lKey.getStringValue() == ArchiveModule::getGlobalItems(ArchiveModule::LAST_MODIFIED).getStringValue())
        {
          ArchiveModule::parseDateTimeItem(lKeyValue, theLastModified);
        }
        else if (lKey.getStringValue() == ArchiveModule::getGlobalItems(ArchiveModule::ENCODING).getStringValue())
        {
          theEncoding = lKeyValue.getStringValue();
          std::transform(
              theEncoding.begin(), theEncoding.end(),
              theEncoding.begin(), ::toupper);
          if (!transcode::is_supported(theEncoding.c_str()))
          {
            std::ostringstream lMsg;
            lMsg << theEncoding << ": unsupported encoding";
              
            throwError(ERROR_INVALID_ENCODING, lMsg.str().c_str());
          }
        }
        else if (lKey.getStringValue() == ArchiveModule::getGlobalItems(ArchiveModule::COMPRESSION).getStringValue())
        {
          theCompression = lKeyValue.getStringValue();
          std::transform(
              theCompression.begin(),
              theCompression.end(),
              theCompression.begin(), ::toupper);
        }
        else if (lKey.getStringValue() == ArchiveModule::getGlobalItems(ArchiveModule::SIZE).getStringValue())
        {
            theSize = lKeyValue.getLongValue();
        }
      }
    } else
        theEntryPath = aEntry.getStringValue();
  }

  /********************
  ** Archive Options **
  ********************/

  ArchiveFunction::ArchiveOptions::ArchiveOptions()
    : theCompression("DEFLATE"),
      theFormat("ZIP"),
      theSkipExtraAttrs(false)
  {}

  void
  ArchiveFunction::ArchiveOptions::setValues(struct archive* aArchive)
  {
    theCompression = ArchiveFunction::compressionName(
        archive_compression(aArchive));
    theFormat = ArchiveFunction::formatName(archive_format(aArchive));
  }

  void
  ArchiveFunction::ArchiveOptions::setValues(Item& aOptions)
  {
    if(aOptions.isJSONItem())
    {
      Item lOptionKey;
      Iterator_t lKeyIter = aOptions.getObjectKeys();
      lKeyIter->open();

      while (lKeyIter->next(lOptionKey))
      {
        Item lOptionValue;
        lOptionValue = aOptions.getObjectValue(lOptionKey.getStringValue());

        if (lOptionKey.getStringValue() == ArchiveModule::getGlobalItems(ArchiveModule::COMPRESSION).getStringValue())
        {
          theCompression = lOptionValue.getStringValue().c_str();
          std::transform(
              theCompression.begin(),
              theCompression.end(),
              theCompression.begin(), ::toupper);
        }
        else if (lOptionKey.getStringValue() == ArchiveModule::getGlobalItems(ArchiveModule::FORMAT).getStringValue())
        {
          theFormat = lOptionValue.getStringValue().c_str();
          std::transform(
              theFormat.begin(),
              theFormat.end(),
              theFormat.begin(), ::toupper);
        }
        else if (lOptionKey.getStringValue() == "skip-extra-attributes")
        {
          theSkipExtraAttrs = lOptionValue.getStringValue() == "true" ? true : false;
        }
      }
      if (theFormat == "ZIP")
      {
        if (theCompression != "STORE" && theCompression != "DEFLATE" && theCompression != "NONE")
        {
          std::ostringstream lMsg;
          lMsg
            << theCompression
            << ": compression algorithm not supported for ZIP format (required: deflate, store)";
          throwError(ERROR_INVALID_OPTIONS, lMsg.str().c_str());
        }
      }
      if (theFormat == "TAR")
      {        if (theCompression != "GZIP"
#ifndef WIN32
            && theCompression != "BZIP2"
            && theCompression != "LZMA"
#endif
          )
        {
          std::ostringstream lMsg;
          lMsg
            << theCompression
            << ": compression algorithm not supported for TAR format (required: gzip"
#ifndef WIN32
            << ", bzip2, lzma"
#endif
            << ")";
          throwError(ERROR_INVALID_OPTIONS, lMsg.str().c_str());
        }
      }
    }
  }

  /************************
  ** Archive Compressor ***
  ************************/

  ArchiveFunction::ArchiveCompressor::ArchiveCompressor()
    : theArchive(0),
      theEntry(0),
      theStream(new std::stringstream())
  {
    theEntry = archive_entry_new();
  }

  ArchiveFunction::ArchiveCompressor::~ArchiveCompressor()
  {
    archive_entry_free(theEntry);
  }

  void
  ArchiveFunction::ArchiveCompressor::setOptions(const ArchiveOptions& aOptions)
  {
    theOptions = aOptions;

    int lFormatCode = formatCode(aOptions.getFormat().c_str());
    int lErr = archive_write_set_format(theArchive, lFormatCode);
    ArchiveFunction::checkForError(lErr, 0, theArchive);

    int lCompressionCode = compressionCode(aOptions.getCompression().c_str());
    setArchiveCompression(theArchive, lCompressionCode);

    if (aOptions.getSkipExtraAttrs())
    {
      // ignore result value because some libarchive versions
      // don't support this option
      archive_write_set_options(theArchive, "zip:skip-extras=true");
    }

    lErr = archive_write_open(
        theArchive, this, 0, ArchiveFunction::writeStream, 0);
    ArchiveFunction::checkForError(lErr, 0, theArchive);
  }

  bool
  ArchiveFunction::ArchiveCompressor::getStreamForString(
      const zorba::String& aEncoding,
      zorba::Item& aFile,
      std::istream*& aResStream,
      uint64_t& aResFileSize) const
  {
    // 1. always need to materialize if transcoding is necessary
    //    or stream is not seekable to compute resulting file size
    if (aFile.isStreamable() &&
        (!aFile.isSeekable() ||
         transcode::is_necessary(aEncoding.c_str())))
    {
      aResStream = &aFile.getStream();

      if (transcode::is_necessary(aEncoding.c_str()))
      {
        transcode::attach(*aResStream, aEncoding.c_str());
      }

      std::stringstream* lStream = new std::stringstream();

      char lBuf[ZORBA_ARCHIVE_MAX_READ_BUF];
      while (aResStream->good())
      {
        aResStream->read(lBuf, ZORBA_ARCHIVE_MAX_READ_BUF);
        lStream->write(lBuf, aResStream->gcount());
        aResFileSize += aResStream->gcount();
      }
      aResStream = lStream;
      return true; // delete after use
    }
    // 2. seekable and no transcoding is best cast
    //    => compute size by seeking and return stream as-is
    else if (aFile.isStreamable())
    {
      aResStream = &aFile.getStream();

      aResStream->seekg(0, std::ios::end);
      aResFileSize = aResStream->tellg();
      aResStream->seekg(0, std::ios::beg);
      return false;
    }
    // 3. non-streamable string
    else
    {
      std::stringstream* lStream = new std::stringstream();

      //    3.1 with transcoding
      if (transcode::is_necessary(aEncoding.c_str()))
      {
        transcode::stream<std::istringstream> lTranscoder(
            aEncoding.c_str(),
            aFile.getStringValue().c_str()
          );
        char lBuf[ZORBA_ARCHIVE_MAX_READ_BUF];
        while (lTranscoder.good())
        {
          lTranscoder.read(lBuf, ZORBA_ARCHIVE_MAX_READ_BUF);
          lStream->write(lBuf, lTranscoder.gcount());
          aResFileSize += lTranscoder.gcount();
        }
      }
      else // 3.2 without transcoding
      {
        zorba::String lString = aFile.getStringValue();
        aResFileSize = lString.length();
        lStream->write(lString.c_str(), aResFileSize);
      }
      aResStream = lStream;
      return true;
    }
  }

  bool
  ArchiveFunction::ArchiveCompressor::getStreamForBase64(
      zorba::Item& aFile,
      std::istream*& aResStream,
      uint64_t& aResFileSize) const
  {
    if (aFile.isStreamable() && aFile.isSeekable())
    {
      aResStream = &aFile.getStream();

      aResStream->seekg(0, std::ios::end);
      aResFileSize = aResStream->tellg();
      aResStream->seekg(0, std::ios::beg);

      if (aFile.isEncoded())
      {
        base64::attach(*aResStream);
        // compute the size of the stream after base64 decoding
        aResFileSize = ((aResFileSize / 4) + !!(aResFileSize % 4)) * 3;
      }
      return false;
    }
    else
    {
      std::stringstream* lStream = new std::stringstream();

      size_t lResFileSize;
      const char* lBinValue = aFile.getBase64BinaryValue(lResFileSize);

      if (aFile.isEncoded())
      {
        aResFileSize = zorba::base64::decode(lBinValue, lResFileSize, *lStream);
      }
      else
      {
        lStream->write(lBinValue, lResFileSize);
        aResFileSize = lResFileSize;
      }
      aResStream = lStream;
      return true;
    }
  }

  bool
  ArchiveFunction::ArchiveCompressor::getStream(
      const ArchiveEntry& aEntry,
      zorba::Item& aFile,
      std::istream*& aResStream,
      uint64_t& aResFileSize) const
  {
    aResFileSize = 0;

    switch (aFile.getTypeCode())
    {
      case store::XS_STRING:
      {
        const zorba::String& lEncoding = aEntry.getEncoding();

        return getStreamForString(lEncoding, aFile, aResStream, aResFileSize);
      }
      case store::XS_BASE64BINARY:
      {
        return getStreamForBase64(aFile, aResStream, aResFileSize);
      }
      default:
      {
        String errNS("http://www.w3.org/2005/xqt-errors");
        Item errQName = ArchiveModule::getItemFactory()->createQName(
            errNS, "FORG0006");
        std::ostringstream lMsg;
        lMsg << aFile.getType().getStringValue()
          << ": invalid content argument "
          << "(xs:string or xs:base64binary)";
        throw USER_EXCEPTION(errQName, lMsg.str());
      }
    }

  }

  void
  ArchiveFunction::ArchiveCompressor::open(
    const ArchiveOptions& aOptions)
  {
    theArchive = archive_write_new();

    if (!theArchive)
      ArchiveFunction::throwError(
        ERROR_CORRUPTED_ARCHIVE, "internal error (couldn't create archive)");

    setOptions(aOptions);
  }

  void
  ArchiveFunction::ArchiveCompressor::compress(
    const std::vector<ArchiveEntry>& aEntries,
    zorba::Iterator_t& aFiles)
  {  
    zorba::Item lFile;
    aFiles->open();

    for (size_t i = 0; i < aEntries.size(); ++i)
    {
      if(aEntries[i].getEntryType() == ArchiveEntry::regular)
      {
        if (!aFiles->next(lFile))
        {
          std::ostringstream lMsg;
          lMsg << "number of entries (" << aEntries.size()
            << ") doesn't match number of content arguments (" << i << ")";
          throwError(ERROR_ENTRY_COUNT_MISMATCH, lMsg.str().c_str());
        }
      }

      const ArchiveEntry& lEntry = aEntries[i];
      
      compress(lEntry, lFile);
    
    }

    if (aFiles->next(lFile))
    {
      std::ostringstream lMsg;
      lMsg << "number of entries (" << aEntries.size()
        << ") less than number of content arguments";
      throwError(ERROR_ENTRY_COUNT_MISMATCH, lMsg.str().c_str());
    }

    aFiles->close();
  }

  void ArchiveFunction::ArchiveCompressor::compress(const ArchiveEntry& aEntry, Item aFile)
  {
      std::istream* lStream;
      bool lDeleteStream;
      uint64_t lFileSize;

      archive_entry_set_pathname(theEntry, aEntry.getEntryPath().c_str());
      archive_entry_set_mtime(theEntry, aEntry.getLastModified(), 0);
      if(aEntry.getEntryType() == ArchiveEntry::regular){
        archive_entry_set_filetype(theEntry, AE_IFREG);
        lDeleteStream = getStream(
          aEntry, aFile, lStream, lFileSize);
        archive_entry_set_perm(theEntry, 0644);
      } else {
        archive_entry_set_filetype(theEntry, AE_IFDIR);
        lDeleteStream = false;
        lFileSize = 0;
        archive_entry_set_perm(theEntry, 0775);
      }
      archive_entry_set_size(theEntry, lFileSize);

      if (theOptions.getFormat() == "ZIP")
      {
        int lNextComp;
        std::string lNextCompString;
        if (aEntry.getCompression().length() > 0)
        {
          lNextCompString = aEntry.getCompression().c_str();
          lNextComp = compressionCode(lNextCompString);
#ifndef ZORBA_LIBARCHIVE_HAVE_SET_COMPRESSION
          std::ostringstream lMsg;
          lMsg << lNextCompString << ": setting different compression algorithms for each entry is not supported by the used version of libarchive";
          throwError(ERROR_DIFFERENT_COMPRESSIONS_NOT_SUPPORTED, lMsg.str().c_str());
#endif
        }
        else
        {
          lNextCompString = theOptions.getCompression();
          lNextComp = compressionCode(lNextCompString);
        }
        if (lNextComp < ZORBA_ARCHIVE_COMPRESSION_DEFLATE && lNextComp != ARCHIVE_COMPRESSION_NONE)
        {
          std::ostringstream lMsg;
          lMsg << lNextCompString << ": compression algorithm not supported for ZIP format (required: deflate, store)";
          throwError(ERROR_INVALID_OPTIONS, lMsg.str().c_str());
        }

#ifdef ZORBA_LIBARCHIVE_HAVE_SET_COMPRESSION
        setArchiveCompression(theArchive, lNextComp);
#endif
      }
      else
      {
        if (aEntry.getCompression().length() > 0)
        {
          std::ostringstream lMsg;
          lMsg << aEntry.getCompression() << ": compression attribute only allowed for zip format";
          throwError(ERROR_DIFFERENT_COMPRESSIONS_NOT_SUPPORTED, lMsg.str().c_str());
        }
      }

      archive_write_header(theArchive, theEntry);

      if(aEntry.getEntryType() == ArchiveEntry::regular)
      {
        char lBuf[ZORBA_ARCHIVE_MAX_READ_BUF];
        while (lStream->good())
        {
          lStream->read(lBuf, ZORBA_ARCHIVE_MAX_READ_BUF);
          archive_write_data(theArchive, lBuf, lStream->gcount());
        }
      }

      archive_entry_clear(theEntry);
      archive_write_finish_entry(theArchive);

      if (lDeleteStream)
      {
        delete lStream;
        lStream = 0;
      }
  }

  void
  ArchiveFunction::ArchiveCompressor::close()
  {
	  archive_write_close(theArchive);
	  archive_write_finish(theArchive);
  }

  std::stringstream*
  ArchiveFunction::ArchiveCompressor::getResultStream()
  {
    return theStream;
  }


  String 
  ArchiveFunction::getURI() const
  {
    return theModule->getURI();
  }

  void
  ArchiveFunction::throwError(
        const char* aLocalName,
        const char* aErrorMessage)
  {
    String errNS(ArchiveModule::getModuleURI());
    Item errQName = ArchiveModule::getItemFactory()->createQName(
        errNS, aLocalName);
    throw USER_EXCEPTION(errQName, aErrorMessage);
  }

  void
  ArchiveFunction::checkForError(
        int aErrNo,
        const char* aLocalName,
        struct archive *a)
  {
    if (aErrNo != ARCHIVE_OK)
    {
      if (!aLocalName)
      {
        throwError(ERROR_CORRUPTED_ARCHIVE, archive_error_string(a));
      }
      else
      {
        throwError(aLocalName, archive_error_string(a));
      }
    }
  }

  zorba::Item
  ArchiveFunction::getOneItem(const Arguments_t& aArgs, int aIndex)
  {
    Item lItem;
    Iterator_t args_iter = aArgs[aIndex]->getIterator();
    args_iter->open();
    args_iter->next(lItem);
    args_iter->close();

    return lItem;
  }

  std::string
  ArchiveFunction::formatName(int f)
  {
    // first 16 bit indicate the format family
    switch (f & ARCHIVE_FORMAT_BASE_MASK)
    {
      case ARCHIVE_FORMAT_TAR: return "TAR";
      case ARCHIVE_FORMAT_ZIP: return "ZIP";
      default: return "";
    }
  }

  std::string
  ArchiveFunction::compressionName(int c)
  {
    switch (c)
    {
      case ARCHIVE_COMPRESSION_NONE: return "NONE";
      case ZORBA_ARCHIVE_COMPRESSION_DEFLATE: return "DEFLATE";
      case ZORBA_ARCHIVE_COMPRESSION_STORE: return "STORE";
      case ARCHIVE_COMPRESSION_GZIP: return "GZIP";
      case ARCHIVE_COMPRESSION_BZIP2: return "BZIP2";
      case ARCHIVE_COMPRESSION_LZMA: return "LZMA";
      default: return "";
    }
  }

  int
  ArchiveFunction::formatCode(const std::string& f)
  {
    if (f == "TAR")
    {
      return ARCHIVE_FORMAT_TAR_USTAR;
    }
    else if (f == "ZIP")
    {
      return ARCHIVE_FORMAT_ZIP;
    }
    else
    {
      std::ostringstream lMsg;
      lMsg << f << ": archive format not supported";
      throwError(ERROR_INVALID_OPTIONS, lMsg.str().c_str());
    }
    return 0;
  }

  int
  ArchiveFunction::compressionCode(const std::string& c)
  {
    if (c == "NONE")
    {
      return ARCHIVE_COMPRESSION_NONE;
    }
    else if (c == "STORE")
    {
      return ZORBA_ARCHIVE_COMPRESSION_STORE;
    }
    else if (c == "DEFLATE")
    {
      return ZORBA_ARCHIVE_COMPRESSION_DEFLATE;
    }
    else if (c == "GZIP")
    {
      return ARCHIVE_COMPRESSION_GZIP;
    }
    else if (c == "BZIP2")
    {
      return ARCHIVE_COMPRESSION_BZIP2;
    }
    else if (c == "LZMA")
    {
      return ARCHIVE_COMPRESSION_LZMA;
    }
    else
    {
      std::ostringstream lMsg;
      lMsg << c << ": compression algorithm not supported";
      throwError(ERROR_INVALID_OPTIONS, lMsg.str().c_str());
    }
    return 0;
  }

  void
  ArchiveFunction::setArchiveCompression(struct archive* a, int c)
  {
    int lErr = 0;
    switch (c)
    {
#ifdef ZORBA_LIBARCHIVE_HAVE_SET_COMPRESSION
      case ZORBA_ARCHIVE_COMPRESSION_STORE:
        lErr = archive_write_zip_set_compression_store(a); break;
      case ZORBA_ARCHIVE_COMPRESSION_DEFLATE:
      case ARCHIVE_COMPRESSION_NONE:
        lErr = archive_write_zip_set_compression_deflate(a); break;
#else
      case ZORBA_ARCHIVE_COMPRESSION_STORE:
        archive_write_set_options(a, "zip:compression=store");
        break;
      case ZORBA_ARCHIVE_COMPRESSION_DEFLATE:
        archive_write_set_options(a, "zip:compression=deflate");
        break;
      case ARCHIVE_COMPRESSION_NONE:
        lErr = archive_write_set_compression_none(a); break;
#endif
      case ARCHIVE_COMPRESSION_GZIP:
        lErr = archive_write_set_compression_gzip(a); break;
      case ARCHIVE_COMPRESSION_BZIP2:
        lErr = archive_write_set_compression_bzip2(a); break;
      case ARCHIVE_COMPRESSION_LZMA:
        lErr = archive_write_set_compression_lzma(a); break;
      default: assert(false);
    }
    ArchiveFunction::checkForError(lErr, 0, a);
  }


  _ssize_t 
  ArchiveItemSequence::readStream(struct archive*, void *data, const void **buff)
  {
    ArchiveItemSequence::CallbackData* lData =
      reinterpret_cast<ArchiveItemSequence::CallbackData*>(data);

    if (lData->theEnd) return 0;

    std::istream* lStream = lData->theStream;

    // seek to where we left of
    if (lData->theSeekable) lStream->seekg(lData->thePos, std::ios::beg);

    lStream->read(lData->theBuffer, ZORBA_ARCHIVE_MAX_READ_BUF);
    *buff = lData->theBuffer;

    if (lStream->eof()) lData->theEnd = true;

    // remember the stream pos before leaving the function
    if (lData->theSeekable) lData->thePos = lStream->tellg();

    return lStream->gcount(); 
  }

  ArchiveItemSequence::ArchiveIterator::ArchiveIterator(zorba::Item& a)
    : theArchiveItem(a),
      theArchive(0),
      theFactory(Zorba::getInstance(0)->getItemFactory())
  {}

  void
  ArchiveItemSequence::ArchiveIterator::open()
  {
    // open archive and allow for all kinds of formats and compression algos
    theArchive = archive_read_new();

    if (!theArchive)
      ArchiveFunction::throwError(
          ERROR_CORRUPTED_ARCHIVE, "internal error (couldn't create archive)");

    int lErr = archive_read_support_compression_all(theArchive);
    ArchiveFunction::checkForError(lErr, 0, theArchive);

    archive_read_support_format_all(theArchive);
    ArchiveFunction::checkForError(lErr, 0, theArchive);

    if (theArchiveItem.isStreamable())
    {
      theData.theStream = &theArchiveItem.getStream();
      theData.theStream->clear();
      theData.theSeekable = theArchiveItem.isSeekable();
      theData.theEnd = false;
      theData.thePos = 0;

      if (theArchiveItem.isEncoded())
      {
        base64::attach(*theData.theStream);
      }

      lErr = archive_read_open(theArchive, &theData, NULL, ArchiveItemSequence::readStream, NULL);
      ArchiveFunction::checkForError(lErr, 0, theArchive);
    }
    else
    {
      size_t lLen = 0;
      char* lData = const_cast<char*>(
          theArchiveItem.getBase64BinaryValue(lLen));

      if (theArchiveItem.isEncoded())
      {
        base64::decode(lData, lLen, &theDecodedData);
        lLen = theDecodedData.size();
        lErr = archive_read_open_memory(theArchive,
            const_cast<char*>(theDecodedData.c_str()), lLen);
        ArchiveFunction::checkForError(lErr, 0, theArchive);
      }
      else
      {
        lErr = archive_read_open_memory(theArchive, lData, lLen);
        ArchiveFunction::checkForError(lErr, 0, theArchive);
      }

    }
  }

  void
  ArchiveItemSequence::ArchiveIterator::close()
  {
    int lErr = archive_read_finish(theArchive);
    ArchiveFunction::checkForError(lErr, 0, theArchive);
    theArchive = 0;
  }


  _ssize_t
  ArchiveFunction::writeStream(
      struct archive *,
      void *func,
      const void *buff,
      size_t n)
  {
    ArchiveFunction::ArchiveCompressor* lFunc =
      static_cast<ArchiveFunction::ArchiveCompressor*>(func);

    const char * lBuf = static_cast<const char *>(buff);
    lFunc->getResultStream()->write(lBuf, n);
  
    return n;
  }

/*******************************************************************************
 ******************************************************************************/
  zorba::ItemSequence_t
    CreateFunction::evaluate(
      const Arguments_t& aArgs,
      const zorba::StaticContext* aSctx,
      const zorba::DynamicContext* aDctx) const 
  {
    std::vector<ArchiveEntry> lEntries;

    {
      Iterator_t lEntriesIter = aArgs[0]->getIterator();

      zorba::Item lEntry;
      lEntriesIter->open();
      while (lEntriesIter->next(lEntry))
      {
        lEntries.resize(lEntries.size() + 1);
        lEntries.back().setValues(lEntry);
      }
      lEntriesIter->close();
    }

    ArchiveOptions lOptions;

    if (aArgs.size() == 3)
    {
      zorba::Item lOptionsItem = getOneItem(aArgs, 2);
      lOptions.setValues(lOptionsItem);
    }
    
    ArchiveCompressor lArchive;
      
    zorba::Iterator_t lFileIter = aArgs[1]->getIterator();
    lArchive.open(lOptions);
    lArchive.compress(lEntries, lFileIter);
    lArchive.close();

    zorba::Item lRes = theModule->getItemFactory()->
      createStreamableBase64Binary(
        *lArchive.getResultStream(),
        &(ArchiveFunction::ArchiveCompressor::releaseStream),
        true, // seekable
        false // not encoded
        );
    return ItemSequence_t(new SingletonItemSequence(lRes));
  }


/*******************************************************************************
 ******************************************************************************/
  zorba::ItemSequence_t
  EntriesFunction::evaluate(
    const Arguments_t& aArgs,
    const zorba::StaticContext* aSctx,
    const zorba::DynamicContext* aDctx) const 
  { 
    Item lArchive = getOneItem(aArgs, 0);
    
    return ItemSequence_t(new EntriesItemSequence(lArchive));
  }

  EntriesFunction::EntriesItemSequence::EntriesIterator::EntriesIterator(
      zorba::Item& aArchive)
    : ArchiveIterator(aArchive)
  {
  }

  bool
  EntriesFunction::EntriesItemSequence::EntriesIterator::next(zorba::Item& aRes)
  {
    struct archive_entry *lEntry;

    int lErr = archive_read_next_header(theArchive, &lEntry);
    
    if (lErr == ARCHIVE_EOF) return false;

    if (lErr != ARCHIVE_OK)
    {
      ArchiveFunction::checkForError(lErr, 0, theArchive);
    }

    std::vector<std::pair<zorba::Item, zorba::Item> > lObjectArray;
    std::pair<zorba::Item, zorba::Item> lElemPair;

    // create text content (i.e. path name)
    String lName = archive_entry_pathname(lEntry);
    lElemPair = std::make_pair<zorba::Item, zorba::Item>(ArchiveModule::getGlobalItems(ArchiveModule::NAME),
                                                         theFactory->createString(lName));
    lObjectArray.push_back(lElemPair);

    // create size attr if the value is set in the archive
    if (archive_entry_size_is_set(lEntry))
    {
      long long lSize = archive_entry_size(lEntry);
      lElemPair = std::make_pair<zorba::Item, zorba::Item>(ArchiveModule::getGlobalItems(ArchiveModule::SIZE),
                                                           theFactory->createInteger(lSize));
      lObjectArray.push_back(lElemPair);
    }

    // create last-modified attr if the value is set in the archive
    if (archive_entry_mtime_is_set(lEntry))
    {
      time_t lTime = archive_entry_mtime(lEntry);
      lElemPair = std::make_pair<zorba::Item, zorba::Item>(ArchiveModule::getGlobalItems(ArchiveModule::LAST_MODIFIED),
                                                           ArchiveModule::createDateTimeItem(lTime));
      lObjectArray.push_back(lElemPair);
    }

    std::string lEntryType;
    if(archive_entry_filetype(lEntry) == AE_IFDIR)
    {
      // this entry is a directory
      lEntryType = "directory";
    }
    else if(archive_entry_filetype(lEntry) == AE_IFREG)
    {
      lEntryType = "regular";
    }
    else
    {
      // error! type not supported!
      // for the time being don't do anything
    }

    lElemPair = std::make_pair<zorba::Item, zorba::Item>(ArchiveModule::getGlobalItems(ArchiveModule::TYPE),
                                                         theFactory->createString(lEntryType));
    lObjectArray.push_back(lElemPair);

    // skip to the next entry and raise an error if that fails
    lErr = archive_read_data_skip(theArchive);
    ArchiveFunction::checkForError(lErr, 0, theArchive);

    aRes = theFactory->createJSONObject(lObjectArray);

    return true;
  }

/*******************************************************************************
 ******************************************************************************/

 /*******************************************************************************
 *  This function is meant to replace all the look for specific headers that are
 *  or are not in a list (ArchiveEntrySet)
 ******************************************************************************/
  struct archive_entry*
    ExtractFunction::ExtractItemSequence::ExtractIterator::lookForHeader(
        bool aMatch,
        ArchiveOptions* aOptions)
  {
    struct archive_entry *lEntry = 0;

    while (true)
    {
      int lErr = archive_read_next_header(theArchive, &lEntry);
      
      if (lErr == ARCHIVE_EOF) return NULL;

      if (lErr != ARCHIVE_OK)
      {
        ArchiveFunction::checkForError(lErr, 0, theArchive);
      }

      if(aOptions)
        aOptions->setValues(theArchive);

      if (theReturnAll) break;

      String lName = archive_entry_pathname(lEntry);
      if(aMatch) {
        if (theEntryNames.find(lName.str()) != theEntryNames.end())
        {
          break;
        }
      } else {
        if (theEntryNames.find(lName.str()) == theEntryNames.end())
        {
          break;
        }
      }
    }

    return lEntry;
  }

  zorba::ItemSequence_t
  ExtractTextFunction::evaluate(
    const Arguments_t& aArgs,
    const zorba::StaticContext* aSctx,
    const zorba::DynamicContext* aDctx) const 
  { 
    Item lArchive = getOneItem(aArgs, 0);

    zorba::String lEncoding("UTF-8");
    if (aArgs.size() == 3)
    {
      zorba::Item lItem = getOneItem(aArgs, 2);
      lEncoding = lItem.getStringValue();
      if (!transcode::is_supported(lEncoding.c_str()))
      {
        std::ostringstream lMsg;
        lMsg << lEncoding << ": unsupported encoding";
          
        throwError(ERROR_INVALID_ENCODING, lMsg.str().c_str());
      }
    }
    
    // return all entries if no second arg is given
    bool lReturnAll = aArgs.size() == 1;

    std::auto_ptr<ExtractItemSequence> lSeq(
        new ExtractTextItemSequence(lArchive, lReturnAll, lEncoding));

    // get the names of all entries that should be retruned
    if (aArgs.size() > 1)
    {
      ExtractFunction::ExtractItemSequence::EntryNameSet& lSet
        = lSeq->getNameSet();

      zorba::Item lItem;
      Iterator_t lIter = aArgs[1]->getIterator();
      lIter->open();
      while (lIter->next(lItem))
      {
        lSet.insert(lItem.getStringValue().str());
      }

      lIter->close();
    }

    return ItemSequence_t(lSeq.release());
  }

  bool
  ExtractTextFunction::ExtractTextItemSequence::ExtractTextIterator::next(
      zorba::Item& aRes)
  {
    struct archive_entry *lEntry = lookForHeader(true);

    //NULL is EOF
    if (!lEntry)
      return false;

    String lResult;

    // reserve some space if we know the decompressed size
    if (archive_entry_size_is_set(lEntry))
    {
      long long lSize = archive_entry_size(lEntry);
      lResult.reserve(lSize);
    }

    char lBuf[ZORBA_ARCHIVE_MAX_READ_BUF];

    // read entire entry into a string
    while (true)
    {
      int s = archive_read_data(
          theArchive, &lBuf, ZORBA_ARCHIVE_MAX_READ_BUF);

      if (s == 0) break;

      if (s < 0)
        throwError(ERROR_CORRUPTED_ARCHIVE, archive_error_string(theArchive));

      lResult.append(lBuf, s);
    }

    if (transcode::is_necessary(theEncoding.c_str()))
    {
      zorba::String lTranscodedString;
      transcode::stream<std::istringstream> lTranscoder(
          theEncoding.c_str(),
          lResult.c_str()
        );
      char buf[1024];
      while (lTranscoder.good())
      {
        lTranscoder.read(buf, 1024);
        lTranscodedString.append(buf, lTranscoder.gcount());
      }
      aRes = theFactory->createString(lTranscodedString);
    }
    else
    {
      aRes = theFactory->createString(lResult);
    }

    return true;
  }

/*******************************************************************************
 ******************************************************************************/
  zorba::ItemSequence_t
    ExtractBinaryFunction::evaluate(
      const Arguments_t& aArgs,
      const zorba::StaticContext* aSctx,
      const zorba::DynamicContext* aDctx) const 
  {
    Item lArchive = getOneItem(aArgs, 0);

    // return all entries if no second arg is given
    bool lReturnAll = aArgs.size() == 1;

    std::auto_ptr<ExtractItemSequence> lSeq(
        new ExtractBinaryItemSequence(lArchive, lReturnAll));

    // get the names of all entries that should be retruned
    if (aArgs.size() > 1)
    {
      ExtractFunction::ExtractItemSequence::EntryNameSet& lSet
        = lSeq->getNameSet();

      zorba::Item lItem;
      Iterator_t lIter = aArgs[1]->getIterator();
      lIter->open();
      while (lIter->next(lItem))
      {
        lSet.insert(lItem.getStringValue().str());
      }

      lIter->close();
    }

    return ItemSequence_t(lSeq.release());
  }

  bool
  ExtractBinaryFunction::ExtractBinaryItemSequence::ExtractBinaryIterator::next(
      zorba::Item& aRes)
  {
    struct archive_entry *lEntry = lookForHeader(true);

    //NULL is EOF
    if (!lEntry)
      return false;
    
    std::vector<unsigned char> lResult;

    // reserve some space if we know the decompressed size
    if (archive_entry_size_is_set(lEntry))
    {
      long long lSize = archive_entry_size(lEntry);
      lResult.reserve(lSize);
    }

    std::vector<unsigned char> lBuf;
    lBuf.resize(ZORBA_ARCHIVE_MAX_READ_BUF);

    // read entire entry into a string
    while (true)
    {
      int s = archive_read_data(
          theArchive, &lBuf[0], ZORBA_ARCHIVE_MAX_READ_BUF);

      if (s == 0) break;

      if (s < 0)
        throwError(ERROR_CORRUPTED_ARCHIVE, archive_error_string(theArchive));

      lResult.insert(lResult.end(), lBuf.begin(), lBuf.begin() + s);
    }

    aRes = theFactory->createBase64Binary(reinterpret_cast<char const*>(&lResult[0]), lResult.size(), false);

    return true;
  }


/*******************************************************************************
 ******************************************************************************/
  zorba::ItemSequence_t
    OptionsFunction::evaluate(
      const Arguments_t& aArgs,
      const zorba::StaticContext* aSctx,
      const zorba::DynamicContext* aDctx) const 
  {
    Item lArchive = getOneItem(aArgs, 0);

    return ItemSequence_t(new OptionsItemSequence(lArchive));
  }

  OptionsFunction::OptionsItemSequence::OptionsIterator::OptionsIterator(Item &aArchive)
      :ArchiveIterator(aArchive)
  {
  }

  bool
  OptionsFunction::OptionsItemSequence::OptionsIterator::next(
      zorba::Item& aRes)
  {
    if (lExhausted) return false;

    lExhausted = true;

    struct archive_entry *lEntry;
    typedef std::pair<zorba::Item, zorba::Item> tArrElemt;
    tArrElemt lElemt;
    std::vector<tArrElemt> lJSONObject;

    // to get the format, we need to peek into the first header
    int lErr = archive_read_next_header(theArchive, &lEntry);

    if (lErr != ARCHIVE_OK && lErr != ARCHIVE_EOF)
    {
      ArchiveFunction::checkForError(lErr, 0, theArchive);
    }

    std::string lFormat =
      ArchiveFunction::formatName(archive_format(theArchive));
    std::string lCompression =
      ArchiveFunction::compressionName(archive_compression(theArchive));

    if (lFormat == "ZIP")
    {
      lCompression = "DEFLATE";
    }

    lElemt = std::make_pair<zorba::Item, zorba::Item>(ArchiveModule::getGlobalItems(ArchiveModule::FORMAT),
                                                      theFactory->createString(lFormat));
    lJSONObject.push_back(lElemt);

    lElemt = std::make_pair<zorba::Item, zorba::Item>(ArchiveModule::getGlobalItems(ArchiveModule::COMPRESSION),
                                                      theFactory->createString(lCompression));
    lJSONObject.push_back(lElemt);

    aRes = theFactory->createJSONObject(lJSONObject);

    return true;
  }

/*******************************************************************************************
 *******************************************************************************************/
  bool
  UpdateFunction::UpdateItemSequence::UpdateIterator::next(
    zorba::Item& aRes)
  {
    struct archive_entry *lEntry = lookForHeader(false, &theOptions);
    
    //NULL is EOF
    if (!lEntry)
      return false;

    //form an ArchiveEntry with the entry
    theEntry.setValues(lEntry);

    if(archive_entry_filetype(lEntry) == AE_IFREG){
      //read entry content
      std::vector<unsigned char> lResult;

      if (archive_entry_size_is_set(lEntry))
      {
        long long lSize = archive_entry_size(lEntry);
        lResult.reserve(lSize);
      }

      std::vector<unsigned char> lBuf;
      lBuf.resize(ZORBA_ARCHIVE_MAX_READ_BUF);

      //read entry into string
      while (true)
      {
        int s = archive_read_data(
          theArchive, &lBuf[0], ZORBA_ARCHIVE_MAX_READ_BUF);
     
        if (s == 0) break;

        if (s < 0)
          throwError(ERROR_CORRUPTED_ARCHIVE, archive_error_string(theArchive));

        lResult.insert(lResult.end(), lBuf.begin(), lBuf.begin() + s);
      }

      aRes = theFactory->createBase64Binary(reinterpret_cast<char const*>(&lResult[0]), lResult.size(), false);
    }

    return true;
  }

  zorba::ItemSequence_t
    UpdateFunction::evaluate(
      const Arguments_t& aArgs,
      const zorba::StaticContext* aSctx,
      const zorba::DynamicContext* aDctx) const 
  {
    //Base64 Binary of the Archive
    Item lArchive = getOneItem(aArgs, 0);

    //Initialize an Update Iterator with the Archive recived from the function
    std::auto_ptr<UpdateItemSequence> lSeq(
    new UpdateItemSequence(lArchive, false));

    std::vector<ArchiveEntry> lEntries;

    //prepare list of entries to be updated into the Archive
    {
      Iterator_t lEntriesIter = aArgs[1]->getIterator();

      zorba::Item lEntry;
      lEntriesIter->open();
      ExtractFunction::ExtractItemSequence::EntryNameSet& lNameSet 
          = lSeq->getNameSet();
      while (lEntriesIter->next(lEntry))
      {
        lEntries.resize(lEntries.size() + 1);
        lEntries.back().setValues(lEntry);
        lNameSet.insert(lEntries.back().getEntryPath().str());
      }
      lEntriesIter->close();
    } 

    //get the iterator of Files to include in the archive
    zorba::Iterator_t lFileIter = aArgs[2]->getIterator();

    //Prepare new archive, for compressing the Files form the original 
    //updated with the new Files specified
    ArchiveCompressor lResArchive;
    ArchiveOptions lOptions;

    Item lItem;
    Iterator_t lSeqIter = lSeq->getIterator();
    
    //read first header and file of the archive so we can get the options before creating 
    //the new archive.
    lSeqIter->open();
    lSeqIter->next(lItem);
    //set the options of the archive
    lOptions = lSeq->getOptions();
    //create new archive with the options read
    lResArchive.open(lOptions);
    if (!lItem.isNull())
    {
      do 
      {
        //add and compress the files of the old archive into the new archive.
        lResArchive.compress(lSeq->getEntry(), lItem);
      } while (lSeqIter->next(lItem));
    }
    lSeqIter->close();

    //add and compress the new file sspecified as a parameter for the function.
    lResArchive.compress(lEntries, lFileIter);
    lResArchive.close();

    Item lRes = theModule->getItemFactory()->
      createStreamableBase64Binary(
      *lResArchive.getResultStream(),
      &(ArchiveFunction::ArchiveCompressor::releaseStream),
      true, // seekable
      false // no encoded
      );
    return ItemSequence_t(new SingletonItemSequence(lRes));
  }

/*******************************************************************************************
 *******************************************************************************************/
  zorba::ItemSequence_t
    DeleteFunction::evaluate(
      const Arguments_t& aArgs,
      const zorba::StaticContext* aSctx,
      const zorba::DynamicContext* aDctx) const 
  {
    //Base64 Binary of the Archive
    Item lArchive = getOneItem(aArgs, 0);

    std::auto_ptr<DeleteItemSequence> lSeq(
      new DeleteItemSequence(lArchive));

    //set list of files to delete from the archive.
    zorba::Item lItem;
    Iterator_t lIter = aArgs[1]->getIterator();
    lIter->open();
    ExtractFunction::ExtractItemSequence::EntryNameSet& lNameSet =
      lSeq->getNameSet();
    while (lIter->next(lItem))
    {
      lNameSet.insert(lItem.getStringValue().str());
    }
    lIter->close();

    //prepare new archive
    ArchiveCompressor lResArchive;
    ArchiveOptions lOptions;

    Item lContent;
    Iterator_t lSeqIter = lSeq->getIterator();
    
    //read first header and file of the archive so we can get the options before creating 
    //the new archive.
    lSeqIter->open();
    lSeqIter->next(lContent);
    //set the options of the archive
    lOptions = lSeq->getOptions();
    //create new archive with the options read
    lResArchive.open(lOptions);
    if (!lContent.isNull())
    {
      do 
      {
        //add and compress the files of the old archive into the new archive.
        lResArchive.compress(lSeq->getEntry(), lContent);
      } while (lSeqIter->next(lContent));
    }
    lSeqIter->close();
    
    lResArchive.close();

    zorba::Item lRes = theModule->getItemFactory()->
      createStreamableBase64Binary(
        *lResArchive.getResultStream(),
        &(ArchiveFunction::ArchiveCompressor::releaseStream),
        true, // seekable
        false // not encoded
        );
    return ItemSequence_t(new SingletonItemSequence(lRes));
  }

  bool
  DeleteFunction::DeleteItemSequence::DeleteIterator::next(
      zorba::Item& aRes)
  {
    struct archive_entry *lEntry = lookForHeader(false, &theOptions);
    
    //NULL is EOF
    if (!lEntry)
      return false;
    
    //form an ArchiveEntry with the entry
    theEntry.setValues(lEntry);

    if(archive_entry_filetype(lEntry) == AE_IFREG){
    
      //read entry content
      std::vector<unsigned char> lResult;

      if (archive_entry_size_is_set(lEntry))
      {
        long long lSize = archive_entry_size(lEntry);
        lResult.reserve(lSize);
      }

      std::vector<unsigned char> lBuf;
      lBuf.resize(ZORBA_ARCHIVE_MAX_READ_BUF);

      //read entry into string
      while (true)
      {
        int s = archive_read_data(
          theArchive, &lBuf[0], ZORBA_ARCHIVE_MAX_READ_BUF);
     
        if (s == 0) break;

        if (s < 0)
          throwError(ERROR_CORRUPTED_ARCHIVE, archive_error_string(theArchive));

        lResult.insert(lResult.end(), lBuf.begin(), lBuf.begin() + s);
      }

      aRes = theFactory->createBase64Binary(reinterpret_cast<char const*>(&lResult[0]), lResult.size(), false);
    }
    // else? if the entry represents a directory what are we
    // going to return??
    // answer: nothing, the directory will have no contents at all

    return true;
  }

} /* namespace zorba */ } /* namespace archive*/

std::ostream& std::operator<<(
  std::ostream& out,
  const zorba::archive::ArchiveFunction::ArchiveEntry& e)
{
  out << "name " << e.getEntryPath() << "; encoding " << e.getEncoding()
    << "; last-modified " << e.getLastModified();
  return out;
}


#ifdef WIN32
#  define DLL_EXPORT __declspec(dllexport)
#else
#  define DLL_EXPORT __attribute__ ((visibility("default")))
#endif

extern "C" DLL_EXPORT zorba::ExternalModule* createModule() {
  return new zorba::archive::ArchiveModule();
}
