xquery version "3.0"
(:
 : Copyright 2011 The FLWOR Foundation.
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

module namespace zip = "http://www.expath.org/ns/zip2";
(:~
 : Create a new zip archiving according to the given spec.
 : The contents can be string and base64Binary items.
 :
 : Example:
 : <!-- specify global compression level -->
 : <entries compression-level="9">
 :   <!-- overwrite compression level with '0' (uncompressed) -->
 :   <entry last-modified="" compression-level="0">
 :     myfile.txt
 :   </entry>
 :   <!-- specify encoding for text entries -->
 :   <entry encoding="Shift_JIS">
 :     dir/myfile.xml
 :   </entry>
 :   <entry>
 :     dir/dir2/image.png
 :   </entry>
 : </entries>
 :
 : @return a base64Binary 
 :
 : @error if the number of entry elements differs from the number
 :        of specified contents: count($entries/entry) ne count($contents)
 : @error if an encoding is specified and the item is a base64Binary
 :)
declare function zip:create($entries as element(entries), $contents as item()*)
    as xs:base64Binary external;
 
(:~
 : Return the specification of the given zip archive.
 :
 : Example:
 : <entries>
 :   <entry last-modified="2009-03-20T03:30:32" compressed-size="232" uncompressed-size"324" encrypted="false">
 :     myfile.txt
 :   </entry>
 :   <entry>
 :     dir/myfile.xml
 :   </entry>
 :   <entry>
 :     dir/dir2/image.png
 :   </entry>
 : </entries>
 :
 : @return specification
 :)
declare function zip:entries($zip as xs:base64Binary)
    as element(entries) external;
 
(:~
 : Return the entry identified by the given path from the zip archive
 : as a string.
 : The default encoding used to read the string is UTF-8.
 :
 : @return a string
 :
 : @error if the file contains invalid utf-8 characters
 : @error if the file doesn't exist
 :)
declare function zip:get-text($zip as xs:base64Binary, $path as xs:string)
    as xs:string external;
 
(:~
 : Return the entry identified by the given path from the zip archive
 : as a string.
 :
 : @return a string
 :
 : @error if the file contains invalid characters
 : @error if a transcoding error happens
 :)
declare function zip:get-text($zip as xs:base64Binary, $path as xs:string, $encoding as xs:string)
    as xs:string external;
 
(:~
 : Return the entry identified by the given path from the zip archive
 : as a base64Binary.
 :
 : @return a base64Binary
 :
 : @error if the file doesn't exist
 :)
declare function zip:get-binary($zip as xs:base64Binary, $path as xs:string)
    as xs:base64Binary external;
 
(:~
 : Add, update and delete entries in a zip archive according to
 : the given spec. The contents can be string and base64Binary items.
 : If a specification entry has a delete attribute with its value set
 : to "true", this entry will be removed.
 :
 : Example:
 : <!-- specify global compression level -->
 : <entries compression-level="9">
 :   <!-- overwrite compression level -->
 :   <entry compression-level="0">
 :     image.png
 :   </entry>
 :   <!-- specify encoding for text entries -->
 :   <entry encoding="UTF-8">
 :     dir/myfile.xml
 :   </entry>
 :   <!-- entry will be deleted -->
 :   <entry delete="true">
 :     to/be/deleted.txt
 :   </entry>
 : </entries>
 :
 : @return the updated base64Binary
 :
 : @error if the number of non-deleting entry elements differs from the number
 :        of specified contents:
 :        count($entries/entry[not(@delete = 'true')]) ne count($contents)
 :)
declare function zip:update($zip as xs:base64Binary, $entries as element(entries), $contents as item()*)
    as xs:base64Binary external;
 
(:~
 : Add entries in a zip archive according to
 : the given spec. The contents can be string and base64Binary items.
 :
 : @return the updated base64Binary
 :
 : @error if the number of entry elements differs from the number
 :        of specified contents: count($entries/entry) ne count($contents)
 :
 :)
declare function zip:add($zip as xs:base64Binary, $entries as element(entries), $contents as item()*)
    as xs:base64Binary external;
 
(:~
 : Replaces entries in a zip archive according to
 : the given spec. The contents can be string and base64Binary items.
 :
 : @return the updated base64Binary
 :
 : @error if the number of entry elements differs from the number
 :        of specified contents: count($entries/entry) ne count($contents)
 : @error if an addressed entry does not exist
 :)
declare function zip:replace($zip as xs:base64Binary, $entries as element(entries), $contents as item()*)
    as xs:base64Binary external;
 
(:~
 : Deletes entries from a zip archive.
 :
 : @return the updated base64Binary
 :
 : @error if an addressed entry does not exist
 :)
declare function zip:delete($zip as xs:base64Binary, $paths as xs:string*)
    as xs:base64Binary external;