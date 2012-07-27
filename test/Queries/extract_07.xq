import module namespace a = "http://www.zorba-xquery.com/modules/archive";
import module namespace f = "http://expath.org/ns/file";

let $f := f:read-binary(fn:resolve-uri("linear-algebra-20120306.epub"))
for $a in a:entries($f)[position() > 2 and  position() < 5]
return <text>{a:extract-binary($f, $a)}</text>
