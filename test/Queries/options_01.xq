import module namespace a = "http://expath.org/ns/archive";
import module namespace f = "http://expath.org/ns/file";

let $zip := f:read-binary(fn:resolve-uri("linear-algebra-20120306.epub"))
let $tar-bz2 := f:read-binary(fn:resolve-uri("simple.tar.bz2"))
let $tar-gz := f:read-binary(fn:resolve-uri("simple.tar.gz"))
return (
  a:options($zip), a:options($tar-bz2), a:options($tar-gz)
)
