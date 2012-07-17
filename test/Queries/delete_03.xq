import module namespace a = "http://www.expath.org/ns/archive";

let $foo-content := "<foo/>"
let $bar-content := xs:base64Binary("YWJj")
let $options := 
<a:options>
 <a:format value="TAR"/>
 <a:algorithm value="GZIP"/>
</a:options>
let $archive0 := a:create(
  ("foo.xml"),
  ($foo-content),
  $options
)
let $archive1 := a:delete($archive0, "foo.xml")
return count(a:entries($archive1))
