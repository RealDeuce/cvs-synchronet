/* $Id: choosegroup.ssjs,v 1.1 2005/03/24 15:16:12 runemaster Exp $ */ 

load("../web/lib/msgslib.ssjs");

template.title="Message Groups on " +system.name;
write_template("header.inc");
load("../web/lib/topnav_html.ssjs");
write_template("leftnav.inc");
template.groups=msg_area.grp_list;
write_template("msgs/choosegroup.inc");
write_template("footer.inc");

msgs_done();
