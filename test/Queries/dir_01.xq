import module namespace a = "http://zorba.io/modules/archive";

let $foo-content := "<foo/>"
let $bar-content := "<bar/>"
let $archive := a:create(
  ("foo.xml", "bar.xml", { "type" : "directory", "name" : "dir1" }),
  ($foo-content, $bar-content)
)
return
  for $e in a:entries($archive)("name") return concat($e, ",")
