import module namespace a = "http://zorba.io/modules/archive";

let $foo-content := "<foo/>"
let $bar-content := xs:base64Binary("YWJj")
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
return count(a:entries($archive1))
