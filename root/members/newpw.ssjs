/* $Id: newpw.ssjs,v 1.3 2005/03/24 19:15:04 runemaster Exp $ */

var sub="";

load("sbbsdefs.js");
load("../web/lib/template.ssjs");

template.title = "Change User Password";

template.pwchangedate=strftime("%A, %B %d, %Y." ,user.security.password_date);

write_template("header.inc");
load("../web/lib/topnav_html.ssjs");
load("../web/lib/leftnav_html.ssjs");
write_template("newpw.inc");
write_template("footer.inc");