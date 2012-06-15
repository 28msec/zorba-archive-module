#ifndef _ORG_EXPATH_WWW_NS_ARCHIVE_H_
#define _ORG_EXPATH_WWW_NS_ARCHIVE_H_

#include <map>

#include <zorba/zorba.h>
#include <zorba/external_module.h>
#include <zorba/function.h>
#include <zorba/dynamic_context.h>

#define ARCHIVE_MODULE_NAMESPACE "http://www.expath.org/ns/archive"

#define MAX_BUF 65536
#define MAX_PATH_LENGTH 512

namespace zorba { namespace archive {

  class ArchiveModule : public ExternalModule {
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

      virtual ~ArchiveModule();

      virtual zorba::String
        getURI() const { return ARCHIVE_MODULE_NAMESPACE; }

      virtual zorba::ExternalFunction*
        getExternalFunction(const String& localName);

      virtual void destroy();

      static ItemFactory*
        getItemFactory()
      {
        return Zorba::getInstance(0)->getItemFactory();
      }

  };

  class ArchiveFunction : public ContextualExternalFunction
  {
    protected:
      class ArchiveEntry
      {
      private:
        int compressionLevel;
        int uncompressedSize;
        bool deleteEntry;
        String entryEncoding;
        String entryPath;

      public:
        ArchiveEntry(String entry)
        {
          entryPath = entry;
          deleteEntry = false;
        }
        ~ArchiveEntry(){}

        void setCompressionLevel(int level)
        {
          compressionLevel = level;
        }
        void setDeleteEntry(bool del)
        {
          deleteEntry = del;
        }
        void setEntryEncoding(String encoding)
        {
          entryEncoding = encoding;
        }
        void setUncompressedSize(int size)
        {
          uncompressedSize = size;
        }
        int getCompressionLevel()
        {
          return compressionLevel;
        }
        bool isDeleting()
        {
          return deleteEntry;
        }
        String getEntryEncoding()
        {
          return entryEncoding;
        }
        int getUncompressedSize()
        {
          return uncompressedSize;
        }
        String getPath()
        {
          return entryPath;
        }
      };

      class ArchiveEntries
      {
      private:
        int compressionLevel;
        int compressedSize;
        int uncompressedSize;
        bool encrypted;
        //lastmodified check time unix
        std::vector<ArchiveEntry> entryList;

      public:

        ArchiveEntries(){ encrypted = false; }
        ~ArchiveEntries(){}

        void setCompressionLevel(int level)
        {
          compressionLevel = level;
        }
        void setCompressedSize(int size)
        {
          compressedSize = size;
        }
        void setUncompressedSize(int size)
        {
          uncompressedSize = size;
        }
        void setEncrypted(bool encrypt)
        {
          encrypted = encrypt;
        }
        int getCompressionLevel()
        {
          return compressionLevel;
        }
        int getCompressedSize()
        {
          return compressedSize;
        }
        int getUncompressedSize()
        {
          return uncompressedSize;
        }
        bool isEncrypted()
        {
          return encrypted;
        }
        void insertEntry(ArchiveEntry aEntry)
        {
          entryList.push_back(aEntry);
        }
        Item getTree();

      };

      const ArchiveModule* theModule;

      static void 
        processEntries(zorba::Item entries_node, ArchiveEntries& entries);

      static void
        throwError(const char*, const std::string);

#ifdef WIN32
      static long
#else
      static ssize_t
#endif    
      readStream(struct archive *a, void *client_data, const void **buff);

      static int
      closeStream(struct archive *a, void *client_data);

    public:

      ArchiveFunction(const ArchiveModule* module);

      virtual ~ArchiveFunction();

      virtual String
        getURI() const;
  };

  class CreateFunction : public ArchiveFunction{
    public:
      CreateFunction(const ArchiveModule* aModule) : ArchiveFunction(aModule) {}

      virtual ~CreateFunction(){}

      virtual zorba::String
        getLocalName() const { return "create"; }

      virtual zorba::ItemSequence_t
        evaluate(const Arguments_t&,
                 const zorba::StaticContext*,
                 const zorba::DynamicContext*) const;
  };

  class EntriesFunction : public ArchiveFunction
  {
    public:
      class EntriesItemSequence : public ItemSequence
      {
        public:
          class EntriesIterator : public Iterator
          {
            protected:
              zorba::Item theArchiveItem;

              struct archive* theArchive;

              std::istream* theStream;

              char* theBuffer;

              zorba::Item theEntryName;
              zorba::Item theUncompressedSizeName;

              zorba::ItemFactory* theFactory;

            public:
              EntriesIterator(zorba::Item& aArchive);

              void
              open();

              bool
              next(zorba::Item& aItem);

              void
              close();

              bool
              isOpen() const;

              std::istream*
              getStream() const;

              char*
              getBuffer() const { return theBuffer; }
          };

        protected:
          zorba::Item theArchive;

        public:
          EntriesItemSequence(zorba::Item& aArchive)
            : theArchive(aArchive)
          {}

          zorba::Iterator_t
          getIterator() { return new EntriesIterator(theArchive); }
      };
      
    private:
      static void 
      list_archive(const char *archivepath, ArchiveEntries& entries);

    public:
      EntriesFunction(const ArchiveModule* aModule) : ArchiveFunction(aModule) {}

      virtual ~EntriesFunction(){}

      virtual zorba::String
      getLocalName() const { return "entries"; }

      virtual zorba::ItemSequence_t
      evaluate(const Arguments_t&,
               const zorba::StaticContext*,
               const zorba::DynamicContext*) const;
  };

  class ExtractTextFunction : public ArchiveFunction{
    public:
      ExtractTextFunction(const ArchiveModule* aModule) : ArchiveFunction(aModule) {}

      virtual ~ExtractTextFunction(){}

      virtual zorba::String
        getLocalName() const { return "extract-text"; }

      virtual zorba::ItemSequence_t
        evaluate(const Arguments_t&,
                 const zorba::StaticContext*,
                 const zorba::DynamicContext*) const;
  };

  class ExtractBinaryFunction : public ArchiveFunction{
    public:
      ExtractBinaryFunction(const ArchiveModule* aModule) : ArchiveFunction(aModule) {}

      virtual ~ExtractBinaryFunction(){}

      virtual zorba::String
        getLocalName() const { return "extract-binary"; }

      virtual zorba::ItemSequence_t
        evaluate(const Arguments_t&,
                 const zorba::StaticContext*,
                 const zorba::DynamicContext*) const;
  };

  class UpdateFunction : public ArchiveFunction{
    public:
      UpdateFunction(const ArchiveModule* aModule) : ArchiveFunction(aModule) {}

      virtual ~UpdateFunction(){}

      virtual zorba::String
        getLocalName() const { return "update"; }

      virtual zorba::ItemSequence_t
        evaluate(const Arguments_t&,
                 const zorba::StaticContext*,
                 const zorba::DynamicContext*) const;
  };
  class DeleteFunction : public ArchiveFunction{
    public:
      DeleteFunction(const ArchiveModule* aModule) : ArchiveFunction(aModule) {}

      virtual ~DeleteFunction(){}

      virtual zorba::String
        getLocalName() const { return "delete"; }

      virtual zorba::ItemSequence_t
        evaluate(const Arguments_t&,
                 const zorba::StaticContext*,
                 const zorba::DynamicContext*) const;
  };

} /* namespace archive  */ } /* namespace zorba */

#endif // _ORG_EXPATH_WWW_NS_ARCHIVE_H_
