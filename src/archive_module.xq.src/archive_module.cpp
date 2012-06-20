#include <zorba/item_factory.h>
#include <zorba/singleton_item_sequence.h>
#include <zorba/diagnostic_list.h>
#include <zorba/empty_sequence.h>
#include <zorba/user_exception.h>
#include <zorba/transcode_stream.h>
#include <stdio.h>
#include <string>
#include <cassert>

#include <iostream>
#include <fstream>
#include <sys/types.h>
#include "archive.h"
#include "archive_entry.h"

#ifdef WIN32
#  include <MMSystem.h>
#endif

#include "archive_module.h"

namespace zorba { namespace archive {


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
    long long lTimeShift = 0;
    struct ::tm gmtm;
#ifdef WIN32
    localtime_s(&gmtm, &aTime);
    if (gmtm.tm_isdst != 0)
      lTimeShift += 3600;
#else
    localtime_r(&aTime, &gmtm);
    lTimeShift = gmtm.tm_gmtoff;
#endif

    Item lModifiedItem = getItemFactory()->createDateTime(
        static_cast<short>(gmtm.tm_year + 1900),
        static_cast<short>(gmtm.tm_mon + 1),
        static_cast<short>(gmtm.tm_mday),
        static_cast<short>(gmtm.tm_hour),
        static_cast<short>(gmtm.tm_min),
        gmtm.tm_sec,
        static_cast<short>(lTimeShift/3600));
    return lModifiedItem;
  }

/*******************************************************************************
 ******************************************************************************/
  ArchiveFunction::ArchiveFunction(const ArchiveModule* aModule)
    : theModule(aModule)
  {
  }

  ArchiveFunction::~ArchiveFunction()
  {
  }

  ArchiveFunction::ArchiveEntries::ArchiveEntry::ArchiveEntry(zorba::Item aEntry)
  {
    if (!aEntry.isNull())
    {
      Item entry_name;
      aEntry.getNodeName(entry_name);

      theEntryPath = aEntry.getStringValue();

      if (entry_name.getLocalName()  == "entry")
      {
        Item attr;
        Iterator_t attr_iter = aEntry.getAttributes();
        attr_iter->open();
        while (attr_iter->next(attr))
        {
          Item attr_name;
          attr.getNodeName(attr_name);

          if (attr_name.getLocalName() == "last-modified")
          {
            //add a way to add the time value
          }
          else if (attr_name.getLocalName() == "size")
          {
            //TODO: Replace when schema is validation
            //theSize = attr_name.getIntValue();
            theSize = std::atoi(attr_name.getStringValue().c_str());

          }
          //TODO: add entry parameters if missing.
        }
      }
    }
  }

  ArchiveFunction::ArchiveEntries::ArchiveEntries(zorba::Iterator_t aEntriesIterator)
  {
    Item lEntry;
    aEntriesIterator->open();
    while(aEntriesIterator->next(lEntry))
    {
      addEntry(lEntry);
    }
    aEntriesIterator->close();
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
        throwError("ARCH9999", archive_error_string(a));
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
      case ARCHIVE_FORMAT_CPIO: return "CPIO";
      case ARCHIVE_FORMAT_SHAR: return "SHAR";
      case ARCHIVE_FORMAT_TAR: return "TAR";
      case ARCHIVE_FORMAT_ISO9660: return "ISO9660";
      case ARCHIVE_FORMAT_ZIP: return "ZIP";
      case ARCHIVE_FORMAT_EMPTY: return "EMPTY";
      case ARCHIVE_FORMAT_AR: return "AR";
      case ARCHIVE_FORMAT_MTREE: return "MTREE";
      case ARCHIVE_FORMAT_RAW: return "RAW";
      case ARCHIVE_FORMAT_XAR: return "XAR";
      default: return "";
    }
  }

  std::string
  ArchiveFunction::compressionName(int c)
  {
    switch (c)
    {
      case ARCHIVE_COMPRESSION_NONE: return "NONE";
      case ARCHIVE_COMPRESSION_GZIP: return "GZIP";
      case ARCHIVE_COMPRESSION_BZIP2: return "BZIP2";
      case ARCHIVE_COMPRESSION_COMPRESS: return "COMPRESS";
      case ARCHIVE_COMPRESSION_PROGRAM: return "PROGRAM";
      case ARCHIVE_COMPRESSION_LZMA: return "LZMA";
      case ARCHIVE_COMPRESSION_XZ: return "XZ";
      case ARCHIVE_COMPRESSION_UU: return "UU";
      case ARCHIVE_COMPRESSION_RPM: return "RPM";
      default: return "";
    }
  }

#ifdef WIN32
  long
#else
  ssize_t
#endif    
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
          "ARCH9999", "internal error (couldn't create archive)");

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

      // TODO do decoding of base64binary here if necessary
      assert(!theArchiveItem.isEncoded());

	    lErr = archive_read_open(
          theArchive, &theData, 0, ArchiveItemSequence::readStream, 0);

      ArchiveFunction::checkForError(lErr, 0, theArchive);
    }
    else
    {
      size_t lLen = 0;
      char* lData = const_cast<char*>(theArchiveItem.getBase64BinaryValue(lLen));
      // TODO do decoding of base64binary here if necessary
      assert(!theArchiveItem.isEncoded());

      lErr = archive_read_open_memory(theArchive, lData, lLen);
      ArchiveFunction::checkForError(lErr, 0, theArchive);
    }
  }

  void
  ArchiveItemSequence::ArchiveIterator::close()
  {
    int lErr = archive_read_finish(theArchive);
    ArchiveFunction::checkForError(lErr, 0, theArchive);
    theArchive = 0;
  }

#ifdef WIN32
  long
#else
  ssize_t
#endif
  ArchiveFunction::writeStream(struct archive *, void *func, const void *buff, size_t n)
  {
    CreateFunction::ArchiveCompressor* lFunc =
      reinterpret_cast<CreateFunction::ArchiveCompressor*>(func);

    const char * theBuff = 
      static_cast<const char *>(buff);
    lFunc->getStream()->write(theBuff, n);
  
    return n;
  }

/*******************************************************************************************
 *******************************************************************************************/

  CreateFunction::ArchiveCompressor::ArchiveCompressor(zorba::Iterator_t aEntries)
    : theArchive(0),
      theEntry(0)
  {
    //pass to constructor
    theArchiveEntries = new ArchiveEntries(aEntries);

    theArchive = archive_write_new();

    if (!theArchive)
      ArchiveFunction::throwError(
        "ARCH9999", "internal error (couldn't create archive)");
        
    theBuffer = new char[1024];
    theStream = new std::stringstream(std::stringstream::in | std::stringstream::out);

    archive_write_set_format_ustar(theArchive);
    archive_write_set_compression_bzip2(theArchive);
    archive_write_set_bytes_per_block(theArchive, 1024);
    
    archive_write_open(theArchive, this, 0, ArchiveFunction::writeStream, 0);
    theEntry = archive_entry_new();
  }

  void
    CreateFunction::ArchiveCompressor::close()
  {
    delete[] theBuffer;
    theBuffer = 0;
    archive_entry_clear(theEntry);
    archive_write_finish_entry(theArchive);
  }

  CreateFunction::ArchiveCompressor::~ArchiveCompressor()
  {
    archive_entry_free(theEntry);
	  archive_write_close(theArchive);
	  archive_write_free(theArchive);
  }

  bool
    CreateFunction::ArchiveCompressor::compress(zorba::Iterator_t aFiles)
  {
    Item lFile;
    aFiles->open();

    for (int pos = 0 ; aFiles->next(lFile) ; pos++)
    {
      archive_entry_set_pathname(theEntry, theArchiveEntries->getEntryPath(pos).c_str());
      //including the size of a file is obligatory
      //TODO: calculate the size of a file if it wasn't set as an entry parameter
      archive_entry_set_size(theEntry, theArchiveEntries->getEntrySize(pos));

      //specifies file as a regular file
      //TODO: modified to allow the creation of empty directories
      archive_entry_set_filetype(theEntry, AE_IFREG);

      //specifies the permits of a file
      archive_entry_set_perm(theEntry, 0644);

      archive_write_header(theArchive, theEntry);


      //int lErr = archive_write_data(theArchive, lFile.getStringValue().c_str(), theArchiveEntries->getEntrySize(pos));
      //ArchiveFunction::checkForError(lErr, 0, theArchive);
        
      theFileStream = &lFile.getStream();
      theFileStream->read(theBuffer, 1024);
      int len = theFileStream->gcount();
      while (len > 0)
      {
        int lErr = archive_write_data(theArchive, theBuffer, len);
        ArchiveFunction::checkForError(lErr, 0, theArchive);
        theFileStream->read(theBuffer, 1024);
        len = theFileStream->gcount();
      }
       
      archive_entry_clear(theEntry);
      archive_write_finish_entry(theArchive);
      
    }

    aFiles->close();
    //TODO: add error check
    return true;
  }


  std::stringstream*
    CreateFunction::ArchiveCompressor::getStream() const
  {
    return theStream;
  }

  zorba::ItemSequence_t
    CreateFunction::evaluate(
      const Arguments_t& aArgs,
      const zorba::StaticContext* aSctx,
      const zorba::DynamicContext* aDctx) const 
  {
    //get iterator for Entries
    Iterator_t entriesIter = aArgs[0]->getIterator();
    
    ArchiveCompressor lArchive(entriesIter);

    Iterator_t fileIter = aArgs[1]->getIterator();
    
    lArchive.compress(fileIter);


    //Testign stream into a file
    /*
    std::filebuf fb;
    fb.open ("C:\\Users\\juanza\\Desktop\\a.zip",std::ios::out);
    std::ostream os(&fb);
    os << lArchive.getStream();
    fb.close();
    */

    std::cout << lArchive.getStream();
    return ItemSequence_t(new EmptySequence());
  }


/*******************************************************************************************
 *******************************************************************************************/
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
    theUntypedQName = theFactory->createQName(
        "http://www.w3.org/2001/XMLSchema", "untyped");

    theEntryName = theFactory->createQName(
        ArchiveModule::getModuleURI(), "entry");

    theLastModifiedName = theFactory->createQName("", "last-modified");
    theUncompressedSizeName = theFactory->createQName("", "size");
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

    Item lNoParent;

    Item lType = theUntypedQName;

    // create entry element
    aRes = theFactory->createElementNode(
        lNoParent, theEntryName, lType, true, false, NsBindings());

    // create text content (i.e. path name)
    String lName = archive_entry_pathname(lEntry);
    Item lNameItem = theFactory->createString(lName);
    theFactory->assignElementTypedValue(aRes, lNameItem);

    // create size attr if the value is set in the archive
    if (archive_entry_size_is_set(lEntry))
    {
      long long lSize = archive_entry_size(lEntry);
      Item lSizeItem = theFactory->createInteger(lSize);
      lType = theUntypedQName;
      theFactory->createAttributeNode(
          aRes, theUncompressedSizeName, lType, lSizeItem);
    }

    // create last-modified attr if the value is set in the archive
    if (archive_entry_mtime_is_set(lEntry))
    {
      time_t lTime = archive_entry_mtime(lEntry);
      Item lModifiedItem = ArchiveModule::createDateTimeItem(lTime);

      lType = theUntypedQName;
      theFactory->createAttributeNode(
          aRes, theLastModifiedName, lType, lModifiedItem);
    }

    // skip to the next entry and raise an error if that fails
    lErr = archive_read_data_skip(theArchive);
    ArchiveFunction::checkForError(lErr, 0, theArchive);

    return true;
  }

/*******************************************************************************
 ******************************************************************************/
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
          
        throwError("ARCH0004", lMsg.str().c_str());
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
        lSet.insert(lItem.getStringValue());
      }

      lIter->close();
    }

    return ItemSequence_t(lSeq.release());
  }

  bool
  ExtractTextFunction::ExtractTextItemSequence::ExtractTextIterator::next(
      zorba::Item& aRes)
  {
    struct archive_entry *lEntry;

    while (true)
    {
      int lErr = archive_read_next_header(theArchive, &lEntry);
      
      if (lErr == ARCHIVE_EOF) return false;

      if (lErr != ARCHIVE_OK)
      {
        ArchiveFunction::checkForError(lErr, 0, theArchive);
      }

      if (theReturnAll) break;

      String lName = archive_entry_pathname(lEntry);
      if (theEntryNames.find(lName) != theEntryNames.end())
      {
        break;
      }
    }

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
        lSet.insert(lItem.getStringValue());
      }

      lIter->close();
    }

    return ItemSequence_t(lSeq.release());
  }

  bool
  ExtractBinaryFunction::ExtractBinaryItemSequence::ExtractBinaryIterator::next(
      zorba::Item& aRes)
  {
    struct archive_entry *lEntry;

    while (true)
    {
      int lErr = archive_read_next_header(theArchive, &lEntry);
      
      if (lErr == ARCHIVE_EOF) return false;

      if (lErr != ARCHIVE_OK)
      {
        ArchiveFunction::checkForError(lErr, 0, theArchive);
      }

      if (theReturnAll) break;

      String lName = archive_entry_pathname(lEntry);
      if (theEntryNames.find(lName) != theEntryNames.end())
      {
        break;
      }
    }

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

      lResult.insert(lResult.end(), lBuf.begin(), lBuf.begin() + s);
    }

    aRes = theFactory->createBase64Binary(&lResult[0], lResult.size());

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

  bool
  OptionsFunction::OptionsItemSequence::OptionsIterator::next(
      zorba::Item& aRes)
  {
    if (lExhausted) return false;

    lExhausted = true;

    struct archive_entry *lEntry;

    // to get the format, we need to peek into the first header
    int lErr = archive_read_next_header(theArchive, &lEntry);

    if (lErr != ARCHIVE_OK && lErr != ARCHIVE_EOF)
    {
      ArchiveFunction::checkForError(lErr, 0, theArchive);
    }

    std::string lFormat =
      ArchiveFunction::formatName(archive_format(theArchive));
    std::string lAlgorithm =
      ArchiveFunction::compressionName(archive_compression(theArchive));

    if (lFormat == "ZIP")
    {
      lAlgorithm = "DEFLATE";
    }

    zorba::Item lUntypedQName = theFactory->createQName(
        "http://www.w3.org/2001/XMLSchema", "untyped");
    zorba::Item lTmpQName = lUntypedQName;

    zorba::Item lOptionsQName = theFactory->createQName(
        ArchiveModule::getModuleURI(), "options");

    zorba::Item lFormatQName = theFactory->createQName(
        ArchiveModule::getModuleURI(), "format");

    zorba::Item lAlgorithmQName = theFactory->createQName(
        ArchiveModule::getModuleURI(), "algorithm");

    zorba::Item lValueQName = theFactory->createQName("", "value"); 

    zorba::Item lNoParent;
    aRes = theFactory->createElementNode(
        lNoParent, lOptionsQName, lTmpQName, true, false, NsBindings());

    lTmpQName = lUntypedQName;
    zorba::Item lFormatItem = theFactory->createElementNode(
        aRes, lFormatQName, lTmpQName, true, false, NsBindings());

    lTmpQName = lUntypedQName;
    zorba::Item lAlgorithmItem = theFactory->createElementNode(
        aRes, lAlgorithmQName, lTmpQName, true, false, NsBindings());

    zorba::Item lTmpItem = theFactory->createString(lFormat);

    lTmpQName = lUntypedQName;
    theFactory->createAttributeNode(
        lFormatItem, lValueQName, lTmpQName, lTmpItem);

    lTmpItem = theFactory->createString(lAlgorithm);

    lTmpQName = lUntypedQName;
    theFactory->createAttributeNode(
        lAlgorithmItem, lValueQName, lTmpQName, lTmpItem);

    return true;
  }

/*******************************************************************************************
 *******************************************************************************************/
 
  zorba::ItemSequence_t
    UpdateFunction::evaluate(
      const Arguments_t& aArgs,
      const zorba::StaticContext* aSctx,
      const zorba::DynamicContext* aDctx) const 
  {
    throwError("ImplementationError", "Function not yet Implemented");

    return ItemSequence_t(new EmptySequence());
  }

/*******************************************************************************************
 *******************************************************************************************/
  zorba::ItemSequence_t
    DeleteFunction::evaluate(
      const Arguments_t& aArgs,
      const zorba::StaticContext* aSctx,
      const zorba::DynamicContext* aDctx) const 
  {
    throwError("ImplementationError", "Function not yet Implemented");

    return ItemSequence_t(new EmptySequence());
  }

} /* namespace zorba */ } /* namespace archive*/

#ifdef WIN32
#  define DLL_EXPORT __declspec(dllexport)
#else
#  define DLL_EXPORT __attribute__ ((visibility("default")))
#endif

extern "C" DLL_EXPORT zorba::ExternalModule* createModule() {
  return new zorba::archive::ArchiveModule();
}
