import module namespace a = "http://zorba.io/modules/archive";

let $foo-content := "<foo/>"
let $bar-content := "<bar/>"
let $archive := a:create(
  ("foo.xml", "bar.xml", { "type" : "directory", "name" : "dir1" }),
  ($foo-content, $bar-content)
)
let $archive2 := a:delete($archive, "nonexistent.xml")
let $entries := a:entries($archive)
let $entries2 := a:entries($archive2)
return count($entries)=count($entries2)
  
