/* $Id: nodelist.ssjs,v 1.4 2005/03/08 01:10:53 runemaster Exp $ */

load("../web/lib/template.ssjs");

template.title= system.name + " - Who's Online";

var sub='';

write_template("header.inc");
write_template("topnav.inc");
write_template("leftnav.inc");
write_template("nodelist.inc");
write_template("footer.inc");
