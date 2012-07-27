import module namespace a = "http://www.zorba-xquery.com/modules/archive";
import schema namespace as = "http://www.zorba-xquery.com/modules/archive";
import module namespace f = "http://expath.org/ns/file";

for $a in a:entries(f:read-binary(resolve-uri("invalid.zip")))
return validate { $a }

