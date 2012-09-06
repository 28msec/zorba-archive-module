import module namespace a = "http://www.zorba-xquery.com/modules/archive";

let $foo-content := "<foo/>"
let $bar-content := "<bar/>"
let $archive := a:create(
  ("foo.xml", "bar.xml", <entry type="directory">dir1</entry>),
  ($foo-content, $bar-content)
)
let $archive2 := a:delete($archive, "nonexistent.xml")
let $entries := a:entries($archive)
let $entries2 := a:entries($archive2)
return $entries=$entries2
  
