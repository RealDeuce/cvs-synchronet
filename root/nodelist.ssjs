/* $Id: nodelist.ssjs,v 1.6 2005/03/24 19:15:08 runemaster Exp $ */

load("../web/lib/template.ssjs");

template.title= system.name + " - Who's Online";

var sub='';

write_template("header.inc");
load("../web/lib/topnav_html.ssjs");
load("../web/lib/leftnav_html.ssjs");
write_template("nodelist.inc");
write_template("footer.inc");
