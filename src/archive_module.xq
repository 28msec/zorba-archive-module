jsoniq version "1.0";

(:
 : Copyright 2012 The FLWOR Foundation.
 :
 : Licensed under the Apache License, Version 2.0 (the "License");
 : you may not use this file except in compliance with the License.
 : You may obtain a copy of the License at
 :
 : http://www.apache.org/licenses/LICENSE-2.0
 :
 : Unless required by applicable law or agreed to in writing, software
 : distributed under the License is distributed on an "AS IS" BASIS,
 : WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 : See the License for the specific language governing permissions and
 : limitations under the License.
 :)

(:~
 : This module provides functionality to work with (possibly compressed)
 : archives. For example, it provides functions to retrieve the names or
 : extract the values of several entries in a ZIP archive. Moreover,
 : there exist functions that allow to create or update archives.<p/>
 :
 : The following archive formats and compression algorithms are supported:
 : <ul>
 :   <li>ZIP (with compression DEFLATE or STORE)</li>
 :   <li>TAR (with compression GZIP)</li>
 : </ul>
 : </p>
 : 
 : @author Luis Rodgriguez, Juan Zacarias, and Matthias Brantner
 :
 : @library <a href="http://code.google.com/p/libarchive/">libarchive</a>
 : @project Zorba/Archive
 :)
module namespace a = "http://zorba.io/modules/archive";
 
declare namespace ver = "http://www.zorba-xquery.com/options/versioning";
declare option ver:module-version "1.0";
  
(:~
 : Creates a new ZIP archive out of the given entries and contents. <p/>
 :
 : All entries are compressed with the DEFLATE compression algorithm.<p/>
 : 
 : The parameters $entries and $contents have the same meaning as for
 : the function a:create with three arguments.<p/>
 :
 : Entry entries can include a type element, this element can have one
 : of two possible values: "regular" or "directory". If "regular" is
 : specified then the entry will be created as a regular file; if "directory"
 : is specified then the entry will be created as a directory, no contents
 : will be read from $contents in this case. If no value is specified for type
 : then it will be set to "regular". <p/>
 : Example:
 : <pre>
 : $zip-file := a:create(
 :    ({ "encoding" : "ISO-8859-1", "type" : "directory", "entry" : "dir1" }, "dir1/file1"),
 :    ("file contents"))
 : </pre>
 : <p/>
 :
 : @param $entries the meta data for the entries in the archive. Each entry
 :   can be of type xs:string or a JSON oibject describing the entry.
 : @param $contents the content for the archive. Each item in the sequence
 :   can be of type xs:string or xs:base64Binary.
 :
 : @return the generated archive as xs:base64Binary
 :
 : @error a:ENTRY-COUNT-MISMATCH if the number of entries that don't describe directories
 :        differs from the number of items in the $contents sequence: 
 :        count($non-directory-entries) ne count($contents)
 : @error a:INVALID-ENTRY-VALS if a values in an entry object are invalid
 : @error a:INVALID-ENCODING if a given encoding is invalid or not supported
 : @error err:FORG0006 if an item in the contents sequence is not of type xs:string
 :   or xs:base64Binary
 :)
declare function a:create(
  $entries as item()*,
  $contents as item()*)
    as xs:base64Binary external;
 
(:~
 : Creates a new archive out of the given entries and contents. <p/>
 :
 : The $entries arguments provides meta data for each entry in the archive.
 : For example, the name of the entry (mandatory) or the last-modified date
 : (optional). An entry can either be of type xs:string to describe the entry
 : name or of type xs:base64Binary to provide additional meta data.<p/>
 :
 : The $contents sequence provides the data (xs:string or xs:base64Binary) for
 : the entries that should be included in the archive. Its length needs to
 : match the length of the entries in the $entries sequence that don't describe
 : directory entries (a:ARCH0001). All items of type xs:base64Binary are decoded
 : before being added to the archive.<p/>
 :
 : For each entry, the name, last-modified date and time, and compression
 : can be specified. In addition, an encoding can be specified which is used to
 : store entries of type xs:string. If no last-modified attribute is given, the
 : default is the current date and time. The compression is useful if various
 : entries in a ZIP archive are compressed using different compression
 : algorithms (i.e. store or deflate).<p/>
 :
 : For example, the following sequence may be used to describe an archive
 : containing two elements: <p/>
 : <pre class="ace-static" ace-mode="xquery">{
 :   "last-modified" : "{fn:current-dateTime()}"
 :   "name" : "myfile.txt"
 : },
 : {
 :   "encoding" : "ISO-8859-1",
 :   "compression" : "store",
 :   "name" : "dir/myfile.xml"
 : }
 : </pre>
 : </p>
 :
 : The $options argument may be used to describe general options for the
 : archive.  For example, the following options can be used to create a ZIP
 : archive in which all entries are compressed with the DEFLATE compression
 : algorithm: <p/>
 : <pre class="ace-static" ace-mode="xquery">{
 :   "format" : "ZIP",
 :   "compression" : "DEFLATE"
 : }
 : </pre>
 : <p/>
 :
 : The result of the function is the generated archive as a item of type
 : xs:base64Binary.<p/>
 :
 :
 : @param $entries the meta data for the entries in the archive. Each entry
 :   can be of type xs:string or an JSON object describing the entry.
 : @param $contents the content for the archive. Each item in the sequence
 :   can be of type xs:string or xs:base64Binary.
 : @param $options the options used to generate the archive.
 :
 : @return the generated archive as xs:base64Binary
 :
 : @error a:ENTRY-COUNT-MISMATCH if the number of entries describing non-directories differs
 :        from the number of items in the $contents sequence:
 :        count($non-directoy-entries) ne count($contents)
 : @error a:INVALID-OPTIONS if the options argument contains invalid values
 : @error a:INVALID-ENTRY-VALS if any values in an entry are invalid
 : @error a:INVALID-ENCODING if a given encoding is invalid or not supported
 : @error a:DIFFERENT-COMPRESSIONS-NOT-SUPPORTED if different compression algorithms
 :        were selected but the actual version of libarchive doesn't support it.
 : @error err:FORG0006 if an item in the contents sequence is not of type xs:string
 :   or xs:base64Binary
 :)
declare function a:create(
  $entries as item()*,
  $contents as item()*,
  $options as object())
    as xs:base64Binary external; 
  
(:~
 : Returns the header information of all entries in the given archive as a JSON
 : objects sequence. <p/>
 :
 : Such information includes the name of the entry, the uncompressed size,
 : as well as the last-modified timestamp. Note that not all values are
 : available in every archive.<p/>
 :
 : @param $archive the archive to list the entries from as xs:base64Binary
 :
 : @return a sequence of strings, one for each entry in the archive
 :
 : @error a:CORRUPTED-ARCHIVE if $archive is not an archive or corrupted
 :)
declare function a:entries($archive as xs:base64Binary)
    as object()* external;
  
(:~
 : Extracts the contents of all entries in the given archive as text
 : using UTF-8 as default encoding. <p/>
 :
 : @param $archive the archive to extract the entries from as xs:base64Binary
 :
 : @return one string for the contents of each entry in the archive
 :
 : @error a:CORRUPTED-ARCHIVE if $archive is not an archive or corrupted
 : @error err:FOCH0001 if any of the entries contains invalid utf-8 characters
 :)
declare function a:extract-text($archive as xs:base64Binary)
    as xs:string* external;
  
(:~
 : Extracts the contets of the entries identified by a given sequence of
 : names as text.
 : The default encoding used to read the string is UTF-8. <p/>
 :
 : @param $archive the archive to extract the entries from as xs:base64Binary
 : @param $entry-names a sequence of names for entries which should be extracted
 :
 : @return a sequence of strings for the given sequence of names or the
 :   empty sequence if no entries match the given names.
 :
 : @error a:CORRUPTED-ARCHIVE if $archive is not an archive or corrupted
 : @error err:FOCH0001 if any of the entries requested contains invalid utf-8 characters
 :)
declare function a:extract-text($archive as xs:base64Binary, $entry-names as xs:string*)
    as xs:string* external;
  
(:~
 : Extracts the contets of the entries identified by a given sequence of
 : names as text. Each entry is treated with the given encoding. <p/>
 :
 : @param $archive the archive to extract the entries from as xs:base64Binary
 : @param $entry-names a sequence of entry names that should be extracted
 : @param $encoding a encoding for transcoding each of the extracted entries
 :
 : @return a sequence of strings for the given sequence of names or the
 :   empty sequence if no entries match the given names.
 :
 : @error a:CORRUPTED-ARCHIVE if $archive is not an archive or corrupted
 : @error a:INVALID-ENCODING if the given $encoding is invalid or not supported
 : @error err:FOCH0001 if a transcoding error happens
 :)
declare function a:extract-text(
  $archive as xs:base64Binary,
  $entry-names as xs:string*,
  $encoding as xs:string)
    as xs:string* external;
  
(:~
 : Returns the entries identified by the given paths from the archive
 : as base64Binary. <p/>
 :
 : @param $archive the archive to extract the entries from as xs:base64Binary
 :
 : @return one xs:base64Binary item for the contents of each entry in the archive
 :
 : @error a:CORRUPTED-ARCHIVE if $archive is not an archive or corrupted
 :)
declare function a:extract-binary($archive as xs:base64Binary)
    as xs:base64Binary* external;
  
(:~
 : Returns the entries identified by the given paths from the archive
 : as base64Binary. <p/>
 :
 : @param $archive the archive to extract the entries from as xs:base64Binary
 :
 : @param $entry-names a sequence of names for entries which should be extracted
 :
 : @return a sequence of xs:base64Binary itmes for the given sequence of names
 :  or the empty sequence if no entries match the given names.
 :
 : @error a:CORRUPTED-ARCHIVE if $archive is not an archive or corrupted
 :)
declare function a:extract-binary($archive as xs:base64Binary, $entry-names as xs:string*)
    as xs:base64Binary* external;
  
(:~
 : Adds and replaces entries in an archive according to
 : the given spec. The contents can be string and base64Binary items. <p/>
 :
 : The parameters $entries and $contents have the same meaning as for
 : the function a:create with three arguments.</p>
 :  
 : @param $archive the archive to add or replace content
 : @param $entries the meta data for the entries in the archive. Each entry
 :   can be of type xs:string or a JSON object. For mandatory fields in the
 :   JSON object see create function.
 : @param $contents the content for the archive. Each item in the sequence
 :   can be of type xs:string or xs:base64Binary.
 :  
 : @return the updated xs:base64Binary
 :
 : @error a:ENTRY-COUNT-MISMATCH if the number of entry elements differs from the number
 :        of items in the $contents sequence: count($non-directory-entries) ne count($contents) 
 : @error a:INVALID-ENTRY-VALS if a value for an entry element is invalid
 : @error a:INVALID-ENCODING if a given encoding is invalid or not supported
 : @error a:DIFFERENT-COMPRESSIONS-NOT-SUPPORTED if different compression algorithms
 :        were selected but the actual version of libarchive doesn't support it.
 : @error err:FORG0006 if an item in the contents sequence is not of type xs:string
 :   or xs:base64Binary
 : @error a:CORRUPTED-ARCHIVE if $archive is not an archive or corrupted
 :
 :)
declare function a:update($archive as xs:base64Binary, $entries as item()*, $contents as item()*)
    as xs:base64Binary external;
  
(:~
 : Deletes entries from an archive. <p/>
 :
 : @param $archive the archive to extract the entries from as xs:base64Binary
 : @param $entry-names a sequence of names for entries which should be deleted
 : 
 : @return the updated base64Binary
 :
 : @error a:CORRUPTED-ARCHIVE if $archive is not an archive or corrupted
 :)
declare function a:delete($archive as xs:base64Binary, $entry-names as xs:string*)
    as xs:base64Binary external;

(:~
 : Returns the algorithm and format options as a JSON object for a given archive.
 : For example, for a ZIP archive, the following options element
 : would be returned: <p/>
 : <pre class="ace-static" ace-mode="xquery">{
 :   "format" : "ZIP",
 :   "compression" : "DEFLATE"
 : }
 : </pre>
 : <p/>
 :
 : @param $archive the archive as xs:base64Binary
 :
 : @return the algorithm and format options as a JSON object
 :
 : @error a:CORRUPTED-ARCHIVE if $archive is not an archive or corrupted
 :)
declare function a:options($archive as xs:base64Binary)
  as object() external;
