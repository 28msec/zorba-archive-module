import module namespace a = "http://expath.org/ns/archive";
import schema namespace as = "http://expath.org/ns/archive";
import module namespace f = "http://expath.org/ns/file";

let $a := f:read-binary(resolve-uri("linear-algebra-20120306.epub"))
let $b := a:delete($a, "EPUB/xhtml/fcla-xml-2.30li46.html")
return (count(a:entries($a)), count(a:entries($b)))
