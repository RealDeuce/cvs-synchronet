load("../web/lib/msgslib.ssjs");

if(sub=='mail') {
	template.sub=new Object;
	template.sub.description="Personal E-Mail";
	template.sub.code="mail";
}
else {
	template.sub=msg_area.sub[sub];
	template.group=msg_area.grp[msg_area.sub[sub].grp_name];
}

if(sub!='mail') {
	if(! msg_area.sub[sub].can_post)  {
		error("You don't have sufficient rights to post in this sub");
	}
}
if(msgbase.open!=undefined && msgbase.open()==false) {
	error(msgbase.last_error);
}
hdr=msgbase.get_msg_header(false,parseInt(http_request.query.reply_to));
hdr=clean_msg_headers(hdr,CLEAN_MSG_REPLY);
if(sub!='mail') {
	if(msg_area.sub[sub].settings&SUB_AONLY)
		template.anonnote=anon_only_message;
	else if(msg_area.sub[sub].settings&SUB_ANON) {
		if(hdr.attr&MSG_ANONYMOUS)
			template.anonnote=anon_reply_message;
		else
			template.anonnote=anon_allowed_message;
	}
	if(msg_area.sub[sub].settings&SUB_PONLY)
		template.privnote=private_only_message;
	else if(msg_area.sub[sub].settings&SUB_PRIV) {
		if(hdr.attr&MSG_PRIVATE)
			template.privnote=private_reply_message;
		else
			template.privnote=private_allowed_message;
	}
}
template.subject=hdr.subject;
if(template.subject.search(/^re:\s+/i)==-1)
	template.subject='Re: '+template.subject;
if(sub=='mail') {
	if(hdr.replyto_net_addr!=undefined && hdr.replyto_net_addr != '')
		template.from=hdr.replyto_net_addr;
	else {
		if(hdr.from_net_addr != undefined && hdr.from_net_addr != '')
			template.from=hdr.from_net_addr;
		else
			template.from=hdr.from;
	}
}
else
	template.from=hdr.from;

template.number=hdr.number;

template.body=msgbase.get_msg_body(false,parseInt(http_request.query.reply_to),true);
if(this.word_wrap != undefined)  {
	template.body=quote_msg(word_wrap(template.body,79),79);
}
else  {
	template.body=template.body.replace(/^(.)/mg,"> $1");
}
title="Reply to message";
write_template("header.inc");
write_template("topnav.inc");
write_template("leftnav.inc");
write_template("msgs/reply.inc");
write_template("footer.inc");

msgs_done();
