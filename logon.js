// logon.js

// Synchronet v3.1 Default Logon Module

// $Id: logon.js,v 1.7 2006/05/01 06:37:53 rswindell Exp $

// @format.tab-size 4, @format.use-tabs true

load("sbbsdefs.js");

//Disable spinning cursor at pause prompts
//bbs.node_settings|=NM_NOPAUSESPIN	

if(user.security.restrictions&UFLAG_G) {
	while(bbs.online) {
		printf("\1y\1hFor our records, please enter your full name: \1w");
		name=console.getstr(25,K_UPRLWR);
		if(!name || !name.length)
			continue;
		bbs.log_str("Guest: " + name);
		user.name = name;
		break;
	}
	
	while(bbs.online) {
		printf("\1y\1hPlease enter your location (City, State): \1w");
		location=console.getstr(30,K_UPRLWR);
		if(!location || !location.length)
			continue;
		bbs.log_str("  " + location);
		user.location=location;
		break;
	}

	if(bbs.online)
		bbs.log_str("\r\n");
	while(bbs.online) {
		printf("\1y\1hWhere did you hear about this BBS?\r\n: \1w");
		ref=console.getstr(70);
		if(!ref || !ref.length)
			continue;
		bbs.log_str(ref + "\r\n");
		break;
	}
}


// Force split-screen chat on ANSI users
if(user.settings&USER_ANSI)
	user.chat_settings|=CHAT_SPLITP;

// Inactivity exemption
if(user.security.exemptions&UFLAG_H)
   	console.status|=CON_NO_INACT;

/******************************
 * Replaces the 2.1 Logon stuff
 ******************************/

// Logon screens

// Print logon screens based on security level
if(file_exists(system.text_dir + "menu/logon" + user.security.level + ".*"))
	bbs.menu("logon" + user.security.level);

// Print successively numbered logon screens (logon, logon1, logon2, etc.)
for(i=0;;i++) {
	fname="logon";
	if(i)
		fname+=i;
	if(!file_exists(system.text_dir + "menu/" + fname + ".*")) {
		if(i>1)
			break;
		continue;
	}
	bbs.menu(fname);
}

// Print one of text/menu/random*.*, picked at random
// e.g. random1.asc, random2.asc, random3.asc, etc.
var random_list = directory(system.text_dir + "menu/random*.*");
if(random_list.length)
	bbs.menu(file_getname(random_list[random(random_list.length)]).slice(0,-4));

console.clear();
bbs.user_event(EVENT_LOGON);

// Last few callers
console.aborted=false;
console.clear();
logonlst=system.data_dir + "logon.lst"
if(file_size(logonlst)<1)
	printf("\1n\1g\1hYou are the first caller of the day!\r\n");
else {
	printf("\1n\1g\1hLast few callers:\1n\r\n");
	console.printtail(logonlst,P_NOATCODES,4);
}
console.crlf();

// Auto-message
auto_msg=system.data_dir + "msgs/auto.msg"
if(file_size(auto_msg)>0) {
	console.printfile(auto_msg,P_NOATCODES);
	console.crlf();
}

// Automatically set shell to WIPSHELL
if(user.settings&USER_WIP)
	user.shell="WIPSHELL";


if(user.security.level==99				/* Sysop logging on */
   && !system.matchuser("guest")		/* Guest account does not yet exist */
   && user.security.flags4&UFLAG_G		/* Sysop has not asked to stop this question */
   ) {
	if(console.yesno("Create Guest/Anonymous user account (highly recommended)"))
		load("makeguest.js");
	else if(!console.yesno("Ask again later"))
		user.security.flags4&=~UFLAG_G;	/* Turn off flag 4G to not ask again */
	console.crlf();
}

// Change to "true" if you want your RLogin server to act as a door game server only
if(false	
	&& bbs.sys_status&SS_RLOGIN) {
	bbs.xtrn_sec();
	bbs.hangup();
}
