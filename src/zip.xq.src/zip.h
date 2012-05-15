#ifndef _ORG_EXPATH_WWW_NS_ZIP2_H_
#define _ORG_EXPATH_WWW_NS_ZIP2_H_

#include <map>

#include <zorba/zorba.h>
#include <zorba/external_module.h>
#include <zorba/function.h>
#include <zorba/dynamic_context.h>

#define ZIP_MODULE_NAMESPACE "http://www.expath.org/ns/zip"

namespace zorba { namespace zip {

  class ZipModule : public ExternalModule {
    protected:
      class ltstr
      {
      public:
        bool operator()(const String& s1, const String& s2) const
        {
          return s1.compare(s2) < 0;
        }
      };

      typedef std::map<String, ExternalFunction*, ltstr> FuncMap_t;

      FuncMap_t theFunctions;

    public:

      virtual ~ZipModule();

      virtual zorba::String
        getURI() const { return ZIP_MODULE_NAMESPACE; }

      virtual zorba::ExternalFunction*
        getExternalFunction(const String& localName);

      virtual void destroy();

      static ItemFactory*
        getItemFactory()
      {
        return Zorba::getInstance(0)->getItemFactory();
      }

  };

  class ZipFunction : public ContextualExternalFunction
  {
    protected:
      const ZipModule* theModule;

      static void 
        processEntries(zorba::Item entries_node);

      static void
        throwError(const char*, const std::string);

    public:

      ZipFunction(const ZipModule* module);

      virtual ~ZipFunction();

      virtual String
        getURI() const;
  };

  class CreateFunction : public ZipFunction{
    public:
      CreateFunction(const ZipModule* aModule) : ZipFunction(aModule) {}

      virtual ~CreateFunction(){}

      virtual zorba::String
        getLocalName() const { return "create"; }

      virtual zorba::ItemSequence_t
        evaluate(const Arguments_t&,
                 const zorba::StaticContext*,
                 const zorba::DynamicContext*) const;
  };

  class EntriesFunction : public ZipFunction{
    public:
      EntriesFunction(const ZipModule* aModule) : ZipFunction(aModule) {}

      virtual ~EntriesFunction(){}

      virtual zorba::String
        getLocalName() const { return "entries"; }

      virtual zorba::ItemSequence_t
        evaluate(const Arguments_t&,
                 const zorba::StaticContext*,
                 const zorba::DynamicContext*) const;
  };

  class GetTextFunction : public ZipFunction{
    public:
      GetTextFunction(const ZipModule* aModule) : ZipFunction(aModule) {}

      virtual ~GetTextFunction(){}

      virtual zorba::String
        getLocalName() const { return "get-text"; }

      virtual zorba::ItemSequence_t
        evaluate(const Arguments_t&,
                 const zorba::StaticContext*,
                 const zorba::DynamicContext*) const;
  };

  class GetBinaryFunction : public ZipFunction{
    public:
      GetBinaryFunction(const ZipModule* aModule) : ZipFunction(aModule) {}

      virtual ~GetBinaryFunction(){}

      virtual zorba::String
        getLocalName() const { return "get-binary"; }

      virtual zorba::ItemSequence_t
        evaluate(const Arguments_t&,
                 const zorba::StaticContext*,
                 const zorba::DynamicContext*) const;
  };

  class UpdateFunction : public ZipFunction{
    public:
      UpdateFunction(const ZipModule* aModule) : ZipFunction(aModule) {}

      virtual ~UpdateFunction(){}

      virtual zorba::String
        getLocalName() const { return "update"; }

      virtual zorba::ItemSequence_t
        evaluate(const Arguments_t&,
                 const zorba::StaticContext*,
                 const zorba::DynamicContext*) const;
  };

  class AddFunction : public ZipFunction{
    public:
      AddFunction(const ZipModule* aModule) : ZipFunction(aModule) {}

      virtual ~AddFunction(){}

      virtual zorba::String
        getLocalName() const { return "add"; }

      virtual zorba::ItemSequence_t
        evaluate(const Arguments_t&,
                 const zorba::StaticContext*,
                 const zorba::DynamicContext*) const;
  };

  class ReplaceFunction : public ZipFunction{
    public:
      ReplaceFunction(const ZipModule* aModule) : ZipFunction(aModule) {}

      virtual ~ReplaceFunction(){}

      virtual zorba::String
        getLocalName() const { return "replace"; }

      virtual zorba::ItemSequence_t
        evaluate(const Arguments_t&,
                 const zorba::StaticContext*,
                 const zorba::DynamicContext*) const;
  };

  class DeleteFunction : public ZipFunction{
    public:
      DeleteFunction(const ZipModule* aModule) : ZipFunction(aModule) {}

      virtual ~DeleteFunction(){}

      virtual zorba::String
        getLocalName() const { return "delete"; }

      virtual zorba::ItemSequence_t
        evaluate(const Arguments_t&,
                 const zorba::StaticContext*,
                 const zorba::DynamicContext*) const;
  };

} /* namespace zip  */ } /* namespace zorba */

#endif // _ORG_EXPATH_WWW_NS_ZIP2_H_