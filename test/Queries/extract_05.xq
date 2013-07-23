import module namespace a = "http://zorba.io/modules/archive";
import module namespace f = "http://expath.org/ns/file";

let $f := f:read-binary(resolve-uri("linear-algebra-20120306.epub"))
for $a in a:entries($f)("name")
where $a eq "EPUB/xhtml/fcla-xml-2.30li91.html"
return a:extract-text($f, $a)
