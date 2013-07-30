import module namespace a = "http://zorba.io/modules/archive";
import module namespace b = "http://zorba.io/modules/base64";

let $foo-content := "<foo/>"
let $bar-content := xs:base64Binary("YWJj")
let $archive := a:create(
  ("foo.xml", "bar.txt"),
  ($foo-content, $bar-content)
)
let $new-archive := a:update($archive, "foo2.xml", "<foo2/>")
return (count(a:entries($new-archive)), a:extract-text($new-archive, "foo2.xml"))

