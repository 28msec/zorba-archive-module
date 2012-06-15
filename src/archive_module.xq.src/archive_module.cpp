#include <zorba/item_factory.h>
#include <zorba/singleton_item_sequence.h>
#include <zorba/diagnostic_list.h>
#include <zorba/empty_sequence.h>
#include <zorba/store_manager.h>
#include <zorba/user_exception.h>
#include <zorba/uri_resolvers.h>
#include <zorba/vector_item_sequence.h>
#include <zorba/serializer.h>
#include <zorba/xquery.h>
#include <stdio.h>
#include <string>
#include <cassert>

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

  String 
    ArchiveFunction::getURI() const
  {
    return theModule->getURI();
  }

  void 
    ArchiveFunction::processEntries(Item entries_node, ArchiveEntries& entries)
  {
    if (!entries_node.isNull())
    {
      Item entries_name;
      entries_node.getNodeName(entries_name);
      
      if (entries_name.getLocalName() == "entries")
      {
        Item attr;
        Iterator_t attr_iter = entries_node.getAttributes();
        attr_iter->open();
        while (attr_iter->next(attr))
        {
          Item attr_name;
          attr.getNodeName(attr_name);
          if (attr_name.getLocalName() == "compression-level")
          {
            //entries.setCompressionLevel(attr.getIntValue());
            entries.setCompressionLevel(atoi(attr.getStringValue().c_str()));
          }
          else if (attr_name.getLocalName() == "compressed-size")
          {
            //entries.setCompressedSize(attr.getIntValue());
            entries.setCompressionLevel(atoi(attr.getStringValue().c_str()));
          }
          else if (attr_name.getLocalName() == "size")
          {
            //entries.setUncompressedSize(attr.getIntValue());
            entries.setCompressionLevel(atoi(attr.getStringValue().c_str()));
          }
          else if (attr_name.getLocalName() == "encrypted")
          {
            //entries.setEncrypted(attr.getBooleanValue());
            attr.getStringValue() == "true" ? entries.setEncrypted(true) : entries.setEncrypted(false);
          }
          else if (attr_name.getLocalName() == "last-modified")
          {
          }
        }
        attr_iter->close();
      }

      Item child;
      Iterator_t children = entries_node.getChildren();
      children->open();

      while (children->next(child))
      {
        if (child.getNodeKind() != store::StoreConsts::elementNode)
          continue;

        Item child_name;
        child.getNodeName(child_name);
        std::string local_name = child_name.getLocalName().c_str();
        if (child_name.getLocalName() == "entry")
        {
          ArchiveEntry entry(child.getStringValue());

          Item attr;
          Iterator_t attr_iter = child.getAttributes();
          attr_iter->open();
          while (attr_iter->next(attr))
          {
            Item attr_name;
            attr.getNodeName(attr_name);
            if (attr_name.getLocalName() == "compression-level")
            {
              //entry.setCompressionLevel(attr.getIntValue());
              entries.setCompressionLevel(atoi(attr.getStringValue().c_str()));
            }
            else if (attr_name.getLocalName() == "delete")
            {
              //entry.setDeleteEntry(attr.getBooleanValue());
              attr.getStringValue() == "true" ? entries.setEncrypted(true) : entries.setEncrypted(false);
            }
            else if (attr_name.getLocalName() == "encoding")
            {
              entry.setEntryEncoding(attr.getStringValue());
            }
          }
          attr_iter->close();
          entries.insertEntry(entry);
        }
      }
        
    }
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

#ifdef WIN32
  long
#else
  ssize_t
#endif    
  ArchiveItemSequence::readStream(struct archive*, void *data, const void **buff)
  {
    ArchiveItemSequence::CallbackData* lData =
      reinterpret_cast<ArchiveItemSequence::CallbackData*>(data);

    std::istream* lStream = lData->theStream;

    lStream->read(lData->theBuffer, ZORBA_ARCHIVE_MAX_READ_BUF);
    *buff = lData->theBuffer;

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

/*******************************************************************************
 ******************************************************************************/
  zorba::ItemSequence_t
    CreateFunction::evaluate(
      const Arguments_t& aArgs,
      const zorba::StaticContext* aSctx,
      const zorba::DynamicContext* aDctx) const 
  {

    Item entries_item;
    Iterator_t arg0_iter = aArgs[0]->getIterator();
    arg0_iter->open();
    arg0_iter->next(entries_item);
    arg0_iter->close();

    ArchiveEntries entries;

    ArchiveFunction::processEntries(entries_item, entries);

    throwError("ImplementationError", "Function not yet Implemented");

    return ItemSequence_t(new EmptySequence());
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

/*******************************************************************************************
 *******************************************************************************************/
 
  zorba::ItemSequence_t
  ExtractTextFunction::evaluate(
    const Arguments_t& aArgs,
    const zorba::StaticContext* aSctx,
    const zorba::DynamicContext* aDctx) const 
  { 
    Item lArchive = getOneItem(aArgs, 0);
    
    return ItemSequence_t(new ExtractItemSequence(lArchive));
  }

  ExtractTextFunction::ExtractItemSequence::ExtractIterator::ExtractIterator(
      zorba::Item& aArchive)
    : ArchiveIterator(aArchive)
  {
  }

  bool
  ExtractTextFunction::ExtractItemSequence::ExtractIterator::next(zorba::Item& aRes)
  {
    struct archive_entry *lEntry;

    int lErr = archive_read_next_header(theArchive, &lEntry);
    
    if (lErr == ARCHIVE_EOF) return false;

    if (lErr != ARCHIVE_OK)
    {
      ArchiveFunction::checkForError(lErr, 0, theArchive);
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
      size_t s = archive_read_data(
          theArchive, &lBuf, ZORBA_ARCHIVE_MAX_READ_BUF);

      if (s == 0) break;

      lResult.append(lBuf, s);
    }

    aRes = theFactory->createString(lResult);

    return true;
  }

/*******************************************************************************************
 *******************************************************************************************/
 
  zorba::ItemSequence_t
    ExtractBinaryFunction::evaluate(
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
