/* $Id: choosegroup.ssjs,v 1.3 2006/02/01 00:08:35 runemaster Exp $ */ 

load("../web/lib/msgslib.ssjs");

template.title="Message Groups on " +system.name;

if(do_header)
	write_template("header.inc");
if(do_topnav)
	load("../web/lib/topnav_html.ssjs");
if(do_leftnav)
load("../web/lib/leftnav_html.ssjs");
if(do_rightnav)
	write_template("rightnav.inc");
	
template.groups=msg_area.grp_list;
	
write_template("msgs/choosegroup.inc");
if(do_footer)
	write_template("footer.inc");

msgs_done();
