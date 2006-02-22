/* $Id$ */

/* FTP link */

 if(user.number || system.matchuser("Guest")) {
    template.ftp_url="ftp://";
    if(user.number && !(user.security.restrictions&UFLAG_G))
            template.ftp_url=template.ftp_url + user.alias + ":" + user.security.password + "&#064;";

    var host = http_request.host;
    if(!host || !host.length)
            host = system.host_name;
    var port = host.indexOf(':');
    if(port>=0)
            host=host.slice(0,port);
    template.ftp_url += host; 
    if(ftp_port!=21)
        template.ftp_url += ftp_port;
    template.ftpidx ="/00index.html?$" + new Date().valueOf().toString(36);
    template.ftpqwk = "/" + system.qwk_id.toLowerCase() + ".qwk";
 }


template.leftnav=new Array;

if(do_extra) {
if(user.number==0 || user.security.restrictions&UFLAG_G)
    
    template.leftnav.push({html: '<li><a href="/login.ssjs">Login</a></li><li><a href="/newuser.ssjs">New User</a></li>' });
else
    template.leftnav.push({html: '<li><a href="/members/userlist.ssjs">User Listing</a></li><li><a href="/members/lastcallers.ssjs">Last Callers</a></li><li><a href="/members/info.ssjs">Information</a></li><li><a href="/members/themes.ssjs">Change Theme</a></li><li><a href="/members/newpw.ssjs">Change Password</a></li><li><a href="/msgs/msgs.ssjs?msg_sub=mail">E-mail</a></li>' });  

if(user.number || (this.login!=undefined && system.matchuser("Guest")))
    template.leftnav.push({html: '<li><a href="/msgs">Message Groups</a></li>' });

if( sub != 'mail' && (http_request.virtual_path == '/msgs/msg.ssjs' || http_request.virtual_path == '/msgs/msgs.ssjs'  || http_request.virtual_path == '/msgs/post.ssjs' || http_request.virtual_path == '/msgs/reply.ssjs' || http_request.virtual_path == '/msgs/savemsg.ssjs' || http_request.virtual_path == '/msgs/subinfo.ssjs' || http_request.virtual_path == '/msgs/subs.ssjs')) {
  for(s in msg_area.grp_list)
        template.leftnav.push({html: '<li style="font-size: 12px;"><a style="color: #68F;" href="/msgs/subs.ssjs?msg_grp=' + msg_area.grp_list[s].name + '" onmouseover="this.style.color=\'white\'" onmouseout="this.style.color=\'#68F\'">' + msg_area.grp_list[s].description + '</a></li>' });
}
if(user.number==0 || user.security.restrictions&UFLAG_G) {
    }
else
    template.leftnav.push({html: '<li><a href="/msgs/choosegroup.ssjs">Set Message Scan</a></li>' });
if( sub != 'mail' && (http_request.virtual_path == '/msgs/choosesubs.ssjs' || http_request.virtual_path == '/msgs/updatesubs.ssjs')) {
    for(s in msg_area.grp_list)
            template.leftnav.push({html: '<li style="font-size: 12px;"><a style="color: #68F;" href="/msgs/choosesubs.ssjs?msg_grp=' + msg_area.grp_list[s].name + '" onmouseover="this.style.color=\'white\'" onmouseout="this.style.color=\'#68F\'">' + msg_area.grp_list[s].description + '</a></li>' });
}
if(user.number==0 || user.security.restrictions&UFLAG_G) {
    }
else
    if(doQWK)
    template.leftnav.push({ html: '<li><a href="' + template.ftp_url + template.ftpqwk + '">Download QWK</a></li>' });
	
} else {

	if(user.number==0 || user.security.restrictions&UFLAG_G)
	    template.leftnav.push({html: '<a href="/login.ssjs">Login</a><a href="/newuser.ssjs">New User</a>' });
	else
	    template.leftnav.push({html: '<a href="/members/userlist.ssjs">User Listing</a><a href="/members/lastcallers.ssjs">Last Callers</a><a href="/members/info.ssjs">Information</a><a href="/members/themes.ssjs">Change Theme</a><a href="/members/newpw.ssjs">Change Password</a><a href="/msgs/msgs.ssjs?msg_sub=mail">E-mail</a>' });  
	if(user.number || (this.login!=undefined && system.matchuser("Guest")))
	    template.leftnav.push({html: '<a href="/msgs">Message Groups</a>' });
	if( sub != 'mail' && (http_request.virtual_path == '/msgs/msg.ssjs' || http_request.virtual_path == '/msgs/msgs.ssjs'  || http_request.virtual_path == '/msgs/post.ssjs' || http_request.virtual_path == '/msgs/reply.ssjs' || http_request.virtual_path == '/msgs/savemsg.ssjs' || http_request.virtual_path == '/msgs/subinfo.ssjs' || http_request.virtual_path == '/msgs/subs.ssjs')) {
	  template.leftnav.push({ html: '<div id="sectionSubLinks">' });
	  for(s in msg_area.grp_list)
	        template.leftnav.push({html: '<a href="/msgs/subs.ssjs?msg_grp=' + msg_area.grp_list[s].name + '">' + msg_area.grp_list[s].description + '</a>' });
	  template.leftnav.push({ html: '</div>' });
	}
	if(user.number==0 || user.security.restrictions&UFLAG_G) {
	    }
	else
	    template.leftnav.push({html: '<a href="/msgs/choosegroup.ssjs">Set Message Scan</a>' });
	if( sub != 'mail' && (http_request.virtual_path == '/msgs/choosesubs.ssjs' || http_request.virtual_path == '/msgs/updatesubs.ssjs')) {
	    template.leftnav.push({ html: '<div id="sectionSubLinks">' });
	    for(s in msg_area.grp_list)
	            template.leftnav.push({html: '<a href="/msgs/choosesubs.ssjs?msg_grp=' + msg_area.grp_list[s].name + '">' + msg_area.grp_list[s].description + '</a>' });
	    template.leftnav.push({ html: '</div>' });
	}
if(user.number==0 || user.security.restrictions&UFLAG_G) {
	    }
	else
	    if(doQWK)
	    template.leftnav.push({ html: '<a href="' + template.ftp_url + template.ftpqwk + '">Download QWK Packet</a>' });
}
write_template("leftnav.inc");
