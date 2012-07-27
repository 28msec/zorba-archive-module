import module namespace a = "http://www.zorba-xquery.com/modules/archive";
import module namespace f = "http://expath.org/ns/file";

  for $a in a:extract-text(f:read-binary(fn:resolve-uri("simple.zip")), "dir1/file1", "foo")
  return <text>{$a}</text>
