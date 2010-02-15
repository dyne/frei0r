<?php

require_once("config.php");


$smarty->assign("page_class",  "software");
$smarty->assign("page_hgroup", "<h1>Frei0r</h1>");
$smarty->assign("page_title",  "free video effect plugins");

$smarty->display("header.tpl");

// sidebar
$smarty->display("doctypes.tpl");
$smarty->display("toc.tpl");

// page content
$smarty->display("body.tpl");


$smarty->display("footer.tpl");
?>

</body>
</html>