/*
	JAVASCRIPT TETRIS INTERBBS
	For Synchronet v3.15+
	Matt Johnson(2009)
*/
//$Id: tetris.js,v 1.10 2012/08/02 18:03:02 mcmlxxix Exp $
load("json-client.js");
var root = js.exec_dir;

if(!file_exists(root + "server.ini")) {
	throw("server initialization file missing");
}

var server_file = new File(root + "server.ini");
server_file.open('r',true);

//var autoUpdate=server_file.iniGetValue(null,"autoUpdate");
var serverAddr=server_file.iniGetValue(null,"host");
var serverPort=server_file.iniGetValue(null,"port");
server_file.close();

var client=new JSONClient(serverAddr,serverPort);

/*
if(autoUpdate) {
}
*/

load(root + "lobby.js");