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

#include "zip.h"

namespace zorba { namespace zip {

/*******************************************************************************************
 *******************************************************************************************/

  zorba::ExternalFunction*
    ZipModule::getExternalFunction(const zorba::String& localName)
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
      else if (localName == "get-text")
      {
        lFunc = new GetTextFunction(this);
      }
      else if (localName == "get-binary")
      {
        lFunc = new GetBinaryFunction(this);
      }
      else if (localName == "add")
      {
        lFunc = new AddFunction(this);
      }
      else if (localName == "replace")
      {
        lFunc = new ReplaceFunction(this);
      }
      else if (localName == "delete")
      {
        lFunc = new DeleteFunction(this);
      }
    }

    return lFunc;
  }

  void ZipModule::destroy()
  {
    delete this;
  }

  ZipModule::~ZipModule()
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
  
  ZipFunction::ZipFunction(const ZipModule* aModule)
    : theModule(aModule)
  {
  }

  ZipFunction::~ZipFunction()
  {
  }

  String 
    ZipFunction::getURI() const
  {
    return theModule->getURI();
  }

  void 
    ZipFunction::processEntries(Item entries_node)
  {
    if (!entries_node.isNull())
    {
      Item child;
      Iterator_t children = entries_node.getChildren();
      children->open();

      while (children->next(child))
      {
        if (child.getNodeKind() != store::StoreConsts::elementNode)
          continue;

        Item child_name;
        child.getNodeName(child_name);

        if (child_name.getLocalName() == "entries")
        {

        }
        else if (child_name.getLocalName() == "entry")
        {

        }
      }
        
    }
  }

  void
    ZipFunction::throwError(const char *err_localname, const std::string aErrorMessage)
  {
    String errNS(ZIP_MODULE_NAMESPACE);
    String errName(err_localname);
    Item errQName = ZipModule::getItemFactory()->createQName(errNS, errName);
    String errDescription(aErrorMessage);
    throw USER_EXCEPTION(errQName, errDescription);
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

    ZipFunction::processEntries(entries_item);

    throwError("ImplementationError", "Function not yet Implemented");

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
    throwError("ImplementationError", "Function not yet Implemented");

    return ItemSequence_t(new EmptySequence());
  }

/*******************************************************************************************
 *******************************************************************************************/
 
  zorba::ItemSequence_t
    GetTextFunction::evaluate(
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
    GetBinaryFunction::evaluate(
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
    AddFunction::evaluate(
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
    ReplaceFunction::evaluate(
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

} /* namespace zorba */ } /* namespace zip*/