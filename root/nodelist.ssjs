/* $Id: nodelist.ssjs,v 1.5 2005/03/12 15:50:30 runemaster Exp $ */

load("../web/lib/template.ssjs");

template.title= system.name + " - Who's Online";

var sub='';

write_template("header.inc");
load("../web/lib/topnav_html.ssjs");
write_template("leftnav.inc");
write_template("nodelist.inc");
write_template("footer.inc");
