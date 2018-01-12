// $Id: showfileavatar.js,v 1.1 2018/01/12 22:20:25 rswindell Exp $

const FM_ANON			=(1<<1);
const USER_ANSI         =(1<<1);

// Avatar support here:
if(!(bbs.file_attr&FM_ANON) && console.term_supports(USER_ANSI)) {
	var Avatar = load({}, 'avatar_lib.js');
	Avatar.draw(system.matchuser(bbs.file_uploader), null, null, /* above: */true, /* right-justified: */true);
	console.attributes = 0;	// Clear the background attribute as the next line might scroll, filling with BG attribute
}