import module namespace a = "http://www.zorba-xquery.com/modules/archive";

let $foo-content := "<foo/>"
let $bar-content := "<bar/>"
let $archive := a:create(
  ("foo.xml", "bar.xml", <entry type="directory">dir1</entry>),
  ($foo-content, $bar-content)
)
let $archive2 := a:delete($archive, "dir1/")
return
  for $e in a:entries($archive2) return concat($e/text(), ",")
