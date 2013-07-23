import module namespace a = "http://zorba.io/modules/archive";
(: import schema namespace as = "http://zorba.io/modules/archive"; :)
import module namespace f = "http://expath.org/ns/file";

for $a in a:entries(f:read-binary(resolve-uri("simple.tar.bz2")))
(: return validate { $a } :)
return $a

