/*
 * http://spamassassin.apache.org/full/3.0.x/dist/spamd/PROTOCOL
 * $Id$
 */

load("sockdefs.js")

function SPAMC_Message(messagefile, addr, port, user)
{
	if(!file_exists(messagefile))
		return(false);
	this.addr=addr;
	if(this.addr==undefined)
		this.addr='127.0.0.1';
	this.port=port;
	if(this.port==undefined)
		this.port='783';
	this.user=user;
	this.messagefile=messagefile;
	this.DoCommand=Message_DoCommand;
	this.check =function() { return(this.DoCommand('CHECK')); };
	this.symbols =function() { return(this.DoCommand('SYMBOLS')); };
	this.report =function() { return(this.DoCommand('REPORT')); };
	this.report_ifspam =function() { return(this.DoCommand('REPORT_IFSPAM')); };
	this.process =function() { return(this.DoCommand('PROCESS')); };
}

function Message_DoCommand(command)
{
	var rcvd=new Array();
	var tmp;
	var sock=new Socket(SOCK_STREAM, "spamc");
	var ret=new Object();
	ret.message='';

	if(!sock.connect(this.addr, this.port)) {
		log("ERROR: spamc.js failed to connect!");
		return(false);
	}
	sock.write(command.toUpperCase()+" SPAMC/1.2\r\n");
	sock.write("Content-length: "+file_size(this.messagefile)+"\r\n");
	if(this.user)	// Optional
		sock.write("User: " + this.user + "\r\n");
	sock.write("\r\n");
	sock.sendfile(this.messagefile);
	sock.is_writeable=false;
	while(1) {
		tmp=sock.recvline();
		if(tmp==undefined || tmp=='')
			break;
		rcvd.push(tmp);
	}
	if(sock.is_connected) {
		// Read the body
		while(1) {
			tmp=sock.recv();
			if(tmp==undefined || tmp=='')
				break;
			ret.message += tmp;
		}
	}
	if(rcvd.length < 1) {
		log("ERROR: No lines read from spamd");
		return(false)
	}
	var tmp=rcvd[0].split(/\s+/,3);
	if(tmp.length < 3) {
		log("ERROR: Unable to parse line '"+rcvd[0]);
		return(false)
	}
	if(tmp[1] != 0) {
		log("ERROR: spamd returned error "+tmp[2]+" ("+tmp[1]+")");
		return(false)
	}

	/* Parse headers */
	for(line=1; line<rcvd.length; line++) {
		if(rcvd[line]=='')
			break;
		tmp=rcvd[line].split(/\s+/);
		if(tmp[0].toUpperCase() == 'SPAM:') {
			if(tmp[1].toLowerCase() == 'true')
				ret.isSpam=true;
			else
				ret.isSpam=false;
			if(tmp[2] == ';') {
				ret.score=parseInt(tmp[3]);
				if(tmp[4] == '/')
					ret.threshold=parseInt(tmp[5]);
			}
		}
	}

	if(command == 'SYMBOLS') {
		if(ret!==false) {
			ret.message=ret.message.replace(/[\r\n]/g,'');
			ret.symbols=ret.message.split(/,/);
			ret.message='';
		}
	}

	return(ret);
}
