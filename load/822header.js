MsgBase.HeaderPrototype.get_rfc822_header=function(force_update)
{
	var content_type;
	var i;

	if(force_update===true)
		delete this.rfc822;

	if(this.rfc822==undefined) {
		this.rfc822='';
		this.rfc822 += "To: "+this.to+"\r\n";
		this.rfc822 += "Subject: "+this.subject+"\r\n";
		this.rfc822 += "Message-ID: "+this.id+"\r\n";
		this.rfc822 += "Date: "+this.date+"\r\n";

		if(!this.from_net_type || this.from_net_addr.length==0)    /* local message */
			this.rfc822 += "From: " + this.from + " <" + this.from.replace(/ /g,".").toLowerCase() + "@" + system.inetaddr + ">\r\n";
		else if(!this.from_net_addr.length)
			this.rfc822 += "From: " + this.from + "\r\n";
		else if(this.from_net_addr.indexOf('@')!=-1)
			this.rfc822 += "From: " + this.from+" <"+this.from_net_addr+">\r\n";
		else
			this.rfc822 += "From: " + this.from+" <"+this.from.replace(/ /g,".").toLowerCase()+"@"+this.from_net_addr+">\r\n";

		this.rfc822 += "X-Comment-To: "+this.to+"\r\n";
		if(this.path != undefined)
			this.rfc822 += "Path: "+system.inetaddr+"!"+this.path+"\r\n";
		if(this.from_org != undefined)
			this.rfc822 += "Organization: "+this.from_org+"\r\n";
		if(this.newsgroups != undefined)
			this.rfc822 += "Newsgroups: "+this.newsgroups+"\r\n";
		if(this.replyto != undefined)
			this.rfc822 += "Reply-To: "+this.replyto+"\r\n";
		else {
			if(base.subnum != -1)
				this.rfc822 += 'Reply-To: "'+this.from+'" <sub:'+base.cfg.code+'@'+system.inet_addr+'>\r\n';
		}
		if(this.reply_id != undefined)
			this.rfc822 += "In-Reply-To: "+this.reply_id+"\r\n";
		if(this.references != undefined)
			this.rfc822 += "References: "+this.references+"\r\n";
		else if(this.reply_id != undefined)
			this.rfc822 += "References: "+this.reply_id+"\r\n";
		if(this.reverse_path != undefined)
			this.rfc822 += "Return-Path: "+this.reverse_path+"\r\n";

		// Fidonet headers
		if(this.ftn_area != undefined)
			this.rfc822 += "X-FTN-AREA: "+this.ftn_area+"\r\n";
		if(this.ftn_pid != undefined)
			this.rfc822 += "X-FTN-PID: "+this.ftn_pid+"\r\n";
		if(this.ftn_TID != undefined)
			this.rfc822 += "X-FTN-TID: "+this.ftn_tid+"\r\n";
		if(this.ftn_flags != undefined)
			this.rfc822 += "X-FTN-FLAGS: "+this.ftn_flags+"\r\n";
		if(this.ftn_msgid != undefined)
			this.rfc822 += "X-FTN-MSGID: "+this.ftn_msgid+"\r\n";
		if(this.ftn_reply != undefined)
			this.rfc822 += "X-FTN-REPLY: "+this.ftn_reply+"\r\n";

		// Other RFC822 headers
		if(this.field_list!=undefined) {
			for(i in this.field_list) 
				if(this.field_list[i].type==RFC822HEADER) {
					if(this.field_list[i].data.toLowerCase().indexOf("content-type:")==0)
						content_type = this.field_list[i].data;
					this.rfc822 += this.field_list[i].data+"\r\n";
				}
		}
		if(content_type==undefined) {
			/* No content-type specified, so assume IBM code-page 437 (full ex-ASCII) */
			this.rfc822 += "Content-Type: text/plain; charset=IBM437\r\n";
			this.rfc822 += "Content-Transfer-Encoding: 8bit\r\n";
		}

		// Unwrap headers
		this.rfc822=this.rfc822.replace(/\s*\r\n\s+/g, " ");
		this.rfc822 += "\r\n";
	}
	return this.rfc822;
}
