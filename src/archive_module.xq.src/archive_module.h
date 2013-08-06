/*
 * Copyright 2012 The FLWOR Foundation.
 * 
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 * 
 * http://www.apache.org/licenses/LICENSE-2.0
 * 
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */


#ifndef ORG_EXPATH_NS_ARCHIVE_H_
#define ORG_EXPATH_NS_ARCHIVE_H_

#include <map>
#include <set>

#include <zorba/zorba.h>
#include <zorba/item_factory.h>
#include <zorba/external_module.h>
#include <zorba/function.h>
#include <vector>

#define ZORBA_ARCHIVE_MAX_READ_BUF 2048

#define ZORBA_ARCHIVE_COMPRESSION_DEFLATE 50
#define ZORBA_ARCHIVE_COMPRESSION_STORE   51

namespace zorba { namespace archive {

#ifdef _WIN64
  typedef long long _ssize_t;
#elif WIN32
  typedef long _ssize_t;
#else
  typedef ssize_t _ssize_t;
#endif

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
      getModuleURI() { return "http://www.zorba-xquery.com/modules/archive"; }

      static zorba::Item
      createDateTimeItem(time_t&);

      static void
      parseDateTimeItem(const zorba::Item& i, time_t&);

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

          // needed if theArchiveItem is not streamable an needs to be decoded
          zorba::String   theDecodedData;

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

      static _ssize_t  
      readStream(struct archive *a, void *client_data, const void **buff);

      // needed for the "non-linear" zip format
#ifdef WIN32
      static __int64 seekStream(struct archive *a, void *data, __int64 request, int whence);
#else
      static off_t seekStream(struct archive *a, void *data, off_t request, int whence);
#endif
      

  };


/*******************************************************************************
 ******************************************************************************/
  class ArchiveFunction : public ContextualExternalFunction
  {
    protected:
      const ArchiveModule* theModule;

    public:
      class ArchiveOptions
      {
      protected:
        std::string theCompression;
        std::string theFormat;
        bool        theSkipExtraAttrs;

      public:

        ArchiveOptions();

        const std::string&
        getCompression() const { return theCompression; }

        const std::string&
        getFormat() const { return theFormat; }

        void
        setValues(Item&);

        void
        setValues(struct archive* aArchive);

        bool
        getSkipExtraAttrs() const { return theSkipExtraAttrs; }

      protected:
        static std::string
        getAttributeValue(
            const Item& aNode,
            const String& aAttrName = "value");
      };

      class ArchiveEntry
      {
      public:
        enum ArchiveEntryType { regular = 0, directory };

      protected:
        String theEntryPath;
        String theEncoding;
        long long theSize;
        time_t theLastModified;
        String theCompression;
        ArchiveEntryType theEntryType;
        bool theSkipExtras;

      public:
        ArchiveEntry();

        const String& getEntryPath() const { return theEntryPath; }

        const String& getEncoding() const { return theEncoding; }

        long long getSize() const { return theSize; }

        const time_t& getLastModified() const { return theLastModified; }
        
        const String& getCompression() const { return theCompression; }

        const ArchiveEntryType& getEntryType() const { return theEntryType; }

        void setValues(zorba::Item& aEntry);

        void setValues(struct archive_entry* aEntry);

        bool skipExtras() const { return theSkipExtras; }
      };

      class ArchiveCompressor
      {
      protected:

        struct archive *theArchive;
        struct archive_entry *theEntry;
        std::stringstream* theStream;
        ArchiveOptions  theOptions;

      public:
        ArchiveCompressor();

        ~ArchiveCompressor();

        void open(
          const ArchiveOptions& aOptions);

        void close();

        void compress(
          const std::vector<ArchiveEntry>& aEntries,
          zorba::Iterator_t& aFiles);

        void compress(
          const ArchiveEntry& aEntry,
          zorba::Item aFile);

        std::stringstream* getResultStream();

        static void
        releaseStream(std::istream* s) { delete s; }

      protected:
        bool
        getStream(
            const ArchiveEntry& aEntry,
            zorba::Item& aFile,
            std::istream*& aResStream,
            uint64_t& aResFileSize) const;

        bool
        getStreamForString(
            const zorba::String& aEncoding,
            zorba::Item& aFile,
            std::istream*& aResStream,
            uint64_t& aResFileSize) const;

        bool
        getStreamForBase64(
            zorba::Item& aFile,
            std::istream*& aResStream,
            uint64_t& aResFileSize) const;


        protected:
          void
          setOptions(const ArchiveOptions& aOptions);
      };

      
      static zorba::Item
      getOneItem(const Arguments_t& aArgs, int aIndex);


      static _ssize_t  
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

      static int
        formatCode(const std::string&);

      static int
        compressionCode(const std::string&);

      static void
        setArchiveCompression(struct archive*, int c);
  };

/*******************************************************************************
 ******************************************************************************/
  class CreateFunction : public ArchiveFunction
  {
    public:
      CreateFunction(const ArchiveModule* aModule)
        : ArchiveFunction(aModule) {}

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
              zorba::Item theEntryType;

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
    public:
      class ExtractItemSequence : public ArchiveItemSequence
      {
        public:

          typedef std::set<std::string> EntryNameSet;
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

              struct archive_entry* lookForHeader(bool aMatch, ArchiveOptions* aOptions = NULL);

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
    public:
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
    public:
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


/*******************************************************************************
 ******************************************************************************/

  class UpdateFunction : public ExtractFunction
  {
    class UpdateItemSequence : public ExtractItemSequence
      {
        public:
          class UpdateIterator : public ExtractIterator
          {
            public:
              UpdateIterator(
                  zorba::Item& aArchive,
                  EntryNameSet& aEntryNames,
                  bool aReturnAll,
                  ArchiveEntry& aEntry,
                  ArchiveOptions& aOptions)
                : ExtractIterator(aArchive, aEntryNames, aReturnAll),
                  theEntry(aEntry),
                  theOptions(aOptions){}

              virtual ~UpdateIterator() {}

              bool
              next(zorba::Item& aItem);

            protected:
              ArchiveEntry& theEntry;
              ArchiveOptions& theOptions;
          };

        public:
          UpdateItemSequence(
              zorba::Item& aArchive,
              bool aReturnAll)
            : ExtractItemSequence(aArchive, aReturnAll)
          {}

          virtual ~UpdateItemSequence() {}

          const ArchiveEntry&
          getEntry() { return theEntry; }

          ArchiveOptions&
          getOptions() { return theOptions; }

          zorba::Iterator_t
          getIterator()
          {
            return new UpdateIterator(
                theArchive, theEntryNames, theReturnAll, theEntry, theOptions);
          }

        protected:
          ArchiveEntry theEntry;
          ArchiveOptions theOptions;

      };

    public:
      UpdateFunction(const ArchiveModule* aModule) 
        : ExtractFunction(aModule) {}

      virtual ~UpdateFunction(){}

      virtual zorba::String
        getLocalName() const { return "update"; }

      virtual zorba::ItemSequence_t
        evaluate(const Arguments_t&,
                 const zorba::StaticContext*,
                 const zorba::DynamicContext*) const;
  };


/*******************************************************************************
 ******************************************************************************/

  class DeleteFunction : public ArchiveFunction{
     protected:
      class DeleteItemSequence : public ExtractFunction::ExtractItemSequence
      {
        public:
          class DeleteIterator : public ExtractFunction::ExtractItemSequence::ExtractIterator
          {
            public:
              DeleteIterator(zorba::Item& aArchive,
                  EntryNameSet& aEntryList,
                  ArchiveEntry& aEntry,
                  ArchiveOptions& aOptions)
                : ExtractIterator(aArchive, aEntryList, false),
                  theEntry(aEntry),
                  theOptions(aOptions){}

              virtual ~DeleteIterator() {}

              bool
              next(zorba::Item& aItem);

            protected:
              ArchiveEntry& theEntry;
              ArchiveOptions& theOptions;
          };

        //public:
          DeleteItemSequence(zorba::Item& aArchive)
            : ExtractFunction::ExtractItemSequence(aArchive, false) {}

          virtual ~DeleteItemSequence() {}

          const ArchiveEntry&
          getEntry() { return theEntry; }

          ArchiveOptions&
          getOptions() { return theOptions; }

          zorba::Iterator_t
          getIterator() 
          { 
            return new DeleteIterator(theArchive, theEntryNames, theEntry, theOptions); 
          }

        protected:
          ArchiveEntry theEntry;
          ArchiveOptions theOptions;
      };

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

namespace std {

ostream&
operator<<(
    ostream& out,
    const zorba::archive::ArchiveFunction::ArchiveEntry& e);

}

#endif // _ORG_EXPATH_WWW_NS_ARCHIVE_H_
