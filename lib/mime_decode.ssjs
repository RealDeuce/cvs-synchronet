function mime_decode(hdr, body)
{
	var Message=new Array;
	var CT;
	var TE;
	var undef;

	for(head in hdr.field_list) {
		if(hdr.field_list[head].data.search(/content-type:/i)!=-1) {
			CT=hdr.field_list[head].data;
		}
		else if(hdr.field_list[head].data.search(/content-transfer-encoding:/i)!=-1) {
			TE=hdr.field_list[head].data;
		}
	}
	if(CT==undefined) {
		Message.type="plain";
		Message.body=decode_body(TE,undef,body);
		return(Message);
	}
	if(CT.search(/multipart\/[^\s;]*/i)!=-1) {
		var bound=CT.match(/;\s*boundary="?([^;\r\n]*?)"?/);
		re=new RegExp ("--"+bound[1]);
		msgbits=body.split(re);
		/* Search for attachments */
		for(bit in msgbits) {
			var pieces=msgbits[bit].split(/\r?\n\r?\n/,2);
			var disp=pieces[0].match(/content-disposition:\s+attachment[;\s]*filename=([^;\r\n]*)/i);
			if(disp==undefined)
				continue;
			if(Message.attachments==undefined)
				Message.attachments=new Array;
			Message.attachments.push(disp[1]);
		}
		/* Search for HTML encoded bit */
		for(bit in msgbits) {
			var pieces=msgbits[bit].split(/\r?\n\r?\n/,2);
			if(pieces[0].search(/content-type: text\/html/i)!=-1) {
				Message.body=decode_body(TE,pieces[0],pieces[1]);
				Message.type="html";
				return(Message);
			}
		}
		/* Search for plaintext bit */
		for(bit in msgbits) {
			pieces=msgbits[bit].split(/\r?\n\r?\n/,2);
			if(pieces[0].search(/content-type: text\/plain/i)!=-1) {
				Message.body=decode_body(TE,pieces[0],pieces[1]);
				Message.type="plain";
				return(Message);
			}
		}
	}

	if(CT.search(/text\/html/i)!=-1) {
		Message.type="html";
		Message.body=decode_body(TE,undef,body);
		return(Message);
	}

	Message.type="plain";
	Message.body=body;
	return(Message);
}

function decode_body(TE, heads, body)
{
	var tmp;

	if(heads!=undefined && heads != "") {
		tmp=heads.match(/content-transfer-encoding: ([^;\r\n]*)/i);
		if(tmp!=undefined)
			tmp=tmp[1];
		else {
			if(TE!=undefined) {
				tmp=TE.match(/content-transfer-encoding: ([^;\r\n]*)/);
				if(tmp!=undefined)
					tmp=tmp[1];
			}
		}
	}
	else {
		if(TE!=undefined) {
			tmp=TE.match(/content-transfer-encoding: ([^;\r\n]*)/);
			if(tmp!=undefined)
				tmp=tmp[1];
		}
		else
			tmp="";
	}
	
	if(tmp==undefined)
		tmp="";
	if(tmp.search(/quoted-principle/i)!=-1) {
		body=body.replace(/=(\r?\n)/g,"$1");
		body=body.replace(/=([A-Z0-9]{2})/ig,function (str,p1,offset,s)
			{
				return ascii(parseInt(p1,16));
			}
		);
		return body;
	}
	if(tmp.search(/base64/i)!=-1) {
		body=body.replace(/[^A-Za-z0-9\+\/\=]/g,'');
		return base64_decode(body);
	}

	return body;
}

function mime_get_attach(hdr, body, filename)
{
	var Message=new Array;
	var CT;
	var TE;
	var undef;

	for(head in hdr.field_list) {
		if(hdr.field_list[head].data.search(/content-type:/i)!=-1) {
			CT=hdr.field_list[head].data;
		}
		else if(hdr.field_list[head].data.search(/content-transfer-encoding:/i)!=-1) {
			TE=hdr.field_list[head].data;
		}
	}
	if(CT==undefined) {
		return(undefined);
	}
	if(CT.search(/multipart\/[^\s;]*/i)!=-1) {
		var bound=CT.match(/;\s*boundary="?([^;\r\n]*?)"?/);
		re=new RegExp ("--"+bound[1]);
		msgbits=body.split(re);
		/* Search for attachments */
		for(bit in msgbits) {
			var pieces=msgbits[bit].split(/\r?\n\r?\n/,2);
			var disp=pieces[0].match(/content-disposition:\s+attachment[;\s]*filename=([^;\r\n]*)/i);
			if(disp==undefined)
				continue;
			if(disp[1]==filename) {
				var contyp=pieces[0].match(/content-type:\s*([^\r\n]*)/i);
				if(contyp!=undefined && contyp[0]!=undefined)
					Message.content_type=contyp[1];
				Message.body=decode_body(undefined,pieces[0],pieces[1]);
				return(Message);
			}
		}
	}
	return(undefined);
}

