import module namespace a = "http://zorba.io/modules/archive";

let $foo-content := "<foo/>"
let $bar-content := xs:base64Binary("YWJj")
let $foo2-content := "<foo2/>"
let $options := 
{
  "format" : "TAR",
  "compression" : "GZIP"
}
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
