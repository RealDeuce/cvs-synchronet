/* $Id$ */

if(!js.global || js.global.SOCK_STREAM==undefined)
	load('sockdefs.js');
if(!js.global || js.global.URL==undefined)
	load("url.js");

/*
 * TODO Stuff:
 *	Asynchronous requests
 *	Keep-alive support
 *	HTTP/1.1 (chunked, trailers, etc)
 *	Parse response headers
 */

function HTTPRequest()
{
	this.AddDefaultHeaders=function(){
		// General Headers
		this.request_headers.push("Connection: close");
		if(js.global.client != undefined)
			this.request_headers.push("Via: "+client.protocol.toLowerCase()+"/1.0 "+system.name);
		// Request Headers
		this.request_headers.push("Accept: text/*");
		this.request_headers.push("Accept-Charset: ISO-8859-13,Latin-9,ISO-8859-15,ISO-8859-1,UTF-8;q=0.5,*;q=0.1");
		this.request_headers.push("Accept-Encoding: ");
		this.request_headers.push("Host: "+this.url.host);
		if(this.referer != undefined)
			this.request_headers.push("Referer: "+this.referer);
		this.request_headers.push("User-Agent: SYNXv0.1");
	};

	this.SetupGet=function(url, referer, base) {
		this.referer=referer;
		this.base=base;
		this.url=new URL(url, this.base);
		if(this.url.scheme!='http' && this.url.scheme!='https')
			throw("Unknown scheme! '"+this.url.scheme+"'");
		if(this.url.path=='')
			this.url.path='/';
		if(this.url.query != '');
		this.request="GET "+this.url.request_path+" HTTP/1.0";
		this.request_headers=[];
		this.AddDefaultHeaders();
	};

	this.SetupPost=function(url, referer, base, data) {
		this.referer=referer;
		this.base=base;
		this.url=new URL(url, this.base);
		if(this.url.scheme!='http')
			throw("Unknown scheme! '"+this.url.scheme+"'");
		if(this.url.path=='')
			this.url.path='/';
		if(this.url.query != '');
		this.request="POST "+this.url.request_path+" HTTP/1.0";
		this.request_headers=[];
		this.AddDefaultHeaders();
		this.body=data;
		this.request_headers.push("Content-Type: application/x-www-form-urlencoded");
		this.request_headers.push("Content-Length: "+data.length);
	};

	this.SendRequest=function() {
		var i;

		if((this.sock=new Socket(SOCK_STREAM))==null)
			throw("Unable to create socket");
		if(!this.sock.connect(this.url.host, this.url.port?this.url.port:(this.url.scheme=='http'?80:443)))
			throw("Unable to connect");
		if(this.url.scheme=='https')
			this.sock.ssl_session=true;
		if(!this.sock.send(this.request+"\r\n"))
			throw("Unable to send request");
		for(i in this.request_headers) {
			if(!this.sock.send(this.request_headers[i]+"\r\n"))
				throw("Unable to send headers");
		}
		if(!this.sock.send("\r\n"))
			throw("Unable to terminate headers");
		if(this.body != undefined) {
			if(!this.sock.send(this.body))
				throw("Unable to send body");
		}
	};

	this.ReadStatus=function() {
		this.status_line=this.sock.recvline(4096);
		if(this.status_line==null)
			throw("Unable to read status");
	};

	this.ReadHeaders=function() {
		var header='';
		var m;
		this.response_headers=[];

		for(;;) {
			header=this.sock.recvline(4096, 120);
			if(header==null)
				throw("Unable to receive headers");
			if(header=='')
				return;
			this.response_headers.push(header);
			m=header.match(/^Content-length:\s+([0-9]+)$/);
			if(m!=null)
				this.contentlength=parseInt(m[0]);
		}
	};

	this.ReadBody=function() {
		var ch;

		this.body='';
		while((ch=this.sock.recv(1))!=null && ch != '') {
			this.body += ch;
		}
	};

	this.ReadResponse=function() {
		this.ReadStatus();
		this.ReadHeaders();
		this.ReadBody();
	};

	this.Get=function(url, referer, base) {
		this.SetupGet(url,referer,base);
		this.SendRequest();
		this.ReadResponse();
		return(this.body);
	};

	this.Post=function(url, data, referer, base) {
		this.SetupPost(url,referer,base,data);
		this.SendRequest();
		this.ReadResponse();
		return(this.body);
	};
}
