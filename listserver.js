// listserver.js

// Mailing List Server module for Synchronet v3.11c

load("sbbsdefs.js");

const REVISION = "$Revision: 1.8 $".split(' ')[1];
const user_list_ext = ".list.sub";

log(LOG_INFO,"ListServer " + REVISION);

js.auto_terminate=false;

var ini_fname = system.ctrl_dir + "listserver.ini";

ini_file = new File(ini_fname);
if(!ini_file.open("r")) {
	log(LOG_ERR,format("!ERROR %s opening ini_file: %s"
		,ini_file.error, ini_fname));
	exit();
}
listserver_address=ini_file.iniGetValue("config","address","listserver@"+system.inet_addr);
listserver_name=ini_file.iniGetValue("config","name","Synchronet ListServer");
alias_list=ini_file.iniGetValue("config","aliases",new Array());
subj_cmd=ini_file.iniGetValue("config","SubjectCommand",false);
list_array=ini_file.iniGetAllObjects("name","list:");
ini_file.close();
if(!list_array.length) {
	log(LOG_ERR,"!No lists defined in " + ini_fname);
	exit();
}

for(var l in list_array) {
	/* Set default list addresses */
	if(!list_array[l].address)
		list_array[l].address = format("%s@%s", list_array[l].name, system.inet_addr);
	if(!msg_area.sub[list_array[l].sub]) {
		log(LOG_WARNING,"!Unrecognized sub-board internal code: " + list_array[l].sub);
		list_array[l].disabled=true;
		continue;
	}
	if(!list_array[l].description)
		list_array[l].description = msg_area.sub[list_array[l].sub].description;
	if(list_array[l].confirm==undefined)
		list_array[l].confirm=true;

	var msgbase = new MsgBase(list_array[l].sub);
	if(msgbase.open()==false) {
		log(LOG_ERR,format("%s !ERROR %s opening msgbase: %s"
			,list_array[l].name, msgbase.error, list_array[l].sub));
		continue;
	}
	list_array[l].msgbase_file = msgbase.file;

	/* Create the user list file if it doesn't exist */
	var user_fname = list_array[l].msgbase_file + user_list_ext;
	if(!file_exists(user_fname))
		file_touch(user_fname);
}

mailbase = new MsgBase("mail");
if(mailbase.open()==false) {
	log(LOG_ERR,"!ERROR " + mailbase.error + " opening mail database");
	exit();
}

/* Inbound message from SMTP Server? */
if(this.recipient_list_filename!=undefined) {	

	var error_file = new File(processing_error_filename);
	if(!error_file.open("w")) {
		log(LOG_ERR,format("!ERROR %s opening processing error file: %s"
			,error_file.error, processing_error_filename));
		exit();
	}

	var rcptlst_file = new File(recipient_list_filename);
	if(!rcptlst_file.open("r")) {
		error_file.writeln(log(LOG_ERR,format("!ERROR %s opening recipient list: %s"
			,rcptlst_file.error, recipient_list_filename)));
		exit();
	}
	var rcpt_list=rcptlst_file.iniGetAllObjects("number");
	rcptlst_file.close();

	var msgtxt_file = new File(message_text_filename);
	if(!msgtxt_file.open("r")) {
		error_file.writeln(log(LOG_ERR,format("!ERROR %s opening message text: %s"
			,msgtxt_file.error, message_text_filename)));
		exit();
	}
	var msgtxt = msgtxt_file.readAll()
	msgtxt_file.close();

	load("mailproc_util.js");	// import parse_msg_header() and get_msg_body()

	var header = parse_msg_header(msgtxt);
	header = convert_msg_header(header);
	var body = get_msg_body(msgtxt);

	var r;
	/* control message for list server? */
	for(r=0;r<rcpt_list.length;r++) {
		if(rcpt_list[r].Recipient.toLowerCase()==listserver_address.toLowerCase())
			break;
		for(n=0;n<alias_list.length;n++) {
			if(rcpt_list[r].Recipient.search(new RegExp(alias_list[n],"i"))!=-1)
				break;
		}
		if(n<alias_list.length)	/* match found */
			break;
	}
	if(r<rcpt_list.length) { 

		log(LOG_INFO,format("ListServer Control message from %s to %s: %s"
			,header.from, header.to, header.subject));

		file_remove(recipient_list_filename);

		if(subj_cmd)
			body.unshift(header.subject);	/* Process the subject as a command */

		var response = process_control_msg(body);
		var resp_hdr = {};

		resp_hdr.subject		= "Synchronet ListServer Results";
		resp_hdr.to				= header.from;
		resp_hdr.to_net_addr	= header.from_net_addr;
		resp_hdr.to_net_type	= NET_INTERNET;
		resp_hdr.from			= listserver_name;
		resp_hdr.from_net_addr	= listserver_address;
		resp_hdr.from_net_type	= NET_INTERNET;
		resp_hdr.from_agent		= AGENT_PROCESS;
		resp_hdr.reply_id		= header.id;

		/* Write response to message */
		if(mailbase.save_msg(resp_hdr, response.body.join('\r\n')))
			log(LOG_INFO,"ListServer Response to control message created");
		else
			log(LOG_ERR,format("ListServer !ERROR %s saving response message to mail msgbase"
				,msgbase.error));

		exit();
	}

	/* contribution to mailing list? */
	for(r=0;r<rcpt_list.length;r++) {
		var l;
		for(l=0;l<list_array.length;l++) {
/** DEBUG
			for(var p in list_array[l])
				log("list_array["+l+"]."+p+" = "+list_array[l][p]);
**/
			if(rcpt_list[r].Recipient.toLowerCase()==list_array[l].address.toLowerCase()
				&& !list_array[l].disabled
				&& !list_array[l].readonly)
				break;
		}
		if(l<list_array.length) {	/* match found */
			log(LOG_INFO,format("ListServer Contribution message from %s to %s: %s"
				,header.from, rcpt_list[r].Recipient, header.subject));

			if(!process_contribution(header, body, list_array[l]))
				break;
		}
	}

	exit();
}

for(var l in list_array) {

	if(js.terminated) {
		log(LOG_WARNING,"Terminated");
		break;
	}

	if(list_array[l].disabled)
		continue;

	list_name = list_array[l].name;

	msgbase = new MsgBase(list_array[l].sub);
	if(msgbase.open()==false) {
		log(LOG_ERR,format("%s !ERROR %s opening msgbase: %s"
			,list_name, msgbase.error, list_array[l].sub));
		delete msgbase;
		continue;
	}

	/* Get subscriber list */
	var user_list = get_user_list(list_array[l]);
	if(!user_list.length) {
		delete msgbase;
		continue;
	}

/***
	if(!user_list.length) {
		log(LOG_NOTICE,"No subscribers to list: " + list_name);
		delete msgbase;
		continue;
	}
***/

	/* Get export message pointer */
	ptr_fname = list_array[l].msgbase_file + ".list.ptr";
	ptr_file = new File(ptr_fname);
	if(!ptr_file.open("w+")) {
		log(LOG_ERR,format("%s !ERROR %s opening/creating file: %s"
			,list_name, ptr_file.error, ptr_fname));
		delete msgbase;
		continue;
	}

	var last_msg = msgbase.last_msg;
	var ptr = Number(ptr_file.readln());

	if(ptr < msgbase.first_msg)
		ptr = msgbase.first_msg;
	else
		ptr++;

	for(;ptr<=last_msg && !js.terminated; ptr++) {
		hdr = msgbase.get_msg_header(
			/* retrieve by offset? */	false,
			/* message number */		ptr,
			/* regenerate msg-id? */	false
			);
		if(hdr == null) {
			/**
			log(LOG_WARNING,format("%s !ERROR %s getting msg header #%lu"
				,list_name, msgbase.error, ptr));
			**/
			continue;
		}
		if(hdr.attr&(MSG_DELETE|MSG_PRIVATE))	{ /* marked for deletion */
			log(LOG_NOTICE,format("%s Skipping %s message #%lu from %s: %s"
				,list_name, hdr.attr&MSG_DELETE ? "deleted":"private"
				,ptr, hdr.from, hdr.subject));
			continue;
		}
		if(hdr.attr&MSG_MODERATED && !(hdr.attr&MSG_VALIDATED)) {
			log(LOG_NOTICE,format("%s Stopping at unvalidated moderated message #%lu from %s: %s"
				,list_name, ptr, hdr.from, hdr.subject));
			ptr--;
			break;
		}

		body = msgbase.get_msg_body(
				 false	/* retrieve by offset */
				,ptr	/* message number */
				,true	/* remove ctrl-a codes */
				,false	/* rfc822 formatted text */
				,true	/* include tails */
				);
		if(body == null) {
			log(LOG_ERR,format("%s !ERROR %s reading text of message #%lu"
				,list_name, msgbase.error, ptr));
			continue;
		}

		rcpt_list = new Array();
		for(u in user_list) {
			if(js.terminated)
				break;
			if(user_list[u].disabled || !user_list[u].address)
				continue;
			log(LOG_DEBUG,format("%s Enqueing message #%lu for %s <%s>"
				,list_name, ptr, user_list[u].name, user_list[u].address));
			rcpt_list.push(	{	to:				user_list[u].name,
								to_net_addr:	user_list[u].address, 
								to_net_type:	NET_INTERNET 
							} );
		}
		if(js.terminated) {
			ptr--;
			break;
		}
		if(rcpt_list.length < 1) {
			log(LOG_NOTICE,format("%s No active subscriptions", list_name));
			continue;
		}

		log(LOG_INFO,format("%s Sending message #%lu from %s to %lu recipients: %s"
			,list_name, ptr, hdr.from, rcpt_list.length, hdr.subject));

		hdr.replyto_net_type = NET_INTERNET;
		hdr.replyto_net_addr = list_array[l].address;
		if(!mailbase.save_msg(hdr,body,rcpt_list))
			log(LOG_ERR,format("%s !ERROR %s saving mail message"
				,list_name, mailbase.error));
	}

	if(ptr > last_msg)
		ptr = last_msg;

	ptr_file.rewind();
	ptr_file.length=0;		// truncate
	ptr_file.writeln(ptr);
	ptr_file.close();
}

/* clean-up */
mailbase.close();

/* End of Main */

/* Handle Mailing List Control Messages (e.g. subscribe/unsubscribe) here */
function process_control_msg(cmd_list)
{
	var response = { body: new Array(), subject: "" };
	
	response.body.push("Synchronet ListServer " +REVISION+ " Response:\r\n");

	for(var c in cmd_list) {
		var cmd=cmd_list[c];
		if(!cmd.length)
			continue;
		var token=cmd.split(/\s+/);
		switch(token[0].toLowerCase()) {
			case "lists":
				response.body.push("List of lists:");
				for(var l in list_array)
					if(!list_array[l].disabled)
						response.body.push("\t"+list_array[l].name.toUpperCase()
										  +"\t\t"+list_array[l].description);
				break;
			case "subscribe":
			case "unsubscribe":
				for(var l in list_array) {
					if(list_array[l].disabled || list_array[l].closed)
						continue;
					if(list_array[l].name.toLowerCase()==token[1].toLowerCase()) {
						response.body.push(subscription_control(token[0], list_array[l], token[2]));
						return(response);
					}
				}
				response.body.push("!List not found: " + token[1]);
				break;
			case "end":
				return(response);
			default:
				response.body.push("!Bad command: " + cmd);
			case "help":
				response.body.push("Available commands:");
				response.body.push("\tlists");
				response.body.push("\tsubscribe");
				response.body.push("\tunsubscribe");
				response.body.push("\thelp");
				response.body.push("\tend");
				break;
		}
	}

	return(response);
}

function get_user_list(list)
{
	var user_list = new Array();
	var user_fname = list.msgbase_file + user_list_ext;
	var user_file = new File(user_fname);
	if(!user_file.open("r")) {
		log(LOG_ERR,format("%s !ERROR %d opening file: %s"
			,list.name, user_file.error, user_fname));
	} else {
		user_list = user_file.iniGetAllObjects();
		user_file.close();
	}
	return user_list;
}

function find_user(user_list, address)
{
	for(var u in user_list)
		if(user_list[u].address.toLowerCase()==address.toLowerCase())
			return(u);
	return(-1);
}
function remove_user(user_list, address)
{
	var u=find_user(user_list, address);
	if(u==-1)
		return(false);
	user_list.splice(u,1);
	return(true);
}

function write_user_list(user_list, user_file)
{
	user_file.rewind();
	user_file.length = 0;
	for(var u in user_list) {
		user_file.writeln("[" + user_list[u].name + "]");
		for(var p in user_list[u])
			user_file.writeln(p + " = " + user_list[u][p]);
	}
}

function subscription_control(cmd, list, address)
{
	if(!address)
		address=sender_address;

	log(LOG_INFO,format("%s Subscription control command (%s) from %s"
		,list.name,cmd,address));

	/* Get subscriber list */
	var user_fname = list.msgbase_file + user_list_ext;
	var user_file = new File(user_fname);
	if(!user_file.open("r+"))
		return log(LOG_ERR,format("%s !ERROR %d opening file: %s"
			,list.name, user_file.error, user_fname));

	user_list = user_file.iniGetAllObjects();
	
	switch(cmd.toLowerCase()) {
		case "unsubscribe":
			if(remove_user(user_list, address)) {
				write_user_list(user_list, user_file);
				return log(LOG_INFO,address + " unsubscribed successfully");
			}
			return("!subscriber not found: " + address);
		case "subscribe":
			if(find_user(user_list, address)!=-1)
				return log(address + " is already subscribed");
			var now=time();
			user_list.push({ 
				 name:					sender_name 
				,address:				address
				,created:				system.timestr(now)
				,last_activity:			system.timestr(now)
				,last_activity_time:	format("%08lxh",now)
			});
			write_user_list(user_list, user_file);
			return log(address + " subscription successful");
	}
}

/* Handle Mailing List Contributions here */
function process_contribution(header, body, list)
{
	// ToDo: apply filtering here?

	var user_list = get_user_list(list);

	// verify author/sender is a list subscriber here

	if(find_user(user_list, sender_address)==-1) {
		error_file.writeln(log(LOG_WARNING,format("%s !ERROR %s is not a subscriber"
			,list.name, sender_address)));
		return(false);
	}

	var msgbase=new MsgBase(list.sub);
	if(!msgbase.open()) {
		error_file.writeln(log(LOG_ERR,format("%s !ERROR %s opening msgbase: %s"
			,list.name, msgbase.error, list.sub)));
		return(false);
	}

	// Convert from RFC822 to Synchronet-compatible
	header = convert_msg_header(header);

	if(!user.compare_ars(msgbase.cfg.moderated_ars))
		header.attr |= MSG_MODERATED;

	if(!msgbase.save_msg(header, body.join('\r\n'))) {
		log(LOG_ERR,format("%s !ERROR %s saving message to sub: %s"
			,list.name, msgbase.error, list.sub));
		return(false);
	}

	return(true);
}