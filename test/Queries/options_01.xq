import module namespace a = "http://zorba.io/modules/archive";
import module namespace f = "http://expath.org/ns/file";

let $zip := f:read-binary(fn:resolve-uri("linear-algebra-20120306.epub"))
let $tar-gz := f:read-binary(fn:resolve-uri("simple.tar.gz"))
return (
  a:options($zip), a:options($tar-gz)
)
