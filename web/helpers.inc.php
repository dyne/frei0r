<?php
  // Return the full path to a file under web DocumentRoot
  // * DO NOT PASS USER INPUT TO THAT FUNCTION *
  // as it could result in displaying arbitrary files.
  function www_path($path = "")
  {
    if (empty($path)) return DYNE_WWW;
    else              return DYNE_WWW . "/" . $path;
  }

  function www_contents($path)
  {
    return file_get_contents(www_path($path));
  }

  function display_file($filename, $prefix = "", $suffix = "")
  {
    if (($contents = www_contents($filename)))
      echo $prefix . $contents . $suffix;
  }

///// auxiliary functions
function showfile($f) {
  $fd = fopen("$f","r");
  if(!$fd) { $text = "<h1>ERROR: $f not found</h1>";
 } else {	 
    $st = fstat($fd);
    $text = fread($fd,$st["size"]); fclose($fd);
  }
  echo($text);
}

function stripslashes_array($value) {
   $value = is_array($value) ?
               array_map('stripslashes_array', $value) :
               stripslashes($value);

   return $value;
}

function div($str) { echo("\n<div id=\"$str\">\n"); }
function section($str) { echo("\n<section id=\"$str\">\n"); }

?>
