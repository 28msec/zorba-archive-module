import module namespace a = "http://zorba.io/modules/archive";

let $foo-content := "<foo/>"
let $bar-content := "<bar/>"
let $archive := a:create(
  ({ "compression" : "store", "name" : "foo.xml" },
   { "compression" : "deflate", "name" : "bar.xml" }),
  ($foo-content, $bar-content)
)
return
  string-join(
    for $a in a:entries($archive)
    return a:extract-text($archive, $a/text())
  ) eq concat($foo-content, $bar-content)
 
