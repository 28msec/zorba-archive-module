
#include <zorba/user_exception.h>
#include <zorba/item_factory.h>
#include <zorba/singleton_item_sequence.h>

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
      if (localName == "entries")
      {
        lFunc = new EntriesFunction(this);
      }
      if (localName == "get-text")
      {
        lFunc = new GetTextFunction(this);
      }
      if (localName == "get-binary")
      {
        lFunc = new GetBinaryFunction(this);
      }
      if (localName == "add")
      {
        lFunc = new AddFunction(this);
      }
      if (localName == "replace")
      {
        lFunc = new ReplaceFunction(this);
      }
      if (localName == "delete")
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
    return ItemSequence_t(new SingletonItemSequence(
      ZipModule::getItemFactory()->createString("Function not implemented")));
  }

/*******************************************************************************************
 *******************************************************************************************/
 
  zorba::ItemSequence_t
    EntriesFunction::evaluate(
      const Arguments_t& aArgs,
      const zorba::StaticContext* aSctx,
      const zorba::DynamicContext* aDctx) const 
  {
    return ItemSequence_t(new SingletonItemSequence(
      ZipModule::getItemFactory()->createString("Function not implemented")));
  }

/*******************************************************************************************
 *******************************************************************************************/
 
  zorba::ItemSequence_t
    GetTextFunction::evaluate(
      const Arguments_t& aArgs,
      const zorba::StaticContext* aSctx,
      const zorba::DynamicContext* aDctx) const 
  {
    return ItemSequence_t(new SingletonItemSequence(
      ZipModule::getItemFactory()->createString("Function not implemented")));
  }

/*******************************************************************************************
 *******************************************************************************************/
 
  zorba::ItemSequence_t
    GetBinaryFunction::evaluate(
      const Arguments_t& aArgs,
      const zorba::StaticContext* aSctx,
      const zorba::DynamicContext* aDctx) const 
  {
    return ItemSequence_t(new SingletonItemSequence(
      ZipModule::getItemFactory()->createString("Function not implemented")));
  }

/*******************************************************************************************
 *******************************************************************************************/
 
  zorba::ItemSequence_t
    UpdateFunction::evaluate(
      const Arguments_t& aArgs,
      const zorba::StaticContext* aSctx,
      const zorba::DynamicContext* aDctx) const 
  {
    return ItemSequence_t(new SingletonItemSequence(
      ZipModule::getItemFactory()->createString("Function not implemented")));
  }

/*******************************************************************************************
 *******************************************************************************************/
 
  zorba::ItemSequence_t
    AddFunction::evaluate(
      const Arguments_t& aArgs,
      const zorba::StaticContext* aSctx,
      const zorba::DynamicContext* aDctx) const 
  {
    return ItemSequence_t(new SingletonItemSequence(
      ZipModule::getItemFactory()->createString("Function not implemented")));
  }

/*******************************************************************************************
 *******************************************************************************************/
 
  zorba::ItemSequence_t
    ReplaceFunction::evaluate(
      const Arguments_t& aArgs,
      const zorba::StaticContext* aSctx,
      const zorba::DynamicContext* aDctx) const 
  {
    return ItemSequence_t(new SingletonItemSequence(
      ZipModule::getItemFactory()->createString("Function not implemented")));
  }

/*******************************************************************************************
 *******************************************************************************************/
 
  zorba::ItemSequence_t
    DeleteFunction::evaluate(
      const Arguments_t& aArgs,
      const zorba::StaticContext* aSctx,
      const zorba::DynamicContext* aDctx) const 
  {
    return ItemSequence_t(new SingletonItemSequence(
      ZipModule::getItemFactory()->createString("Function not implemented")));
  }

} /* namespace zorba */ } /* namespace zip*/