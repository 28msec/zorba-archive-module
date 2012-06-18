#ifndef _ORG_EXPATH_WWW_NS_ARCHIVE_H_
#define _ORG_EXPATH_WWW_NS_ARCHIVE_H_

#include <map>
#include <set>

#include <zorba/zorba.h>
#include <zorba/item_factory.h>
#include <zorba/external_module.h>
#include <zorba/function.h>

#define ZORBA_ARCHIVE_MAX_READ_BUF 2048

namespace zorba { namespace archive {

/*******************************************************************************
 ******************************************************************************/
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
        getURI() const { return getModuleURI(); }

      virtual zorba::ExternalFunction*
        getExternalFunction(const String& localName);

      virtual void destroy();

      static ItemFactory*
        getItemFactory()
      {
        return Zorba::getInstance(0)->getItemFactory();
      }

      static zorba::String
      getModuleURI() { return "http://www.expath.org/ns/archive"; }

      static zorba::Item
      createDateTimeItem(time_t&);

  };


/*******************************************************************************
 ******************************************************************************/
  class ArchiveItemSequence : public ItemSequence
  {
    public:
      struct CallbackData
      {
        std::istream* theStream;
        char          theBuffer[ZORBA_ARCHIVE_MAX_READ_BUF];
        bool          theSeekable;
        bool          theEnd;
        std::streampos thePos;

        CallbackData()
          : theStream(0), theSeekable(false), theEnd(false), thePos(0) {}
      };

    public:
      class ArchiveIterator : public Iterator
      {
        protected:
        protected:
          zorba::Item theArchiveItem;

          struct archive* theArchive;

          CallbackData    theData;

          zorba::ItemFactory* theFactory;

        public:
          ArchiveIterator(zorba::Item& aArchive);

          virtual ~ArchiveIterator() {}

          void
          open();

          bool
          next(zorba::Item& aItem) = 0;

          void
          close();

          bool
          isOpen() const { return theArchive != 0; }
      };

    protected:
      zorba::Item theArchive;

    public:
      ArchiveItemSequence(zorba::Item& aArchive)
        : theArchive(aArchive)
      {}

      virtual ~ArchiveItemSequence() {}

    protected:
#ifdef WIN32
      static long
#else
      static ssize_t
#endif    
      readStream(struct archive *a, void *client_data, const void **buff);

  };


/*******************************************************************************
 ******************************************************************************/
  class ArchiveFunction : public ContextualExternalFunction
  {
    protected:
      const ArchiveModule* theModule;

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

      static void 
        processEntries(zorba::Item entries_node, ArchiveEntries& entries);

    protected:
      static zorba::Item
      getOneItem(const Arguments_t& aArgs, int aIndex);

    public:

      ArchiveFunction(const ArchiveModule* module);

      virtual ~ArchiveFunction();

      virtual String
        getURI() const;

      static void
        throwError(const char*, const char*);

      static void
        checkForError(int aErrNo, const char* aLocalName, struct archive *a);

  };

/*******************************************************************************
 ******************************************************************************/
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

/*******************************************************************************
 ******************************************************************************/
  class EntriesFunction : public ArchiveFunction
  {
    protected:
      class EntriesItemSequence : public ArchiveItemSequence
      {
        public:
          class EntriesIterator : public ArchiveIterator
          {
            protected:
              zorba::Item theUntypedQName;
              zorba::Item theEntryName;
              zorba::Item theUncompressedSizeName;
              zorba::Item theLastModifiedName;

            public:
              EntriesIterator(zorba::Item& aArchive);

              virtual ~EntriesIterator() {}

              bool
              next(zorba::Item& aItem);
          };

        public:
          EntriesItemSequence(zorba::Item& aArchive)
            : ArchiveItemSequence(aArchive)
          {}

          virtual ~EntriesItemSequence() {}

          zorba::Iterator_t
          getIterator() { return new EntriesIterator(theArchive); }
      };
      
    public:
      EntriesFunction(const ArchiveModule* aModule)
        : ArchiveFunction(aModule) {}

      virtual ~EntriesFunction() {}

      virtual zorba::String
      getLocalName() const { return "entries"; }

      virtual zorba::ItemSequence_t
      evaluate(const Arguments_t&,
               const zorba::StaticContext*,
               const zorba::DynamicContext*) const;
  };

/*******************************************************************************
 ******************************************************************************/
  class ExtractTextFunction : public ArchiveFunction
  {
    protected:
      struct StringComparator
      {
        bool operator() (const zorba::String& s1, const zorba::String& s2)
        {
          return s1.compare(s2);
        }
      };

      typedef std::set<zorba::String, StringComparator> EntryNameSet;
      typedef EntryNameSet::const_iterator EntryNameSetIter;

    protected:
      class ExtractItemSequence : public ArchiveItemSequence
      {
        public:
          class ExtractIterator : public ArchiveIterator
          {
            public:
              ExtractIterator(
                  zorba::Item& aArchive,
                  zorba::String& aEncoding,
                  EntryNameSet& aEntryNames,
                  bool aReturnAll);

              virtual ~ExtractIterator() {}

              bool
              next(zorba::Item& aItem);

            protected:
              zorba::String& theEncoding;
              EntryNameSet& theEntryNames;
              bool theReturnAll;
          };

        public:
          ExtractItemSequence(
              zorba::Item& aArchive,
              zorba::String& aEncoding,
              bool aReturnAll)
            : ArchiveItemSequence(aArchive),
              theEncoding(aEncoding),
              theReturnAll(aReturnAll)
          {}

          virtual ~ExtractItemSequence() {}

          zorba::Iterator_t
          getIterator()
          {
            return new ExtractIterator(
                theArchive, theEncoding, theEntryNames, theReturnAll);
          }

          EntryNameSet&
          getNameSet() { return theEntryNames; }

        protected:
          zorba::String theEncoding;
          EntryNameSet theEntryNames;
          bool theReturnAll;
      };

    public:
      ExtractTextFunction(const ArchiveModule* aModule)
        : ArchiveFunction(aModule) {}

      virtual ~ExtractTextFunction() {}

      virtual zorba::String
        getLocalName() const { return "extract-text"; }

      virtual zorba::ItemSequence_t
        evaluate(const Arguments_t&,
                 const zorba::StaticContext*,
                 const zorba::DynamicContext*) const;
  };

/*******************************************************************************
 ******************************************************************************/
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
