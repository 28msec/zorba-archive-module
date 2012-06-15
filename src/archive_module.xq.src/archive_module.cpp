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

#include <sys/types.h>
#include "archive.h"
#include "archive_entry.h"

#ifdef WIN32
#  include <MMSystem.h>
#endif

#include "archive_module.h"

namespace zorba { namespace archive {


/*******************************************************************************************
 *******************************************************************************************/

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

/*******************************************************************************************
 *******************************************************************************************/
  
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
          else if (attr_name.getLocalName() == "uncompressed-size")
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
    ArchiveFunction::throwError(const char *err_localname, const std::string aErrorMessage)
  {
    String errNS(ARCHIVE_MODULE_NAMESPACE);
    String errName(err_localname);
    Item errQName = ArchiveModule::getItemFactory()->createQName(errNS, errName);
    String errDescription(aErrorMessage);
    throw USER_EXCEPTION(errQName, errDescription);
  }

  Item
    ArchiveFunction::ArchiveEntries::getTree()
  {
    ItemFactory* factory = ArchiveModule::getItemFactory();
    Item doc = factory->createDocumentNode(ARCHIVE_MODULE_NAMESPACE,"");

    Item nodeName = factory->createQName("", "entries");
    Item untypedNodeName = factory->createQName("http://www.w3.org/2001/XMLSchema", "untyped");

    Item root = factory->createElementNode(doc, nodeName, untypedNodeName, true, false, NsBindings());
    
    std::vector<ArchiveFunction::ArchiveEntry>::iterator lIter = entryList.begin();
    std::vector<ArchiveFunction::ArchiveEntry>::iterator lEnd = entryList.end();

    for (; lIter != lEnd; ++lIter)
    {
      Item lEntry = factory->createElementNode(root, factory->createQName("", "entry"), untypedNodeName, true, false, NsBindings());
        factory->createTextNode(lEntry, lIter->getPath());
        if (lIter->getCompressionLevel() > -1)
        {
          factory->createAttributeNode(lEntry, factory->createQName("", "compression-level"), untypedNodeName, factory->createInteger(lIter->getCompressionLevel()));
        }
        if (lIter->getUncompressedSize() > -1)
        {
          factory->createAttributeNode(lEntry, factory->createQName("", "uncompressed-size"), untypedNodeName, factory->createInteger(lIter->getUncompressedSize()));
        }
    }
    return root;
  }
  
/*******************************************************************************************
 *******************************************************************************************/
 
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

/*******************************************************************************************
 *******************************************************************************************/
#ifdef WIN32
  long
#else
  ssize_t
#endif    
  ArchiveFunction::readStream(struct archive *, void *func, const void **buff)
  {
    // TODO make this more general to work with arbitrary item sequences
    // create a struct containing the user data
    EntriesFunction::EntriesItemSequence::EntriesIterator* lFunc =
      reinterpret_cast<EntriesFunction::EntriesItemSequence::EntriesIterator*>(func);

    std::istream* lStream = lFunc->getStream();

    lStream->read(lFunc->getBuffer(), 1024);
    *buff = lFunc->getBuffer();
    return lStream->gcount();
  }

/*******************************************************************************************
 *******************************************************************************************/
  zorba::ItemSequence_t
  EntriesFunction::evaluate(
    const Arguments_t& aArgs,
    const zorba::StaticContext* aSctx,
    const zorba::DynamicContext* aDctx) const 
  { 
    // TODO factorize this code into a function in the base class
    Item lArchive;
    Iterator_t args_iter = aArgs[0]->getIterator();
    args_iter->open();
    args_iter->next(lArchive);
    args_iter->close();
    
    return ItemSequence_t(new EntriesItemSequence(lArchive));
  }

  EntriesFunction::EntriesItemSequence::EntriesIterator::EntriesIterator(
      zorba::Item& aArchive)
    : theArchiveItem(aArchive),
      theArchive(0),
      theStream(0),
      theBuffer(0)
  {
    theFactory = Zorba::getInstance(0)->getItemFactory();
    // TODO use module uri here
    theEntryName = theFactory->createQName(
        "http://www.expath.org/ns/archive", "entry");
    
    theUncompressedSizeName = theFactory->createQName("", "uncompressed-size");
  }

  void
  EntriesFunction::EntriesItemSequence::EntriesIterator::open()
  {
    theStream = &theArchiveItem.getStream();

    // TODO do decoding of base64binary here if necessary
    // TODO handle non-streaming base64Binary here

    // TODO define constant and allocate on stack
    theBuffer = new char[1024];

    theArchive = archive_read_new();
	  archive_read_support_compression_all(theArchive);
	  archive_read_support_format_all(theArchive);
	  archive_read_open(theArchive, this, 0, ArchiveFunction::readStream, 0);
  }

  bool
  EntriesFunction::EntriesItemSequence::EntriesIterator::next(zorba::Item& i)
  {
    struct archive_entry *lEntry;

    if (archive_read_next_header(theArchive, &lEntry) != ARCHIVE_OK)
    {
      return false;
    }

    String lName = archive_entry_pathname(lEntry);
    int lSize = archive_entry_size(lEntry);

    // TODO create element according to schema here
    Item lNoParent;
    Item lType = theFactory->createQName("http://www.w3.org/2001/XMLSchema", "untyped");
    i = theFactory->createElementNode(lNoParent, theEntryName, lType, true, false, NsBindings());

    theFactory->createTextNode(i, lName);

    Item lSizeItem = theFactory->createInteger(lSize);
    lType = theFactory->createQName("http://www.w3.org/2001/XMLSchema", "untyped");
    theFactory->createAttributeNode(i, theUncompressedSizeName, lType, lSizeItem);

    archive_read_data_skip(theArchive);

    return true;
  }

  void
  EntriesFunction::EntriesItemSequence::EntriesIterator::close()
  {
    delete[] theBuffer;
    theBuffer = 0;

    archive_read_finish(theArchive);
    theArchive = 0;
  }

  bool
  EntriesFunction::EntriesItemSequence::EntriesIterator::isOpen() const
  {
    return theArchive != 0;
  }

  std::istream*
  EntriesFunction::EntriesItemSequence::EntriesIterator::getStream() const
  {
    return theStream;
  }

/*******************************************************************************************
 *******************************************************************************************/
 
  zorba::ItemSequence_t
    ExtractTextFunction::evaluate(
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
