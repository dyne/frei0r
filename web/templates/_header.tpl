<!doctype html>
<html class="{$page_class}">
<head>
  <title> :: d y n e . o r g :: {$page_title}</title>

  <link rel="stylesheet" type="text/css" href="stylesheets/dyne5.css" media="all" title="Dyne" />
  <link rel="stylesheet" type="text/css" href="stylesheets/org-mode.css" />
  <link rel="stylesheet" type="text/css" href="stylesheets/org-htmlize-src.css" />

{if isset($stylesheet) && $stylesheet neq ""}
  <link rel="stylesheet" type="text/css" href="stylesheets/{$stylesheet}" />
{/if}

  <link rel="icon"       type="image/png" href="{$moebius_png}" />
  <link rel="alternate"  type="application/rss+xml" title="RSS 2.0" href="http://feeds.dyne.org/planet_dyne" />
  <link rel="alternate"  type="application/ical+xml" title="Ical 1.0" href="http://dyne.org/events/dyne.ics" />

  <link rel="about"      type="text/html" href="http://dyne.org/dyne_foundation" />
  <link rel="code"       type="text/html" href="http://code.dyne.org" />
  <link rel="culture"    type="text/html" href="http://dyne.org/culture" />
  <link rel="discussion" type="text/html" href="http://lists.dyne.org" />
  <link rel="donate"     type="text/html" href="http://dyne.org/donate" />
  <link rel="events"     type="text/html" href="http://dyne.org/events" />
  <link rel="software"   type="text/html" href="http://dyne.org/software" />

  <meta http-equiv="Content-Type" content="text/html;charset=utf-8" />
{if isset($description) && $description neq ""}
  <meta name="Description" content="{$description}" />
{else}
  <meta name="Description" content="Since 2000 dyne.org is an atelier for digital artisans. We design free software and ideas for the arts, sharing a grassroot access to technology, education and freedom of creation." />
{/if}
  <meta name="Generator" content="Weaver Birds" />
{if isset($keywords) && $keywords neq ""}
  <meta name="Keywords" content="{$keywords}" />
{else}
  <meta name="Keywords" content="free, software, art, source, open, creativity, code, gnu, linux, bsd, streaming, audio, video, development, seminar, performance, theatre, music, hacktivism, gpl, copyleft, multimedia, ict4d, documentation, community" />
{/if}
  <meta name="MSSmartTagsPreventParsing" content="True" /><!-- IE sucks. Why do I have to opt-out from their stupid hack? -->
  <!-- HTML5 support for MSIE hahahaha http://remysharp.com/2009/01/07/html5-enabling-script/ -->
  <!--[if IE]>
  <script src="http://html5shiv.googlecode.com/svn/trunk/html5.js"></script>
  <![endif]-->
</head>
<body>
<header>
  <nav><ul id="dyne_links">
    <li><a href="http://dyne.org/">dyne</a> //</li>
    <li><a href="http://dyne.org/software">software</a></li>
    <li>:: <a href="http://dyne.org/culture">culture</a></li>
    <li>:: <a href="http://dyne.org/events">events</a></li>
    <li>:: <a href="http://planet.dyne.org/">news</a></li>
    <li>:: <a href="http://lists.dyne.org/">discussion</a></li>
    <li>:: <a href="http://museum.dyne.org/">museum</a></li>
    <li>\\ <a href="http://freaknet.org/">freaknet</a></li>
  </ul></nav>
  <hgroup>{$page_hgroup}</hgroup>
</header>
