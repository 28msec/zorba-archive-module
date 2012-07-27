import module namespace a = "http://www.zorba-xquery.com/modules/archive";

let $foo-content := "<foo/>"
let $bar-content := "<bar/>"
let $archive := a:create(
  (<a:entry compression="store">foo.xml</a:entry>, <a:entry compression="deflate">bar.xml</a:entry>),
  ($foo-content, $bar-content)
)
return
  string-join(
    for $a in a:entries($archive)
    return a:extract-text($archive, $a/text())
  ) eq concat($foo-content, $bar-content)
 
