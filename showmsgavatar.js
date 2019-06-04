// $Id: showmsgavatar.js,v 1.6 2019/06/04 05:02:08 rswindell Exp $

// This can be loaded from text/menu/msghdr.asc via @EXEC:SHOWMSGAVATAR@
// Don't forget to include or exclude the blank line after if you do
// (or don't) want a blank line separating message headers and body text

// This script may be used instead of (not in addition to) showmsghdr.js
// Use this script if you have/want a custom message header defined in
// msghdr.asc, i.e. with @-codes (not using text.dat strings).
// If you do not want the avatar right-justified, copy this file to your
// mods directory and change that parameter below.

require("smbdefs.js", 'MSG_ANONYMOUS');
require("userdefs.js", 'USER_ANSI');

function draw_default_avatar(sub)
{
	var options = bbs.mods.avatars_options;
	if(!options) {
		options = load({}, "modopts.js", "avatars");
		if(!options)
			options = { cached: true };
		bbs.mods.avatars_options = options;	// cache the options
	}
	var avatar = options[sub + "_default"];
	if(!avatar)
		avatar = options[msg_area.sub[sub].grp_name.toLowerCase() + "_default"];
	if(!avatar)
		avatar = options.sub_default;
	if(avatar)
		Avatar.draw_bin(avatar, /* above: */true, /* right-justified: */true, bbs.msghdr_top_of_screen);
}

// Avatar support here:
if(!(bbs.msg_attr&MSG_ANONYMOUS) 
	&& (console.term_supports()&(USER_ANSI|USER_NO_EXASCII)) == USER_ANSI) {
	var Avatar = load({}, 'avatar_lib.js');
	var success = Avatar.draw(bbs.msg_from_ext, bbs.msg_from, bbs.msg_from_net, /* above: */true, /* right-justified: */true
		,bbs.msghdr_top_of_screen);
	if(!success) {
		draw_default_avatar(bbs.smb_sub_code);
	}
	console.attributes = 7;	// Clear the background attribute as the next line might scroll, filling with BG attribute
}
