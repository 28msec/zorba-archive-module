import module namespace a = "http://www.zorba-xquery.com/modules/archive";
import module namespace f = "http://expath.org/ns/file";

  for $a in a:extract-binary(f:read-binary(fn:resolve-uri("simple.zip")), "dir1/file1")
  return <text>{$a}</text>
