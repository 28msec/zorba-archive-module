xquery version "1.0";

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
 : <p>This module provides functionality to work with (possibly compressed)
 : archives. For example, it provides functions to retrieve the names or
 : extract the values of several entries in a ZIP archive. Moreover,
 : there exist functions that allow to create or update archives.</p>
 :
 : <p>
 : The following archive formats are supported:
 : <ul>
 :   <li>ZIP</li>
 :   <li>TAR</li>
 :   <li>CPIO</li>
 :   <li>SHAR</li>
 :   <li>ISO9660</li>
 :   <li>AR</li>
 :   <li>MTREE</li>
 :   <li>RAW</li>
 :   <li>XAR</li>
 : </ul>
 :
 : Additionally, the modules supports the following compression algorithms:
 :
 : <ul>
 :   <li>GZIP</li>
 :   <li>BZIP2</li>
 :   <li>COMPRESS</li>
 :   <li>LZMA</li>
 :   <li>XZ</li>
 :   <li>UU (not supported for writing archives)</li>
 :   <li>RPM (not supported for writing archives)</li>
 : </ul>
 : </p>
 : 
 : @author Luis Rodgriguez, Juan Zacarias, and Matthias Brantner
 :
 : @library <a href="http://code.google.com/p/libarchive/">libarchive</a>
 :)
module namespace a = "http://www.expath.org/ns/archive";
 
import schema namespace options = "http://www.expath.org/ns/archive";

declare namespace ver = "http://www.zorba-xquery.com/options/versioning";
declare option ver:module-version "1.0";
  
(:~
 : Create a new archive according to the given spec.
 : The contents can be string and base64Binary items.
 :
 : Example:
 : <!-- specify compression level (0 = uncompressed) -->
 : <a:entry last-modified="" compression-level="0">
 :   myfile.txt
 : </a:entry>
 : <!-- specify encoding for text entries -->
 : <a:entry encoding="Shift_JIS">
 :   dir/myfile.xml
 : </a:entry>
 : <a:entry>
 :   dir/dir2/image.png
 : </a:entry>
 :
 : @return a base64Binary 
 :
 : @error if the number of entry elements differs from the number
 :        of specified contents: count($entries) ne count($contents)
 : @error if an encoding is specified and the item is a base64Binary
 :)
declare function a:create(
  $entries as item()*,
  $contents as item()*)
    as xs:base64Binary external;
 
(:~
 : <a:options archive-format="zip">
 :   <a:algorithm value="stored"/>
 : </a:options>
 :)
declare function a:create(
  $entries as item()*,
  $contents as item()*,
  $options as element(a:options))
    as xs:base64Binary external; 
  
(:~
 : Returns the header information of all entries in the given archive.
 :
 : <p>Such information includes the name of the entry, the uncompressed size,
 : as well as the last-modified timestamp. Note that not all values are
 : available in every archive.</p>
 :
 : @param $archive the archive to list the entries from as xs:base64Binary
 :
 : @return a sequence of strings, one for each entry in the archive
 :
 : @error a:ARCH9999 if $archive is not an archive or corrupted
 :)
declare function a:entries($archive as xs:base64Binary)
    as element(a:entry)* external;
  
(:~
 : Extracts the contents of all entries in the given archive as text
 : using UTF-8 as default encoding.
 :
 : @param $archive the archive to extract the entries from as xs:base64Binary
 :
 : @return one string for the contents of each entry in the archive
 :
 : @error a:ARCH9999 if $archive is not an archive or corrupted
 : @error err:FOCH0001 if any of the entries contains invalid utf-8 characters
 :)
declare function a:extract-text($archive as xs:base64Binary)
    as xs:string* external;
  
(:~
 : Extracts the contets of the entries identified by a given sequence of
 : names as text.
 : The default encoding used to read the string is UTF-8.
 :
 : @param $archive the archive to extract the entries from as xs:base64Binary
 : @param $entry-names a sequence of entry names that should be extracted
 :
 : @return a sequence of strings for the given sequence of names or the
 :   empty sequence if no entries match the given names.
 :
 : @error a:ARCH9999 if $archive is not an archive or corrupted
 : @error err:FOCH0001 if any of the entries requested contains invalid utf-8 characters
 :)
declare function a:extract-text($archive as xs:base64Binary, $entry-names as xs:string*)
    as xs:string* external;
  
(:~
 : Extracts the contets of the entries identified by a given sequence of
 : names as text. Each entry is treated with the given encoding.
 :
 : @param $archive the archive to extract the entries from as xs:base64Binary
 : @param $entry-names a sequence of entry names that should be extracted
 : @param $encoding a encoding for transcoding each of the extracted entries
 :
 : @return a sequence of strings for the given sequence of names or the
 :   empty sequence if no entries match the given names.
 :
 : @error a:ARCH9999 if $archive is not an archive or corrupted
 : @error a:ARCH0004 if the given $encoding is invalid or not supported
 : @error err:FOCH0001 if a transcoding error happens
 :)
declare function a:extract-text(
  $archive as xs:base64Binary,
  $entry-names as xs:string*,
  $encoding as xs:string)
    as xs:string* external;
  
(:~
 : Returns the entries identified by the given paths from the archive
 : as base64Binary.
 :
 : @param $archive the archive to extract the entries from as xs:base64Binary
 :
 : @return one xs:base64Binary item for the contents of each entry in the archive
 :
 : @error a:ARCH9999 if $archive is not an archive or corrupted
 :)
declare function a:extract-binary($archive as xs:base64Binary)
    as xs:base64Binary* external;
  
(:~
 : Returns the entries identified by the given paths from the archive
 : as base64Binary.
 :
 : @param $entry-names a sequence of entry names that should be extracted
 :
 : @return a sequence of xs:base64Binary itmes for the given sequence of names
 :  or the empty sequence if no entries match the given names.
 :
 : @error a:ARCH9999 if $archive is not an archive or corrupted
 :)
declare function a:extract-binary($archive as xs:base64Binary, $entry-names as xs:string*)
    as xs:base64Binary* external;
  
(:~
 : Adds and replaces entries in a archive according to
 : the given spec. The contents can be string and base64Binary items.
 :
 : @return the updated base64Binary
 :
 : @error if the number of entry elements differs from the number
 :        of specified contents: count($entries) ne count($contents)
 :
 :)
declare function a:update($archive as xs:base64Binary, $entries as item()*, $contents as item()*)
    as xs:base64Binary external;
  
(:~
 : Deletes entries from a archive.
 :
 : @return the updated base64Binary
 :
 : @error if an addressed entry does not exist
 :)
declare function a:delete($archive as xs:base64Binary, $entry-names as xs:string*)
    as xs:base64Binary external;

(:~
 : Returns the algorithm and format options of the given archive.
 : For example, for a ZIP archive, the following options element
 : would be returned:
 : <pre>
 : &lt;options xmlns="http://www.expath.org/ns/archive">
 :   &lt;format value="ZIP"/>
 :   &lt;algorithm value="DEFLATE"/>
 : &lt;/options>
 : </pre>
 :
 : @param $archive the archive as xs:base64Binary
 :
 : @return the algorithm and format options
 :
 : @error a:ARCH9999 if $archive is not an archive or corrupted
 :)
declare function a:options($archive as xs:base64Binary)
  as element(a:options) external;
