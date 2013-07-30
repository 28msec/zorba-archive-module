import module namespace a = "http://zorba.io/modules/archive";

let $fake_archive := xs:base64Binary("5Pb8")
return 
  a:options($fake_archive)
