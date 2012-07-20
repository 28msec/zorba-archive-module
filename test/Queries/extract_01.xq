import module namespace a = "http://expath.org/ns/archive";
import schema namespace as = "http://expath.org/ns/archive";
import module namespace f = "http://expath.org/ns/file";

for $a in a:extract-text(f:read-binary(resolve-uri("simple.zip")))
return <text>{ $a }</text>
