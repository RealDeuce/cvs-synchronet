// nntpservice.js

// Synchronet Service for the Network News Transfer Protocol (RFC 977)

// $Id$

// Example configuration (in ctrl/services.cfg):

// NNTP		119	0-unlimited	0		nntpservice.js

// Tested clients:
//					Microsoft Outlook Express 6
//					Netscape Communicator 4.77
//					Xnews 5.04.25
//					Mozilla 1.1 (Requires -auto, and a prior login via other method)

const REVISION = "$Revision$".split(' ')[1];

var tearline = format("--- Synchronet %s%s-%s NNTP Service %s\r\n"
					  ,system.version,system.revision,system.platform,REVISION);
var tagline	=  format(" *  %s - %s - telnet://%s\r\n"
					  ,system.name,system.location,system.inetaddr);

load("sbbsdefs.js");
load("newsutil.js");

var debug = false;
var no_anonymous = false;
var auto_login = false;
var msgs_read = 0;
var msgs_posted = 0;
var slave = false;
var bogus_cmd_counter = 0;
var max_bogus_cmds = 10;
var filter_bogus_clients = false;
var include_mail = true;

// Parse arguments
for(i=0;i<argc;i++) {
	if(argv[i].toLowerCase()=="-d")
		debug = true;
	else if(argv[i].toLowerCase()=="-f")
		filter_bogus_clients = true;
	else if(argv[i].toLowerCase()=="-na")
		no_anonymous = true;
	else if(argv[i].toLowerCase()=="-mail")
		include_mail = true;
	else if(argv[i].toLowerCase()=="-auto") {
		no_anonymous = true;
		auto_login = true;
	}
}

// Write a string to the client socket
function write(str)
{
	client.socket.send(str);
}

function writeln(str)
{
	if(debug)
		log(format("rsp: %s",str));
	write(str + "\r\n");
}

// Courtesy of Michael J. Ryan <tracker1@theroughnecks.com>
function getReferenceTo(reference) {
	//sbbs msg_id pattern.
	var re = /^.*<[^\.]+\.([\d]+)\.([^@]+)@[^>]*>\s*$/;

	//Default Response
	var to = "All"

	//if TO is already established, return...
	if (reference=="")
		return to;

	//if this didn't originate from this bbs.
	if (reference.indexOf(system.inet_addr) < 0)
		return to;

	//Load the msgbase the original post was from
	if (!reference.match(re))
		return to;

	var sub = reference.replace(re,"$2");
	var msg = parseInt(reference.replace(re,"$1"));

	var msgbase = new MsgBase(sub);
	if(msgbase.open!=undefined && msgbase.open()==false)
		return to;

	var hdr = msgbase.get_msg_header(false,msg);
	if (hdr != null)
		to = hdr.from;

	msgbase.close();

	return to;
}

var username='';
var msgbase=null;
var selected=null;
var current_article=0;
var quit=false;

writeln(format("200 %s News (Synchronet %s%s-%s%s NNTP Service %s)"
		,system.name,system.version,system.revision,system.platform,system.beta_version
		,REVISION));

if(!no_anonymous)
	login("guest");	// Login as guest/anonymous by default

while(client.socket.is_connected && !quit) {

	// Get Request
	cmdline = client.socket.recvline(512 /*maxlen*/, 300 /*timeout*/);

	if(cmdline==null) {
		log("!TIMEOUT waiting for request");
		break;
	}

	if(cmdline=="") 	/* ignore blank commands */
		continue;

	log(format("cmd: %s",cmdline));

	cmd=cmdline.split(' ');

	switch(cmd[0].toUpperCase()) {
		case "AUTHINFO":
			if(cmd[1]==undefined) {
				writeln("500 Syntax error or unknown command");
				break;
			}
			switch(cmd[1].toUpperCase()) {
				case "USER":
					username='';
					for(i=2;cmd[i]!=undefined;i++) {
						if(i>2)
							username+=' ';
						username+=cmd[i];
					}
					writeln("381 More authentication required");
					break;
				case "PASS":
					if(login(username,cmd[2]))
						writeln("281 Authentication successful");
					else
						writeln("502 Authentication failure");
					break;
				default:
					writeln("500 Syntax error or unknown command");
					break;
			}
			continue;
		case "MODE":
			writeln("200 Hello, you can post");
			continue;
		case "DATE":
			d = new Date();
			writeln("111 " +
				format("%u%02u%02u%02u%02u%02u"
					,d.getUTCFullYear()
					,d.getUTCMonth()+1
					,d.getUTCDate()
					,d.getUTCHours()
					,d.getUTCMinutes()
					,d.getUTCSeconds()
				));
			continue;
		case "QUIT":
			writeln("205 closing connection - goodbye!");
			quit=true;
			continue;
	}

	if(!logged_in) {
		if (auto_login) {
			log("Autologin Search: Started");
			var oUser = new User(1);
			sUser = false;
			sPassword = ""
			iLastOn = 0;
			for (var i=1; i<=system.stats.total_users; i++) {
				oUser.number = i;
				if (oUser.ip_address == client.ip_address)
					if (!(oUser.security.restrictions&UFLAG_Q)) //don't count qnet users
						if (oUser.stats.laston_date > iLastOn) { //more recent than last match
							iLastOn = oUser.stats.laston_date;
							sUser = oUser.alias;
							sPassword = oUser.security.password;
							log("!Autologin Search: Match Found ("+sUser+")");
						}

			}
			oUser = null;
			log("Autologin Search: Finished");

			if (sUser)
				login(sUser,sPassword);
		}

		if(!logged_in) {
			writeln("502 Authentication required");
			log("!Authentication required");
			continue;
		}
	}

	/* These commands require login/authentication */
	switch(cmd[0].toUpperCase()) {

		case "SLAVE":
			slave = true;
			writeln("202 slave status noted");
			break;

		case "LIST":
			if(cmd[1]!=undefined && cmd[1].toUpperCase()=="OVERVIEW.FMT") {
				writeln("215 Order of fields in overview database.");
				writeln("Subject:");
				writeln("From:");
				writeln("Date:");
				writeln("Message-ID:");
				writeln("References:");
				writeln("Bytes:");
				writeln("Lines:");
			}
			else {
				writeln("215 list of newsgroups follows");
				if(include_mail && user.security.level == 99) {
					msgbase=new MsgBase("mail");
					if(msgbase.open()==true) {
						writeln(format("mail %u %u n", msgbase.last_msg, msgbase.first_msg));
						msgbase.close();
					}
				}
				for(g in msg_area.grp_list)
					for(s in msg_area.grp_list[g].sub_list) {
						msgbase=new MsgBase(msg_area.grp_list[g].sub_list[s].code);
						if(msgbase.open!=undefined && msgbase.open()==false)
							continue;
						writeln(format("%s %u %u %s"
							,msg_area.grp_list[g].sub_list[s].newsgroup
							,msgbase.last_msg
							,msgbase.first_msg
							,msg_area.grp_list[g].sub_list[s].can_post ? "y" : "n"
							));
						msgbase.close();
					}
			}
			writeln(".");	// end of list
			break;

		case "XGTITLE":
			writeln("282 list of newsgroups follows");
			if(include_mail && user.security.level == 99)
				writeln("mail complete mail database");
			for(g in msg_area.grp_list)
				for(s in msg_area.grp_list[g].sub_list)
					writeln(format("%s %s"
						,msg_area.grp_list[g].sub_list[s].newsgroup
						,msg_area.grp_list[g].sub_list[s].description
						));
			writeln(".");	// end of list
			break;

		case "NEWGROUPS":
			writeln("231 list of new newsgroups follows");
			writeln(".");	// end of list
			break;

		case "GROUP":
			if(cmd[1]==undefined) {
				writeln("411 no group specified");
				break;
			}
			found=false;
			if(include_mail && user.security.level==99 && cmd[1].toLowerCase()=="mail") {
				msgbase=new MsgBase("mail");
				if(msgbase.open()==true) {
					selected = { newsgroup: "mail" };
					found=true;
				}
			}
			if(!found) {
				for(g in msg_area.grp_list)
					for(s in msg_area.grp_list[g].sub_list)
						if(msg_area.grp_list[g].sub_list[s].newsgroup.toLowerCase()==cmd[1].toLowerCase()) {
							msgbase=new MsgBase(msg_area.grp_list[g].sub_list[s].code);
							if(msgbase.open!=undefined && msgbase.open()==false)
								continue;
							found=true;
							selected=msg_area.grp_list[g].sub_list[s];
							break;
						}
			}
			if(found)
				writeln(format("211 %u %u %u %s group selected"
					,msgbase.total_msgs	// articles in group
					,msgbase.first_msg
					,msgbase.last_msg
					,selected.newsgroup
					));
			else {
				writeln("411 no such newsgroup");
				log("!no such group");
				bogus_cmd_counter++;
			}
			break;

		case "OVER":
		case "XOVER":
			if(msgbase==null) {
				writeln("412 no newsgroup selected");
				break;
			}
			writeln("224 Overview information follows");
			var first, last;
			if(cmd[1]==undefined)
				first=last=current_article;
			else if(cmd[1].indexOf('-')>=0)	{ /* range */
				range=cmd[1].split('-');
				first=Number(range[0]);
				last=Number(range[1]);
			} else
				first=last=Number(cmd[1]);
			for(i=first;i<=last;i++) {
				hdr=msgbase.get_msg_header(false,i);
				if(hdr==null)
					continue;
				if(hdr.attr&MSG_DELETE)	/* marked for deletion */
					continue;
				writeln(format("%u\t%s\t%s\t%s\t%s\t%s\t%u\t%u"
					,i
					,hdr.subject
					,hdr.from
					,hdr.date
					,hdr.id			// message-id
					,hdr.reply_id	// references
					,hdr.data_length	// byte count
					,Math.round(hdr.data_length/79)+1	// line count
					));
			}
			writeln(".");	// end of list
			break;

		case "XHDR":
			if(cmd[1]==undefined || cmd[2]==undefined) {
				writeln("500 Syntax error or unknown command");
				break;
			}
			if(msgbase==null) {
				writeln("412 no newsgroup selected");
				break;
			}
			writeln("221 Header follows");
			var first, last;
			if(cmd[2].indexOf('-')>=0)	{ /* range */
				range=cmd[2].split('-');
				first=Number(range[0]);
				last=Number(range[1]);
			} else
				first=last=Number(cmd[2]);
			for(i=first;i<=last;i++) {
				hdr=msgbase.get_msg_header(false,i);
				if(hdr==null)
					continue;
				if(hdr.attr&MSG_DELETE)	/* marked for deletion */
					continue;
				var field="";
				switch(cmd[1].toLowerCase()) {	/* header */
					case "to":
						field=hdr.to;
						break;
					case "subject":
						field=hdr.subject;
						break;
					case "from":
						field=hdr.from;
						break;
					case "reply-to":
						field=hdr.replyto;
						break;
					case "date":
						field=hdr.date;
						break;
					case "message-id":
						field=hdr.id;
						break;
					case "references":
						field=hdr.reply_id;
						break;
					case "lines":
						field=Math.round(hdr.data_length/79)+1;
						break;
					/* FidoNet header fields */
					case "x-ftn-pid":
						field=hdr.ftn_pid;
						break;
					case "x-ftn-area":
						field=hdr.ftn_area;
						break;
					case "x-ftn-flags":
						field=hdr.ftn_flags;
						break;
					case "x-ftn-msgid":
						field=hdr.ftn_msgid;
						break;
					case "x-ftn-reply":
						field=hdr.ftn_reply;
						break;
				}
				if(field==undefined)
					field="";

				writeln(format("%u %s",i,field.toString()));
			}
			writeln(".");	// end of list
			break;

		case "ARTICLE":
		case "HEAD":
		case "BODY":
		case "STAT":
			if(msgbase==null) {
				writeln("412 no newsgroup selected");
				bogus_cmd_counter++;
				break;
			}
			if(cmd[1]==undefined) {
				writeln("420 no current article has been selected");
				bogus_cmd_counter++;
				break;
			}
			if(cmd[1]!='') {
				if(cmd[1].indexOf('<')>=0)		/* message-id */
					current_article=cmd[1];
				else
					current_article=Number(cmd[1]);
			}
			if(typeof(current_article)=="number" 
				&& (current_article<1 || isNaN(current_article))) {
				writeln("420 no current article has been selected");
				break;
			}

			hdr=null;
			body=null;
			hdr=msgbase.get_msg_header(false,current_article);

			if(hdr==null) {
				writeln("430 no such article found: " + current_article);
				break;
			}

			current_article=hdr.number;

			if(cmd[0].toUpperCase()!="HEAD")
				body=msgbase.get_msg_body(false,current_article
					,true /* remove ctrl-a codes */
					,true /* rfc822 formatted text */);

			// force taglines for QNET Users on local messages
			if(user.security.restrictions&UFLAG_Q && !hdr.from_net_type)
				body += "\r\n" + tearline + tagline;

/* Eliminate dupe loops
			if(user.security.restrictions&UFLAG_Q && hdr!=null)
*/
			if(hdr.attr&MSG_MODERATED && !(hdr.attr&MSG_VALIDATED)) {
				writeln("430 unvalidated message");
				break;
			}
			if(hdr.attr&MSG_DELETE) {
				writeln("430 deleted message");
				break;
			}
			if(hdr.attr&MSG_PRIVATE
				&& hdr.to.toLowerCase()!=user.alias.toLowerCase()
				&& hdr.to.toLowerCase()!=user.name.toLowerCase()) {
				writeln("430 private message");
				break;
			}

			switch(cmd[0].toUpperCase()) {
				case "ARTICLE":
					writeln(format("220 %d %s article retrieved - head and body follow",current_article,hdr.id));
					break;
				case "HEAD":
					writeln(format("221 %d %s article retrieved - header follows",current_article,hdr.id));
					break;
				case "BODY":
					writeln(format("222 %d %s article retrieved - body follows",current_article,hdr.id));
					break;
				case "STAT":
					writeln(format("223 %d %s article retrieved",current_article,hdr.id));
					break;
			}
			if(cmd[0].toUpperCase()=="STAT")
				break;

			if(cmd[0].toUpperCase()!="BODY") {

				if(!hdr.from_net_type)	/* local message */
					writeln(format("From: \"%s\" <%s@%s>"
						,hdr.from
						,hdr.from.replace(/ /g,".").toLowerCase()
						,system.inetaddr));
				else if(!hdr.from_net_addr.length)
					writeln(format("From: %s",hdr.from));
				else if(hdr.from_net_addr.indexOf('@')!=-1)
					writeln(format("From: \"%s\" <%s>"
						,hdr.from
						,hdr.from_net_addr));
				else
					writeln(format("From: \"%s\" <%s@%s>"
						,hdr.from
						,hdr.from.replace(/ /g,".").toLowerCase()
						,hdr.from_net_addr));

				if(hdr.path==undefined)
					hdr.path="not-for-mail";

				if(hdr.newsgroups==undefined)
					hdr.newsgroups = selected.newsgroup;

				if(hdr.from_org==undefined && !hdr.from_net_type)
					hdr.from_org=system.name;

				write_news_header(hdr,writeln);	// from newsutil.js
			}
			if(hdr!=null && body!=null)	/* both, separate with blank line */
				writeln("");
			if(body!=null) {
				write(body);
				msgs_read++;
			}
			writeln(".");
			break;

		case "NEXT":
		case "LAST":
			if(msgbase==null) {
				writeln("412 no newsgroup selected");
				break;
			}
			if(current_article<1) {
				writeln("420 no current article has been selected");
				break;
			}
			if(cmd[0].toUpperCase()=="NEXT")
				current_article++;
			else
				current_article--;
			writeln(format("223 %u %u article retrieved - request text separately"
				,current_article
				,current_article
				));
   			break;

		case "IHAVE":
			if(cmd[1]!=undefined && cmd[1].indexOf('@' + system.inetaddr)!=-1) {	// avoid dupe loop
				writeln("435 that article came from here. Don't send.");
				continue;
			}
		case "POST":
			if(user.security.restrictions&UFLAG_P) {
				writeln("440 posting not allowed");
				break;
			}
			writeln("340 send article to be posted. End with <CR-LF>.<CR-LF>");

			var hdr={ from: "", subject: "" };
			var posted=false;
			var header=true;
			var body="";
			var newsgroups=new Array();
			var lines=0;
			while(client.socket.is_connected) {

				line = client.socket.recvline(512 /*maxlen*/, 300 /*timeout*/);

				if(line==null) {
					log("!TIMEOUT waiting for text line");
					break;
				}

				//log(format("msgtxt: %s",line));

				if(line==".") {
					log(format("End of message text (%u chars)",body.length));
					break;
				}
				if(line=="" && header) {
					header=false;
					continue;
				}

				if(!header) {	/* Body text, append to 'body' */
					if(line.charAt(0)=='.')
						line=line.slice(1);		// Skip prepended dots
					body += line;
					body += "\r\n";
					lines++;
					continue;
				}
				log(line);

				parse_news_header(hdr,line);	// from newsutil.js
			}

			if(user.limits!=undefined
				&& lines > user.limits.lines_per_message) {
				log(format("!Message of %u lines exceeds user limit (%u lines)"
					,lines,user.limits.lines_per_message));
				writeln(format("441 posting failed, number of lines (%u) exceeds max (%u)"
					,lines,user.limits.lines_per_message));
				break;
			}

			newsgroups=hdr.newsgroups.split(',');

			if(hdr.to==undefined && hdr.reply_id!=undefined)
				hdr.to=getReferenceTo(hdr.reply_id);

			if(hdr.to==undefined && hdr.newsgroups!=undefined)
				hdr.to=hdr.newsgroups;

			if(!(user.security.restrictions&(UFLAG_G|UFLAG_Q))) {	// !Guest and !Network Node
				hdr.from=user.alias;
				hdr.from_ext=user.number;
			}

			/* Security logging */
			hdr.from_ip_addr	= client.ip_address;
			hdr.from_host_name	= client.host_name;
			hdr.from_protocol	= client.protocol;
			hdr.from_port		= client.port;

			if(system.trashcan("subject",hdr.subject)) {
				log(format("!BLOCKED subject: %s",hdr.subject));
				var reason = format("Blocked subject from %s (%s): %s"
					,user.alias,hdr.from,hdr.subject);
				system.spamlog("NNTP","BLOCKED"
					,reason,client.host_name,client.ip_address,hdr.to);
				writeln("441 posting failed");
				break;
			}

            for(n in newsgroups)
    			for(g in msg_area.grp_list)
				    for(s in msg_area.grp_list[g].sub_list)
					    if(msg_area.grp_list[g].sub_list[s].newsgroup.toLowerCase()
							==newsgroups[n].toLowerCase()) {
						    if(!msg_area.grp_list[g].sub_list[s].can_post)
							    continue;

						    if(msgbase!=null) {
							    msgbase.close();
							    delete msgbase;
						    }
						    if(msg_area.grp_list[g].sub_list[s].settings&SUB_NAME
							    && !(user.security.restrictions&(UFLAG_G|UFLAG_Q)))
							    hdr.from=user.name;	// Use real names
							if(msg_area.grp_list[g].sub_list[s].is_moderated)
								hdr.attr|=MSG_MODERATED;
							else
								hdr.attr&=~MSG_MODERATED;

						    msgbase=new MsgBase(msg_area.grp_list[g].sub_list[s].code);
							if(msgbase.open!=undefined && msgbase.open()==false)
								continue;
						    if(msgbase.save_msg(hdr,body)) {
							    log(format("%s posted a message (%u chars, %u lines) on %s"
									,user.alias, body.length, lines, newsgroups[n]));
							    writeln("240 article posted ok");
							    posted=true;
								msgs_posted++;
						    } else
							    log(format("!ERROR saving mesage: %s",msgbase.last_error));
					    }
			if(!posted) {
				log("!post failure");
				writeln("441 posting failed");
			}
   			break;

		default:
			writeln("500 Syntax error or unknown command");
			log("!unknown command");
			break;
	}

	if(user.security.restrictions&UFLAG_G	/* Only guest/anonymous logins can be "bogus" */
		&& bogus_cmd_counter >= max_bogus_cmds) {
		log(format("!TOO MANY BOGUS COMMANDS (%u)", bogus_cmd_counter));
		if(filter_bogus_clients) {
			log("!FILTERING CLIENT'S IP ADDRESS: " + client.ip_address);
			system.filter_ip("NNTP","- TOO MANY BOGUS COMMANDS (Example: " + cmdline +")"
				, client.host_name, client.ip_address, client.user_name);
		}
		break;
	}
}

// Log statistics

if(msgs_read)
	log(format("%u messages read",msgs_read));
if(msgs_posted)
	log(format("%u messages posted",msgs_posted));

/* End of nntpservice.js */

