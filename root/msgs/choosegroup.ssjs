/* $Id: choosegroup.ssjs,v 1.2 2005/03/24 19:15:05 runemaster Exp $ */ 

load("../web/lib/msgslib.ssjs");

template.title="Message Groups on " +system.name;
write_template("header.inc");
load("../web/lib/topnav_html.ssjs");
load("../web/lib/leftnav_html.ssjs");
template.groups=msg_area.grp_list;
write_template("msgs/choosegroup.inc");
write_template("footer.inc");

msgs_done();
