import module namespace a = "http://expath.org/ns/archive";

let $foo-content := "<foo/>"
let $bar-content := xs:base64Binary("YWJj")
let $foo2-content := "<foo2/>"
let $options := 
<a:options>
 <a:format>TAR</a:format>
 <a:compression>GZIP</a:compression>
</a:options>
let $archive0 := a:create(
  ("foo.xml"),
  ($foo-content),
  $options
)
let $archive1 := a:delete($archive0, "foo.xml")
let $archive2 := a:update($archive1, "foo2.xml", $foo2-content)
return (
  count(a:entries($archive2)),
  a:extract-text($archive2, "foo2.xml")
)
