/*
	UBER BLOX!
	A javascript block puzzle game similar to GameHouse "Super Collapse" 
	by Matt Johnson (2009)

	for Synchronet v3.15+
*/

load("json-client.js");
var root = js.exec_dir;

if(!file_exists(root + "server.ini")) {
	throw("server initialization file missing");
}

var server_file = new File(root + "server.ini");
server_file.open('r',true);
//var autoUpdate=server_file.iniGetValue(null,"autoUpdate");
var serverAddr=server_file.iniGetValue(null,"serverAddr");
var serverPort=server_file.iniGetValue(null,"serverPort");
server_file.close();

var client=new JSONClient(serverAddr,serverPort);

/*
if(autoUpdate) {
	console.putmsg("\r\n\1nChecking for updates...");
}
*/

load(root + "game.js");