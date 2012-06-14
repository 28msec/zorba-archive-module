module namespace a = "http://www.expath.org/ns/archive";
 
(:import schema namespace options = "http://www.expath.org/ns/archive";:)

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
  $entries as element(a:entry)*,
  $contents as item()*)
    as xs:base64Binary external;
 
(:~
 : <a:options archive-format="zip">
 :   <a:algorithm value="stored"/>
 : </a:options>
 :)
declare function a:create(
  $entries as element(a:entry)*,
  $contents as item()*,
  $options as element(a:options))
    as xs:base64Binary external; 
  
(:~
 : Return the specification of the given archive.
 :
 : Example:
 : <a:entry last-modified="2009-03-20T03:30:32" compressed-size="232" uncompressed-size"324" encrypted="false">
 :   myfile.txt
 : </a:entry>
 : <a:entry>
 :   dir/myfile.xml
 : </a:entry>
 : <a:entry>
 :   dir/dir2/image.png
 : </a:entry>
 :
 : @return specification
 :)
declare function a:entries($archive as xs:base64Binary)
    as element(a:entry)* external;
  
(:~
 : Returns all entries from the archive as a string.
 : The default encoding used to read the string is UTF-8.
 :
 : @return strings
 :
 : @error if the file contains invalid utf-8 characters
 : @error if the file doesn't exist
 :)
declare function a:extract-text($archive as xs:base64Binary)
    as xs:string* external;
  
(:~
 : Returns the entries identified by the given paths from the archive
 : as a string.
 : The default encoding used to read the string is UTF-8.
 :
 : @return strings
 :
 : @error if the file contains invalid utf-8 characters
 : @error if the file doesn't exist
 :)
declare function a:extract-text($archive as xs:base64Binary, $entry-names as xs:string*)
    as xs:string* external;
  
(:~
 : Returns the entries identified by the given paths from the archive
 : as a string.
 :
 : @return strings
 :
 : @error if the file contains invalid characters
 : @error if a transcoding error happens
 :)
declare function a:extract-text($archive as xs:base64Binary, $entry-names as xs:string*, $encoding as xs:string)
    as xs:string* external;
  
(:~
 : Returns all entries from the archive
 : as base64Binary.
 :
 : @return base64Binary items
 :
 : @error if the file does not exist
 :)
declare function a:extract-binary($archive as xs:base64Binary)
    as xs:base64Binary* external;
  
(:~
 : Returns the entries identified by the given paths from the archive
 : as base64Binary.
 :
 : @return base64Binary items
 :
 : @error if the file does not exist
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
declare function a:update($archive as xs:base64Binary, $entries as element(entry)*, $contents as item()*)
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