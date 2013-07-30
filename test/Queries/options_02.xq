import module namespace a = "http://zorba.io/modules/archive";
import module namespace f = "http://expath.org/ns/file";

let $tar-bz2 := f:read-binary(fn:resolve-uri("simple.tar.bz2"))
return a:options($tar-bz2)
