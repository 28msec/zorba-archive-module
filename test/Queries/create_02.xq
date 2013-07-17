import module namespace a = "http://www.zorba-xquery.com/modules/archive";
import module namespace b = "http://zorba.io/modules/base64";

let $foo-content := "<foo/>"
let $bar-content := xs:base64Binary("5Pb8")
let $archive := a:create(
  ("foo.xml", <a:entry encoding="ISO-8859-1">bar.xml</a:entry>),
  ($foo-content, $bar-content)
)
return
  b:decode(a:extract-binary($archive, "bar.xml"), "ISO-8859-1") eq "äöü"
