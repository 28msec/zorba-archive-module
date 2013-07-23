import module namespace a = "http://zorba.io/modules/archive";

let $foo-content := "<foo/>"
let $bar-content := "<bar/>"
let $archive := a:create(
  ("foo.xml", "bar.xml"),
  ($foo-content, $bar-content)
)
return
  string-join(
    for $name in a:entries($archive)("name")
    return a:extract-text($archive, $name)
  ) eq concat($foo-content, $bar-content)

