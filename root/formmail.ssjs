// Basic FormMail script, emulates Matt's FormMail.pl Script

// $Id: formmail.ssjs,v 1.10 2005/03/13 05:17:42 rswindell Exp $

load("sbbsdefs.js");

// List of supported 'hidden' field (not included in body text)
var hidden_fields = {
	recipient:1,
	redirect:1,
	subject:1
};

var value_fmt_string = "%-10s = %s";
if(http_request.query.value_fmt_string
	&& strip_ctrl(http_request.query.value_fmt_string).length)
	value_fmt_string = strip_ctrl(http_request.query.value_fmt_string);

var redir = strip_ctrl(http_request.query.redirect);
if(!redir)
	redir = http_request.header.referer;

function results(level, text)
{
	log(level,text);
	writeln("<html>");
	writeln("<head>");
	writeln("<title>Sending e-mail</title>");
	writeln("</head>");

	writeln("<body>");
	writeln("<center>");
	if(level<=LOG_WARNING)
		writeln("!ERROR: ".bold());
	writeln(text);
	writeln("<p>");
	writeln(("Click here to return to " + redir.toString().italics()).link(redir));
	writeln("</body>");
	writeln("</html>");
	exit();
}

var msgbase=new MsgBase("mail");
if(!msgbase.open())
	results(LOG_ERR,format("%s opening mail base", msgbase.error));

//------------------------
// Build the body text
//------------------------
var i;
var body="Form fields follow:\r\n\r\n";
for(i in http_request.query) {
	if(hidden_fields[i])
		continue;
	if(http_request.query[i].toString().length)
		body += format(value_fmt_string, i, http_request.query[i]) + "\r\n";
}

body+=format("\r\nvia %s\r\nat %s [%s]\r\n"
			 ,http_request.header['user-agent']
			 ,http_request.remote_host, http_request.remote_ip);

//------------------------
// Build the e-mail header
//------------------------
var hdr = { from: 'FormMail',
			subject: 'WWW Form Submission' };

// Use form-specified message subject
if(http_request.query.subject)		
	hdr.subject			=strip_ctrl(http_request.query.subject[0]);

// Use form-specified email address
if(http_request.query.email && http_request.query.email.toString().length) {
	hdr.from_net_addr	=strip_ctrl(http_request.query.email);
	hdr.from			=strip_ctrl(http_request.query.email);
}

// Use form-specified real name
if(http_request.query.realname && http_request.query.realname.toString().length)
	hdr.from			=strip_ctrl(http_request.query.realname);

//-------------------------
// Build the recipient list
//-------------------------
var recipient = ['Sysop'];

// Use form-specified recipient list
if(http_request.query.recipient)
	recipient=http_request.query.recipient[0].split(/\s*,\s*/);

var rcpt_list = [];

for(i in recipient) {

	var rcpt  = { to: strip_ctrl(recipient[i]) };

	rcpt.to_net_type=netaddr_type(rcpt.to);
	if(rcpt.to_net_type!=NET_NONE)
		rcpt.to_net_addr=rcpt.to;
	else {
		var usrnum=system.matchuser(rcpt.to);
		if(usrnum==0)
			results(LOG_ERR,"bad local username specified: " + rcpt.to);
		rcpt.to_ext=usrnum;
	}
	rcpt_list.push(rcpt);
}

if(!msgbase.save_msg(hdr,client,body,rcpt_list))
	results(LOG_ERR,format("%s saving message", msgbase.error));

results(LOG_INFO,"E-mail sent from " + hdr.from + " to " + recipient.toString().italics().bold() + " successfully.");
