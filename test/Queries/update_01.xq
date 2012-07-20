import module namespace a = "http://expath.org/ns/archive";
import module namespace b = "http://www.zorba-xquery.com/modules/converters/base64";

let $foo-content := "<foo/>"
let $bar-content := xs:base64Binary("YWJj")
let $archive := a:create(
  ("foo.xml", "bar.txt"),
  ($foo-content, $bar-content)
)
let $new-archive := a:update($archive, "foo.xml", "<bar/>")
return (count(a:entries($new-archive)), a:extract-text($new-archive, "foo.xml"))

