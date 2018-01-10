// $Id: showmsgavatar.js,v 1.1 2018/01/10 23:37:33 rswindell Exp $

// This can be loaded from text/menu/msghdr.asc via @EXEC:SHOWMSGAVATAR@
// Don't forget to include or exclude the blank line after if you do
// (or don't) want a blank line separating message headers and body text

// This script may be used instead of (not in addition to) showmsghdr.js
// Use this script if you have/want a custom message header defined in
// msghdr.asc, i.e. with @-codes (not using text.dat strings).
// If you do not want the avatar right-justified, copy this file to your
// mods directory and change that parameter below.

load('smbdefs.js');
var   USER_ANSI         =(1<<1);

// Avatar support here:
if(!(bbs.msg_attr&MSG_ANONYMOUS) && console.term_supports(USER_ANSI)) {
	var Avatar = load({}, 'avatar_lib.js');
	Avatar.draw(bbs.msg_from_ext, bbs.msg_from, bbs.msg_from_net, /* above: */true, /* right-justified: */true);
	console.attributes = 0;	// Clear the background attribute as the next line might scroll, filling with BG attribute
}