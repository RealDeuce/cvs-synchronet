// fingerservice.js

// Synchronet Service for the Finger protocol (RFC 1288)

// Example configuration (in ctrl/services.cfg):

// Finger		79	0-unlimited	0		fingerservice.js

// Add -n to the configuration line to eliminate user age and gender
// information from the query results.

// !WARNING!
// Finger is an open protocol utilizing no forms of authorization
// or authentication. FINGER IS A KNOWN AND ACCEPTED SECURITY RISK. 
// Detailed information about your system and its users is made 
// available to ANYONE using this service. If there is anything in
// this script that you do not want to be made available to anyone
// and everyone, please comment-out (using /* and */) that portion
// of the script.

const VERSION="Synchronet Finger Service Version 1.00a";

var include_age_gender=true;

load("nodedefs.js");

for(i=0;i<argc;i++)
	if(argv[i].toLowerCase()=="-n")
		include_age_gender = false;

// Write a string to the client socket
function write(str)
{
	client.socket.send(str);
}

// Write a crlf terminated string to the client socket
function writeln(str)
{
	write(str + "\r\n");
}

// Send the contents of a text file to the client socket
function send_file(fname)
{
	f = new File(fname);
	if(!f.open("r")) 
		return;
	txt = f.readAll();
	f.close();
	for(l in txt)
		writeln(txt[l]);
}

// Returns true if a connection on the local 'port' was succesful
function test_port(port)
{
	sock = new Socket();
	success = sock.connect("localhost",port);
	sock.close();

	return(success);
}

// Get Finger Request (the main entry point) 
request = client.socket.recvline(128 /*maxlen*/, 10 /*timeout*/);

if(request==null) {
	log("!TIMEOUT waiting for request");
	exit();
}

log(format("client request: '%s'",request));

if(request.substr(0,2).toUpperCase()=="/W")	// "higher level of verbosity"
	request=request.slice(2);				// ignored...

while(request.charAt(0)==' ')	// skip prepended spaces
	request=request.slice(1);

if(request=="") {	// no specific user requested, give list of active users
	log("client requested active user list");
	write(format("%-25.25s %-40.40s %7s Node\r\n"
		,"User","Action",include_age_gender ? "Age Sex":""));
	var dashes="----------------------------------------";
	write(format("%-25.25s %-40.40s %3.3s %3.3s %4.4s\r\n"
		,dashes,dashes
		,include_age_gender ? dashes : ""
		,include_age_gender ? dashes : ""
		,dashes));
	var user = new User(1);
	for(n=0;n<system.node_list.length;n++) {
		if(system.node_list[n].status!=NODE_INUSE)
			continue;
		user.number=system.node_list[n].useron;
		var action=format(NodeAction[system.node_list[n].action]
			,system.node_list[n].aux);
		write(format("%-25.25s %-40.40s %3s %3s %4d\r\n"
			,user.alias
			,action
			,include_age_gender ? user.age.toString() : ""
			,include_age_gender ? user.gender : ""
			,n+1
			));
	}
	exit();
}

if(request.charAt(0)=='?') {	// Handle "special" requests
	request=request.slice(1);
	switch(request.toLowerCase()) {

		case "ver":
			writeln(VERSION);
			writeln(server.version);
			writeln(system.version_notice + system.revision);
			writeln("Compiled " + system.compiled_when + " with " + system.compiled_with);
			writeln(system.js_version);
			writeln(system.os_version);
			break;

		case "time":
			writeln(system.timestr() + " " + system.zonestr());
			break;

		case "logon.lst":
			send_file(system.data_dir + "logon.lst");
			break;

		case "auto.msg":
			send_file(system.data_dir + "msgs/auto.msg");
			break;

		case "stats":	/* Statistics */
			for(i in system.stats)
				writeln(i + " = " + system.stats[i]);

			total	= time()-system.uptime;
			days	= Math.floor(total/(24*60*60));
		    if(days) 
				total%=(24*60*60);
			hours	= Math.floor(total/(60*60));
			min		= (Math.floor(total/60))%60;
			sec		= total%60;

			writeln(format("uptime = %u days, %u hours, %u minutes and %u seconds"
				,days,hours,min,sec));
			break;

		case "nodelist":
			var user = new User(1);
			for(n=0;n<system.node_list.length;n++) {
				write(format("Node %2d ",n+1));
				if(system.node_list[n].status==NODE_INUSE) {
					user.number=system.node_list[n].useron;
					write(format("%s (%u %s) ", user.alias, user.age, user.gender));
					write(format(NodeAction[system.node_list[n].action],system.node_list[n].aux));
				} else
					write(format(NodeStatus[system.node_list[n].status],system.node_list[n].aux));

				write("\r\n");
			}			
			break;

		case "services":	/* Services running on this host */
			if(test_port(23))
				writeln("Telnet");
			if(test_port(513))
				writeln("RLogin");
			if(test_port(21))
				writeln("FTP");
			if(test_port(25))
				writeln("SMTP");
			if(test_port(110))
				writeln("POP3");
			if(test_port(119))
				writeln("NNTP");
			if(test_port(70))
				writeln("Gopher");
			if(test_port(80))
				writeln("HTTP");
			if(test_port(113))
				writeln("IDENT");
			if(test_port(6667))
				writeln("IRC");
			break;

		default:
			writeln("Supported special requests:");
			writeln("\tver");
			writeln("\ttime");
			writeln("\tstats");
			writeln("\tservices");
			writeln("\tnodelist");
			writeln("\tauto.msg");
			writeln("\tlogon.lst");
			log(format("!UNSUPPORTED SPECIAL REQUEST: '%s'",request));
			break;
	}
	exit();
}

// User info is handled here

var usernum=Number(request);
if(!usernum) {
	var at = request.indexOf('@');
	if(at>0)
		request = request.substr(0,at-1);

	usernum = system.matchuser(system.alias(request));
	if(!usernum) {
		log(format("!UNKNOWN USER: '%s'",request));
		exit();
	}
}
var user = new User(usernum);
if(user == null) {
	log(format("!INVALID USER NUMBER: %d",usernum));
	exit();
}

uname = format("%s #%d",user.alias,user.number);
write(format("User: %-30s In real life: %s\r\n"
	  ,uname,user.name));
write(format("From: %s\r\n",user.location));
if(include_age_gender) {
	birth=format("Birth: %s (Age: %u years)"
		  ,user.birthdate,user.age);
	write(format("%-42s Gender: %s\r\n"
		  ,birth,user.gender));
}
write(format("Shell: %-34s  Editor: %s\r\n"
	  ,user.command_shell,user.editor));
write(format("Last login %s %s\r\nvia %s from %s [%s]\r\n"
	  ,system.timestr(user.stats.laston_date)
	  ,system.zonestr()
	  ,user.connection
	  ,user.host_name
	  ,user.ip_address));

/* End of fingerservice.js */
