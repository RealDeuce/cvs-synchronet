//load("synchronet-json.js");
/* socket mod version */
Socket.prototype.VERSION = "$Revision$".replace(/\$/g,'').split(' ')[1];
/* round trip packet time */
Socket.prototype.latency = 0;
/* one way (latency / 2) */
Socket.prototype.time_offset = 0; 
/* largest receivable packet */
Socket.prototype.max_recv = 102400;
/* timeout for socket.recvline() */
Socket.prototype.recv_wait = 1;
/* last ping sent */
Socket.prototype.ping_sent = 0;

/* socket prototype to automatically encode JSON data */
Socket.prototype.sendJSON = function(object) {
	try {
		var data=JSON.stringify(object,this.replacer,this.space)+"\r\n";
		this.send(data); 
		log(LOG_DEBUG,"-->" + this.descriptor + ": " + data);
	} catch(e) {
		log(LOG_ERROR,e);
	}
};

/*  socket prototype to automatically decode JSON data */
Socket.prototype.recvJSON = function() { 
	var packet=this.recvline(this.max_recv,this.recv_wait); 
	if(packet != null) {
		log(LOG_DEBUG,"<--" + this.descriptor + ": " + packet);
		try {
			packet=JSON.parse(packet,this.reviver);
		} 
		catch(e) {
			log(LOG_ERROR,e);
		}
	}
	return packet;
};

/* ping pong */		
Socket.prototype.pingOut = function(func) {
	var ping={
		scope:"SOCKET",
		func:func,
		data:Date.now()
	}
	this.sendJSON(ping);
	this.ping_sent=ping.time;
};

/* calculate latency */
Socket.prototype.pingIn = function(packet) {
	var time=Date.now();
	var latency=(time-this.ping_sent)/2;
	var gap=(this.ping_sent+latency) - packet.data;
	if(this.time_offset != gap) {
		this.time_offset=gap;
		this.latency=latency;
		log(LOG_DEBUG,this.descriptor + "latency rt: " + this.latency + " ow: " + this.time_offset);
	}
};
