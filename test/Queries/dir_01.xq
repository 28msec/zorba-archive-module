import module namespace a = "http://www.zorba-xquery.com/modules/archive";

let $foo-content := "<foo/>"
let $bar-content := "<bar/>"
let $archive := a:create(
  ("foo.xml", "bar.xml", <entry type="directory">dir1</entry>),
  ($foo-content, $bar-content)
)
return
  for $e in a:entries($archive) return concat($e/text(), ",")
