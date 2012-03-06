//$Id: maze.js,v 1.8 2012/03/06 22:13:05 mcmlxxix Exp $
/*
	SYNCHRONET MAZE RACE 
	A Javascript remake 
	of the Atari game "Maze Craze"

	For Synchronet v3.15+
	Matt Johnson(2008)
*/
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

load(root + "game.js");