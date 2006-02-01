/* $Id$ */

load("../web/lib/msgslib.ssjs");

template.title="Choosing Message Subs in Group: "+msg_area.grp[grp].description;

if(do_header)
write_template("header.inc");

template.group=msg_area.grp[grp];
template.subs=new Array;

for(s in msg_area.grp[grp].sub_list) {
  ischecked = 1; /* New Message Scan Off */
  if(msg_area.grp[grp].sub_list[s].scan_cfg&SCAN_CFG_NEW)
    ischecked = 2;
  if(msg_area.grp[grp].sub_list[s].scan_cfg&SCAN_CFG_YONLY)
    ischecked = 3;
    var thissub=msg_area.grp[grp].sub_list[s];
    msgbase = new MsgBase(msg_area.grp[grp].sub_list[s].code);
    if(msgbase.open()) {
        var lastdate="N/A";
        msgs=msgbase.total_msgs;
        if(msgs != undefined && msgs > 0) {
            lastdate=msgbase.get_msg_index(true,msgs-1);
            if(lastdate!=undefined && lastdate != null) {
                lastdate=lastdate.time;
                if(lastdate>0)
                    lastdate=strftime("%b-%d-%y",lastdate);
            }
        }
        msgbase.close();
        thissub.messages=msgs;
        thissub.lastmsg=lastdate;
        thissub.ischecked=ischecked;
    }
    template.subs.push(thissub);
}

if(do_topnav)
	load("../web/lib/topnav_html.ssjs");
if(do_leftnav)
load("../web/lib/leftnav_html.ssjs");
if(do_rightnav)
	write_template("rightnav.inc");
write_template("msgs/choosesubs.inc");
if(do_footer)
	write_template("footer.inc");

msgs_done();
