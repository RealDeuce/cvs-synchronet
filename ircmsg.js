/* ircmsg.js */

/* $Id$ */

load("irclib.js");	// Thanks Cyan!

const REVISION = "$Revision$".split(' ')[1];

var server="irc.synchro.net";
var channel="#channel";
var port=6667;
var nick="nick";
var msg;
var join=false;
var passedmsg=0;

for(i=0;i<argc;i++) {
	switch(argv[i]) {
		case "-s":
			server=argv[++i];
			break;
		case "-j":
			join=true;
			break;
		case "-c":
			channel=argv[++i];
			break;
		case "-p":
			port=argv[++i];
			break;
		case "-n":
			nick=argv[++i];
			break;
		case "-m":
			msg=argv[++i];
			passedmsg=1;
			break;
	}
}

if(passedmsg && (msg==undefined || msg.search(/^[\r\n]*$/)!=-1)) {
	log("-m specified with blank message... aborting");
	exit();
}

log("Using nick: " + nick);
log("Connecting to: " +server+ " port " + port);
my_server = IRC_client_connect(server,nick,undefined,undefined,port);
if (!my_server) {
        log("!Couldn't connect to " + server);
        exit();
}

var done=0;
while(!done) {
	while(!done && (response=my_server.recvline())) {
		var resp=response.split(/\s+/);
		if(resp[1]=='433') {
			/* Nick in use... */
			nick+='_';
			my_server.send("NICK " + nick + "\r\n");
			
		}
		if(resp[1]=='422' || resp[1]=='376')
			done=1;
		log(response);
	}
}

if(join) {
	log("Joining: " + channel);
	my_server.send("JOIN " + channel + "\r\n");
	while(my_server.poll(5) && (response=my_server.recvline()))
		log(response);
}

if(msg) {
	msg=expand_tabs(msg);
	send(msg);
}
else while(msg=readln()) {
	msg=expand_tabs(msg);
	send(msg);
}

while(my_server.poll(0) && (response=my_server.recvline()))
	log(response);

IRC_quit(my_server);

function send(msg)
{
	if(msg==undefined || msg.search(/^[\r\n]*$/)!=-1) {
		log("Not sending blank message");
		return;
	}
	log("Sending: " + msg);
	if(!my_server.send("PRIVMSG "+channel+" :"+msg+"\r\n"))
		alert("send failure");
}

function expand_tabs(msg)
{
	if(msg!=undefined) {
		var s=msg;
		do {
			msg=s;
			s=msg.replace(/\t/,
				function(str,offset,s) {
					return('        '.substr(0,8 - offset % 8));
			});
		} while(s!=msg);
	}
	return(msg);
}
