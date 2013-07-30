import module namespace a = "http://zorba.io/modules/archive";
import module namespace b = "http://zorba.io/modules/base64";

let $fake_archive := xs:base64Binary("YWJj")
let $new-archive := a:update($fake_archive, "foo2.xml", "<foo2/>")
return (count(a:entries($new-archive)), a:extract-text($new-archive, "foo2.xml"))

