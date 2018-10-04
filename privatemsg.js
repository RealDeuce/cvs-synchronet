// privatemsg.js

// Private Message (Ctrl-P) Hot Key Handler

// $Id: privatemsg.js,v 1.2 2018/10/04 06:23:50 rswindell Exp $

// Install this module in SCFG->External Programs->Global Hot Key Events:
// Global Hot Key             Ctrl-P        
// Command Line               ?privatemsg.js
//
// or, by running "jsexec privatemsg.js install"

if(argc == 1 && argv[0] == "install") {
	var cnflib = load({}, "cnflib.js");
	var xtrn_cnf = cnflib.read("xtrn.cnf");
	if(!xtrn_cnf) {
		alert("Failed to read xtrn.cnf");
		exit(-1);
	}
	xtrn_cnf.hotkey.push({ key: /* Ctrl-P */16, cmd: '?privatemsg.js' });
	
	if(!cnflib.write("xtrn.cnf", undefined, xtrn_cnf)) {
		alert("Failed to write xtrn.cnf");
		exit(-1);
	}

	exit(0);
}

require("sbbsdefs.js", 'SS_USERON');
require("text.js", 'PrivateMsgPrompt');

bbs.replace_text(PrivateMsgPrompt, 
    "\r\n\1b\1hPrivate: \1g~T\1n\1gelegram, " +
	"\1h~M\1n\1gessage, \1h~C\1n\1ghat, \1h~I\1n\1gnter-BBS, or \1h~Q\1n\1guit: \1c\1h");

if(!(bbs.sys_status&SS_USERON))
	exit();

var saved_node_action = bbs.node_action;

outer_loop:
while(bbs.online && !(console.aborted)) {
	if(user.reststrictions&UFLAG_C) {
		console.print(bbs.text(R_SendMessages));
		break; 
	}
	bbs.nodesync();
	console.mnemonics(bbs.text(PrivateMsgPrompt));
	bbs.sys_status&=~SS_ABORT;
	var ch;
	while(bbs.online && !console.aborted) {  /* Watch for incoming messages */
		ch=console.inkey(/* mode: */K_UPPER, /* timeout: */1000);
		if(ch && "TMCIQ\r".indexOf(ch)>=0)
			break;
		
		console.line_counter = 0;
		bbs.nodesync();
		if(console.line_counter)	// Something was displayed, so redisplay our prompt
			continue outer_loop;
	}

	console.line_counter = 0;
	switch(ch) {
		case '':
			continue;
		case 'T':   /* Telegram */
			console.print("Telegram\r\n");
			bbs.put_telegram();
			break;
		case 'M':   /* Message */
			console.print("Message\r\n");
			bbs.put_node_message();
			break;
		case 'C':   /* Chat */
			console.print("Chat\r\n");
			bbs.private_chat();
			bbs.node_action = saved_node_action;
			break;
		case 'I':	/* Inter-BBS */
			console.print("Inter-BBS\r\n");
			load({}, 'sbbsimsg.js');
			break;
		default:
			console.print("Quit\r\n");
			exit();
	}
}

