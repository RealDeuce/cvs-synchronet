/* $Id: leftnav_html.ssjs,v 1.3 2005/03/30 01:50:14 runemaster Exp $ */

template.leftnav=new Array;

if(user.number==0 || user.security.restrictions&UFLAG_G)
    template.leftnav.push({html: '<a href="/login.ssjs">Login</a><a href="/newuser.ssjs">New User</a>' });
else
    template.leftnav.push({html: '<a href="/members/userlist.ssjs">User Listing</a><a href="/members/info.ssjs">Information</a><a href="/members/themes.ssjs">Change Theme</a><a href="/members/newpw.ssjs">Change Password</a><a href="/msgs/msgs.ssjs?msg_sub=mail">E-mail</a>' });  

if(user.number || (this.login!=undefined && system.matchuser("Guest")))
    template.leftnav.push({html: '<a href="/msgs">Message Groups</a>' });

if( sub != 'mail' && (http_request.virtual_path == '/msgs/choosegroup.ssjs' || http_request.virtual_path == '/msgs/choosesub.ssjs' || http_request.virtual_path == '/msgs/msg.ssjs' || http_request.virtual_path == '/msgs/msgs.ssjs'  || http_request.virtual_path == '/msgs/post.ssjs' || http_request.virtual_path == '/msgs/reply.ssjs' || http_request.virtual_path == '/msgs/savemsg.ssjs' || http_request.virtual_path == '/msgs/subinfo.ssjs' || http_request.virtual_path == '/msgs/subs.ssjs' || http_request.virtual_path == '/msgs/updatesubs.ssjs')) {
  template.leftnav.push({ html: '<span id="sectionSubLinks">' });
  for(s in msg_area.grp_list)
        template.leftnav.push({html: '<a href="/msgs/subs.ssjs?msg_grp=' + msg_area.grp_list[s].description + '">' + msg_area.grp_list[s].description + '</a>' });
  template.leftnav.push({ html: '</span>' });
}
      
if(user.number==0 || user.security.restrictions&UFLAG_G) {
    }
else
    template.leftnav.push({html: '<a href="/msgs/choosegroup.ssjs">Set Message Scan</a>' });
if(user.number==0 || user.security.restrictions&UFLAG_G) {
    }
else
    template.leftnav.push({ html: '<a href="' + template.ftp_url + template.ftpqwk + '">Download QWK Packet</a>' });
        
write_template("leftnav.inc");