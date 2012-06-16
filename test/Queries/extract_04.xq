import module namespace a = "http://www.expath.org/ns/archive";
import module namespace f = "http://expath.org/ns/file";

for $a in a:extract-text(f:read-binary(fn:resolve-uri("transcoding.zip")), "dir2/iso-8859-1.txt", "ISO-8859-1")
return <text>{$a}</text>
