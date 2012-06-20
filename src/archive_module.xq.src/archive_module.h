#ifndef _ORG_EXPATH_WWW_NS_ARCHIVE_H_
#define _ORG_EXPATH_WWW_NS_ARCHIVE_H_

#include <map>
#include <set>

#include <zorba/zorba.h>
#include <zorba/item_factory.h>
#include <zorba/external_module.h>
#include <zorba/function.h>
#include <vector>

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

      class ArchiveEntries
      {
      protected:

        class ArchiveEntry
        {
        protected:
          String theEntryPath;
          int theSize;
          time_t theLastModified;

        public:
          ArchiveEntry();
          ArchiveEntry(zorba::Item aEntry);
          ~ArchiveEntry(){};

          String getEntryPath() { return theEntryPath; }
          int getSize() { return theSize; }
          int getLastModified() { return theLastModified; }

        };
        
        ArchiveEntry *theEntry;
        std::vector<ArchiveEntry> theEntries;

        void addEntry(zorba::Item aEntry)
        {
          theEntry = new ArchiveEntry(aEntry);
          theEntries.push_back(*theEntry);
          delete theEntry;
        };

      public:
        ArchiveEntries();
        ArchiveEntries(zorba::Iterator_t aEntriesIterator);
        ~ArchiveEntries(){};

        String 
        getEntryPath(int aPos)
        {
          return theEntries[aPos].getEntryPath();
        }
        int 
        getEntrySize(int aPos)
        {
          return theEntries[aPos].getSize();
        }
        int 
        getEntryLastModified(int aPos)
        {
          return theEntries[aPos].getLastModified();
        }

      };

      static zorba::Item
      getOneItem(const Arguments_t& aArgs, int aIndex);

#ifdef WIN32
      static long
#else
      static ssize_t
#endif    
      writeStream(struct archive *a, void *client_data, const void *buff, size_t n);

      static int
      closeStream(struct archive *a, void *client_data);

    public:

      ArchiveFunction(const ArchiveModule* module);

      virtual ~ArchiveFunction();

      virtual String
        getURI() const;

      static void
        throwError(const char*, const char*);

      static void
        checkForError(int aErrNo, const char* aLocalName, struct archive *a);

      static std::string
        formatName(int f);

      static std::string
        compressionName(int c);

  };

/*******************************************************************************
 ******************************************************************************/
  class CreateFunction : public ArchiveFunction{
    public:

      class ArchiveCompressor
      {
        protected:

          ArchiveEntries *theArchiveEntries;
          struct archive *theArchive;
          struct archive_entry *theEntry;
          std::stringstream* theStream;
          std::istream* theFileStream;
          char* theBuffer;
          

        public:
          
          ArchiveCompressor(zorba::Iterator_t aEntries);

          ~ArchiveCompressor();

          void open();

          void close();

          bool compress(zorba::Iterator_t aFiles);

          std::stringstream* getStream() const;
      };

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
  class ExtractFunction : public ArchiveFunction
  {
    protected:
      class ExtractItemSequence : public ArchiveItemSequence
      {
        public:
          struct StringComparator
          {
            bool operator() (const zorba::String& s1, const zorba::String& s2)
            {
              return s1.compare(s2);
            }
          };

          typedef std::set<zorba::String, StringComparator> EntryNameSet;
          typedef EntryNameSet::const_iterator EntryNameSetIter;

          class ExtractIterator : public ArchiveIterator
          {
            public:
              ExtractIterator(
                  zorba::Item& aArchive,
                  EntryNameSet& aEntryNames,
                  bool aReturnAll)
                : ArchiveIterator(aArchive),
                  theEntryNames(aEntryNames),
                  theReturnAll(aReturnAll) {}

              virtual ~ExtractIterator() {}

            protected:
              EntryNameSet& theEntryNames;
              bool theReturnAll;
          };

        public:
          ExtractItemSequence(
              zorba::Item& aArchive,
              bool aReturnAll)
            : ArchiveItemSequence(aArchive),
              theReturnAll(aReturnAll)
          {}

          virtual ~ExtractItemSequence() {}

          EntryNameSet&
          getNameSet() { return theEntryNames; }

        protected:
          EntryNameSet theEntryNames;
          bool theReturnAll;
      };

    public:
      ExtractFunction(const ArchiveModule* aModule)
        : ArchiveFunction(aModule) {}

      virtual ~ExtractFunction() {}
  };

/*******************************************************************************
 ******************************************************************************/
  class ExtractTextFunction : public ExtractFunction
  {
    protected:
      class ExtractTextItemSequence : public ExtractItemSequence
      {
        public:
          class ExtractTextIterator : public ExtractIterator
          {
            public:
              ExtractTextIterator(
                  zorba::Item& aArchive,
                  ExtractItemSequence::EntryNameSet& aEntryNames,
                  bool aReturnAll,
                  zorba::String& aEncoding)
                : ExtractIterator(aArchive, aEntryNames, aReturnAll),
                  theEncoding(aEncoding) {}

              virtual ~ExtractTextIterator() {}

              bool
              next(zorba::Item& aItem);

            protected:
              zorba::String& theEncoding;
          };

        public:
          ExtractTextItemSequence(
              zorba::Item& aArchive,
              bool aReturnAll,
              zorba::String& aEncoding)
            : ExtractItemSequence(aArchive, aReturnAll),
              theEncoding(aEncoding)
          {}

          virtual ~ExtractTextItemSequence() {}

          zorba::Iterator_t
          getIterator()
          {
            return new ExtractTextIterator(
                theArchive, theEntryNames, theReturnAll, theEncoding);
          }

        protected:
          zorba::String theEncoding;
      };

    public:
      ExtractTextFunction(const ArchiveModule* aModule)
        : ExtractFunction(aModule) {}

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
  class ExtractBinaryFunction : public ExtractFunction
  {
    protected:
      class ExtractBinaryItemSequence : public ExtractItemSequence
      {
        public:
          class ExtractBinaryIterator : public ExtractIterator
          {
            public:
              ExtractBinaryIterator(
                  zorba::Item& aArchive,
                  ExtractItemSequence::EntryNameSet& aEntryNames,
                  bool aReturnAll)
                : ExtractIterator(aArchive, aEntryNames, aReturnAll) {}

              virtual ~ExtractBinaryIterator() {}

              bool
              next(zorba::Item& aItem);
          };

        public:
          ExtractBinaryItemSequence(
              zorba::Item& aArchive,
              bool aReturnAll)
            : ExtractItemSequence(aArchive, aReturnAll)
          {}

          virtual ~ExtractBinaryItemSequence() {}

          zorba::Iterator_t
          getIterator()
          {
            return new ExtractBinaryIterator(
                theArchive, theEntryNames, theReturnAll);
          }
      };

    public:
      ExtractBinaryFunction(const ArchiveModule* aModule)
        : ExtractFunction(aModule) {}

      virtual ~ExtractBinaryFunction() {}

      virtual zorba::String
        getLocalName() const { return "extract-binary"; }

      virtual zorba::ItemSequence_t
        evaluate(const Arguments_t&,
                 const zorba::StaticContext*,
                 const zorba::DynamicContext*) const;
  };

/*******************************************************************************
 ******************************************************************************/
  class OptionsFunction : public ArchiveFunction
  {
    protected:
    class OptionsItemSequence : public ArchiveItemSequence
    {
      public:
        class OptionsIterator : public ArchiveIterator
        {
          public:
            OptionsIterator(zorba::Item& aArchive)
              : ArchiveIterator(aArchive) {}

            virtual ~OptionsIterator() {}

            void
            open()
            {
              ArchiveIterator::open();
              lExhausted = false;
            }

            bool
            next(zorba::Item& aItem);

          protected:
            bool lExhausted;
        };

      public:
        OptionsItemSequence(zorba::Item& aArchive)
          : ArchiveItemSequence(aArchive)
        {}

        virtual ~OptionsItemSequence() {}

        zorba::Iterator_t
        getIterator()
        {
          return new OptionsIterator(theArchive);
        }
    };

    public:
      OptionsFunction(const ArchiveModule* aModule)
        : ArchiveFunction(aModule) {}

      virtual ~OptionsFunction() {}

      virtual zorba::String
        getLocalName() const { return "options"; }

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
