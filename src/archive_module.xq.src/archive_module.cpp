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
  struct datastr
  {
    char name[MAX_PATH_LENGTH];
    void *buff;
    FILE *f;
  #ifdef WIN32
    long size;
  #else
    ssize_t size;
  #endif
  };

#ifdef WIN32
  long
#else
  ssize_t
#endif    
    readArchive(struct archive *a, void *client_data, const void **buff)
  {
    size_t bytes_read;
    struct datastr *data = (struct datastr *)client_data;
    *buff = data->buff;
    bytes_read = fread(data->buff, 1, data->size, data->f);
    return bytes_read;
  }

  int
    closeArchive(struct archive *a, void *client_data)
  {
    struct datastr* data = (struct datastr *)client_data;
    if (data->f != NULL)
    {
      fclose(data->f);
      data->f = NULL;
    }
    if (data->buff != NULL)
    {
      free(data->buff);
      data->buff = NULL;
    }

    return ARCHIVE_OK;
  }

  void
    EntriesFunction::list_archive(const char *archivepath, ArchiveEntries& entries)
  {
    struct datastr *data;
    struct archive *a;
    struct archive_entry *entry;

    int r;

    data = (datastr *)malloc(sizeof(datastr));
    data->size = MAX_BUF;
    data->buff = malloc(data->size);
    strcpy(data->name, archivepath);
    data->f = fopen(data->name, "rb");
    
    if (data->f == NULL)
      throwError("FileNotFound", "File was not found");

    a = archive_read_new();
	  archive_read_support_compression_all(a);
	  archive_read_support_format_all(a);
	  r = archive_read_open(a, data, NULL, readArchive, closeArchive);

    if (r != ARCHIVE_OK)
      throwError("FileError", "Error opening file");
   
    while (archive_read_next_header(a, &entry) == ARCHIVE_OK)
    {
      char * s = new char[MAX_PATH_LENGTH];
      sprintf(s, archive_entry_pathname(entry));
      ArchiveEntry zEntry(s);
      zEntry.setUncompressedSize(archive_entry_size(entry));
      archive_read_data_skip(a);
      entries.insertEntry(zEntry);
      delete s;
    }
    r = archive_read_finish(a);
    if (r != ARCHIVE_OK)
      throwError("FileError", "Error opening file");
    free(data);
  }

  zorba::ItemSequence_t
    EntriesFunction::evaluate(
      const Arguments_t& aArgs,
      const zorba::StaticContext* aSctx,
      const zorba::DynamicContext* aDctx) const 
  { 
    Item lItem;
    Iterator_t args_iter = aArgs[0]->getIterator();
    args_iter->open();
    args_iter->next(lItem);
    args_iter->close();

    ArchiveEntries entries;
    list_archive(lItem.getStringValue().c_str(), entries);

    Item lTree = entries.getTree();
    
    return ItemSequence_t(new SingletonItemSequence(lTree));
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
