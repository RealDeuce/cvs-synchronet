/* $Id: nodelist.ssjs,v 1.7 2006/02/01 00:08:01 runemaster Exp $ */

load("../web/lib/template.ssjs");

template.title= system.name + " - Who's Online";

var sub='';

if(do_header)
	write_template("header.inc");
if(do_topnav)
	load("../web/lib/topnav_html.ssjs");
if(do_leftnav)
load("../web/lib/leftnav_html.ssjs");
if(do_rightnav)
	write_template("rightnav.inc");
write_template("nodelist.inc");
if(do_footer)
	write_template("footer.inc");

