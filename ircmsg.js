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
			break;
	}
}

log("Using nick: " + nick);
log("Connecting to: " +server+ " port " + port);
my_server = IRC_client_connect(server,nick,undefined,undefined,port);
if (!my_server) {
        log("!Couldn't connect to " + server);
        exit();
}
while(my_server.poll(5) && (response=my_server.recvline()))
	log(response);

if(join) {
	log("Joining: " + channel);
	my_server.send("JOIN " + channel + "\r\n");
	while(my_server.poll(5) && (response=my_server.recvline()))
		log(response);
}
if(msg)
	send(msg);
else while(msg=readln())
	send(msg);

while(my_server.poll(5) && (response=my_server.recvline()))
	log(response);

IRC_quit(my_server);

function send(msg)
{
	log("Sending: " + msg);
	if(!my_server.send("PRIVMSG "+channel+" :"+msg+"\r\n"))
		alert("send failure");
}
