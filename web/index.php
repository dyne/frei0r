<?php

/* TO CHANGE WEBSITE CONTENTS DON'T EDIT THIS FILE
   instead you should be modifying index.org
   and in general all *.org files (see orgmode.org)
   these files are then rendered serverside
   
   THIS PHP FILE CONTAINS NO RELEVANT CONTENT */

require_once("helpers.inc.php");

define("DYNE_DEBUG_RENDERING_TIME", false);

/* Smarty template class configuration */
if (!defined('SMARTY_DIR')) {
    define("SMARTY_DIR", "/usr/share/php/smarty/libs/"); }
if (!is_dir(constant("SMARTY_DIR")) || !require_once("smarty/Smarty.class.php")) {
    echo "SMARTY is supposed to be installed in " . constant("SMARTY_DIR") . " but is not.";
    echo "Install it or edit SMARTY_DIR in " . __FILE__;
    exit;
}


global $smarty;
$smarty = new Smarty;
$smarty->compile_check = true; 
$smarty->debugging     = false;
$smarty->caching       = 0;

$smarty->cache_dir     = "cache";
$smarty->template_dir  = "templates";
$smarty->compile_dir   = "templates_c";
$smarty->plugins_dir   = array('/usr/share/php/smarty/plugins');


$smarty->assign("page_class",  "software org-mode");
$smarty->assign("page_hgroup", "<h1>Frei0r</h1>");
$smarty->assign("page_title",  "free video effect plugins");

$smarty->assign("pagename","software");
$smarty->display("_header.tpl");

// sidebar
div("menu");
$smarty->display("software/doctypes.tpl");
showfile("index-toc.html");
echo("</div>");

// page content
showfile("index-body.html");

$smarty->display("_footer.tpl");

?>
