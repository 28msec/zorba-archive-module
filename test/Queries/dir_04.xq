import module namespace a = "http://www.zorba-xquery.com/modules/archive";

let $foo-content := "<foo/>"
let $bar-content := "<bar/>"
let $archive := a:create(
  ("foo.xml", "bar.xml", <entry type="directory">dir1</entry>),
  ($foo-content, $bar-content)
)
let $archive2 := a:update($archive, <a:entry type="directory">newdir</a:entry>, ())
let $entries := a:entries($archive)
let $entries2 := a:entries($archive2)
return for $e in $entries2 return concat($e/text(), ",")
  
