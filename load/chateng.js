/*
	JAVASCRIPT MULTI-USER CHAT ENGINE -- BY MCMLXXIX (Matt Johnson) 08/2008
	***UPDATED: 11/2009
	-----------------------------------------------
	FULLSCREEN DEFAULT OR WINDOWED MODE WITH SUPPLIED PARAMETERS

	NOTE: 
	For a simple full screen chat, create a script like the following:
	
	load("chateng.js");
	Chat();
	
	This will call the following Chat() function 
	to handle the engine in its simplest form
	
	For more advanced chatting, see 
	ChatEngine this.init() method
*/
function Chat(key,engine)
{
	if(!engine) 
	{
		engine=new ChatEngine();
		engine.init();
	}
	engine.redraw();
	if(key)
	{
		engine.processKey(key);
		switch(key)
		{
			case '\x1b':	
				return false;
			default:
				return true;
		}
	}
	else
	{
		while(1)
		{
			engine.receive();
			key=console.inkey(K_NOCRLF|K_NOSPIN|K_NOECHO,5);
			if(key) {
				engine.processKey(key);
				switch(key.toUpperCase())
				{
					case '\x1b':	
						engine.exit();
						return false;
					default:
						key="";
						break;
				}
			}
		}
	}
}
//*************MAIN ENGINE*************
load("commclient.js");
load("scrollbar.js");
load("graphic.js");
//load("str_cmds.js");
load("nodedefs.js");
load("msgwndw.js");

bbs.sys_status |= SS_PAUSEOFF;	
oldpass=console.ctrl_key_passthru;

const flag_normal=			"#";
const flag_global=			"!";
const flag_private=		"%";
const flag_alert=			2;
const flag_notice=			1;
const flag_message=		0;

const local_color=			"\1n\1g";
const remote_color=		"\1n\1c";
const alert_color=			"\1r\1h";
const notice_color=		"\1n\1w";
const input_color=			"\1n";
const private_color=		"\1n\1y";
const global_color=		"\1n\1m";
	
function ChatEngine(root)
{	
	//TODO: the only time this will be used is for storing chat history
	//maybe give ALL chat history files their own home, independent of the parent script
	var root_dir=(root?root:js.exec_dir);
	var stream=new ServiceConnection("chat",user.alias);
	this.input_line=new InputLine();
	this.channel=new Channel();
	
	// USEFUL METHODS 
	this.init=function(chan,c,r,x,y,ix,iy,iw,bg) //NOTE: DESTROYS BUFFER AND MESSAGE LIST
	{
		var channel="#" + chan.replace(/\s+/g,'_');
		this.joinMessage(channel);
		this.input_line=new InputLine();
		this.input_line.init(ix,iy,iw,bg);
		this.channel.init(channel,x,y,c,r,bg);
	}
	this.initbox=function()
	{
		this.channel.initbox();
		this.input_line.initbox();
	}
	this.exit=function()
	{
		this.exitMessage();
		stream.close();
		console.ctrlkey_passthru=oldpass;
		bbs.sys_status&=~SS_MOFF;
		bbs.sys_status&=~SS_PAUSEOFF;
		console.attributes=ANSI_NORMAL;
	}
	this.exitMessage=function()
	{
		//if(user.compare_ars("QUIET")) return false;
		//var message=new Message(user.alias + " has left the chan. " + timeStamp(time()),flag_notice);
		var quit=new ChannelAction("QUIT",this.channel.chan,user.alias);
		this.send(quit);
		//this.send(message);
	}
	this.joinMessage=function(chan)
	{
		var nick=new Nick(user.alias,user.name);
		this.send(nick);
		//if(user.compare_ars("QUIET")) return false;
		if(this.channel.chan !=chan) {
			if(this.channel.chan) {
				var part=new ChannelAction("PART",this.channel.chan,user.alias);
				this.send(part);
			}
			var join=new ChannelAction("JOIN",chan,user.alias);
			this.send(join);
		}
	}
	this.resize=function(x,y,c,r,ix,iy,iw) //NOTE: DOES NOT DESTROY BUFFER OR MESSAGE LIST
	{
		this.input_line.init(ix,iy,iw);
		this.channel.init(x,y,c,r);
		this.redraw();
	}
	this.getUserList=function()
	{
		
	}
	this.findUser=function(id)
	{
		//return stream.findUser(id);
		return true;
	}
	this.redraw=function()
	{
		this.channel.draw();
		this.input_line.draw();
	}
	this.send=function(message)
	{
		if(!message.level) message.level=flag_message;
		if(!message.scope) message.scope=flag_normal;
		if(!message.chan) message.chan=this.channel.chan;
		if(!stream.send(message)) {
			this.channel.alert("message not sent.");
		}
	}
	this.receive=function()
	{
		var notices=stream.getNotices();
		while(notices.length) {
			this.channel.notice(notices.shift());
		}
		var packet=stream.receive();
		this.processData(packet);
	}
	this.processData=function(packet)
	{
		if(packet && packet.func) {
			this.channel.process(packet);
		}
	}
	this.processKey=function(key) //PROCESS ALL INCOMING KEYSTROKES
	{
		switch(key.toUpperCase())
		{
		//borrowed Deuce's feseditor.js
		case '\x00':	/* CTRL-@ (NULL) */
		case '\x03':	/* CTRL-C (Center Line) */
		case '\x04':	/* CTRL-D (Quick find in SyncEdit)*/
		case '\x0b':	/* CTRL-K */
		case '\x0c':	/* CTRL-L (Insert Line) */
		case '\x0e':	/* CTRL-N */
		case '\x0f':	/* CTRL-O (Quick Save/exit in SyncEdit) */
		case '\x10':	/* CTRL-P */
		case '\x11':	/* CTRL-Q (XOff) (Quick Abort in SyncEdit) */
		case '\x12':	/* CTRL-R (Quick redraw in SyncEdit) */
		case '\x13':	/* CTRL-S (Xon)  */
		case '\x14':	/* CTRL-T (Justify Line in SyncEdit) */
		case '\x15':	/* CTRL-U (Quick Quote in SyncEdit) */
		case '\x16':	/* CTRL-V (toggle insert mode) */
		case '\x17':	/* CTRL-W (Delete Word) */
		case '\x18':	/* CTRL-X (PgDn in SyncEdit) */
		case '\x19':	/* CTRL-Y (Delete Line in SyncEdit) */
		case '\x1a':	/* CTRL-Z (EOF) (PgUp in SyncEdit)  */
		case '\x1c':	/* CTRL-\ (RegExp) */
		case '\x1f':	/* CTRL-_ Safe quick-abort*/
		case '\x7f':	/* DELETE */
		case '\x1b':	/* ESC (This should parse extra ANSI sequences) */
		case KEY_LEFT:
		case KEY_RIGHT:
			break;
		case KEY_UP:
		case KEY_DOWN:
		case KEY_HOME:	
		case KEY_END:	
			this.channel.scroll(key);
			break;
		case '\b':
			this.input_line.backspace();
			break;
		case '\r':
		case '\n':
			this.submit();
			break;
		case '\x09':	/* CTRL-I TAB */
			this.input_line.toggle();
			break;
		case '@':
			if(!user.compare_ars("SYSOP") && !(bbs.sys_status&SS_TMPSYSOP)) break;
		default:
			this.input_line.bufferKey(key);
			break;
		}
		return true;
	}
	this.submit=function()
	{
		var message=this.input_line.submit();
		if(message) {
			if(!message.dest) message.dest=this.channel.chan;
			this.send(message);
			this.channel.process(message);
		}
	}
	this.cycle=function()
	{
		this.receive();
	}
}
function Channel()
{
	this.columns=console.screen_columns;
	this.rows=console.screen_rows;
	this.x=1;
	this.y=1;
	this.window;
	this.scrollbar;
	this.fullscreen=true;
	this.ignored=[];
	this.box=false;
	this.chan="";
	this.bg="";
	
	this.init=function(chan,x,y,c,r,bg)
	{
		if(bg) this.bg=bg;
		if(chan) this.chan=chan;

		if(!(x || y || c || r)) {
			this.fullscreen=true;
			console.ctrlkey_passthru=oldpass;
			bbs.sys_status&=~SS_MOFF;
			return;
		}
		if(x && y) {
			this.x=x;
			this.y=y;
			this.fullscreen=false;
		} 
		if(c && r) {
			this.columns=c;
			this.rows=r;
			this.fullscreen=false;
		}
		
		if(this.box) this.initbox();
		if(this.columns>=console.screen_columns) this.columns=console.screen_columns-1;
		this.scrollbar=new Scrollbar(this.x+this.columns,this.y,this.rows,"vertical","\1k\1h"); 
		this.window=new Graphic(this.columns,this.rows,getColor(this.bg));
		console.ctrlkey_passthru="+ACGKLOPQRTUVWXYZ_";
		bbs.sys_status|=SS_MOFF;
	}
	this.initbox=function()
	{
		this.box=new Window(this.x-1,this.y-1,this.columns+2,this.rows+2);
		this.box.init("\1n\1cCHAT","\1n\1c" + this.chan);
	}
	this.ignore=function(alias)
	{
		if(this.ignored[alias]==true) this.ignored[alias]=false;
		else ignored[alias]==true;
	}
	this.process=function(data)
	{
		if(this.ignored[data.origin]==true) return false;
		switch(data.func) 
		{
		case "JOIN":
			if(data.chan==this.chan) this.post(data.origin + " joined " + data.chan.substr(1).replace(/_/g,' '),notice_color);
			break;
		case "PART":
			if(data.chan==this.chan) this.post(data.origin + " left " + data.chan.substr(1).replace(/_/g,' '),notice_color);
			break;
		case "QUIT":
			if(data.chan==this.chan) this.post(data.origin + " left chat",notice_color);
			break;
		case "PRIVMSG":
			if(data.dest[0]=="#") {
				if(data.dest=="#global") {
					if(data.origin.toUpperCase()==user.alias.toUpperCase()) {
						this.post(data.msg,global_color,data.origin);
					} else {
						this.post(data.msg,global_color + "\1h",data.origin);
					}
				} else if(data.dest==this.chan) {
					if(data.origin.toUpperCase()==user.alias.toUpperCase()) {
						this.post(data.msg,local_color,data.origin);
					} else {
						this.post(data.msg,remote_color,data.origin);						
					}
				}
			} else {
				if(data.dest.toUpperCase()==user.alias.toUpperCase()) {
					this.post(data.msg,private_color + "\1h",data.origin);
				} else if(data.origin.toUpperCase()==user.alias.toUpperCase()) {
					this.post(data.msg,private_color,data.origin);
				}
			}
			break;
		}
		/*
		switch(data.scope)
		{
		case flag_private:
			if(data.chan) {
				if(data.chan==user.alias) this.post(data.msg,private_color + "\1h",data.origin);
				else this.post(data.msg,private_color,data.origin + "\1h-" +private_color+ data.chan);
			}
			break;
		case flag_global:
			switch(data.origin) {
				case user.alias:
					this.post(data.msg,global_color,data.origin);
					break;
				default:
					this.post(data.msg,global_color + "\1h",data.origin);
					break;
			}
			break;
		case flag_normal:
		default:
			if(data.chan==this.chan) {
				if(!data.origin) {
					switch(data.level) 
					{
					case flag_alert:
						this.post(data.msg,alert_color);
						break;
					case flag_notice:
					default:
						this.post(data.msg,notice_color);
						break;
					}
				} else	if(data.origin==user.alias) {
					this.post(data.msg,local_color,data.origin);
				} else this.post(data.msg,remote_color,data.origin);
			}
			break;
		}
		*/
	}
	this.scroll=function(key) 
	{
		if(!this.fullscreen && this.window.length>this.window.height) {
			switch(key)
			{
				case '\x02':	/* CTRL-B KEY_HOME */
					this.window.home();
					break;
				case '\x05':	/* CTRL-E KEY_END */
					this.window.end();
					break;
				case KEY_DOWN:
					this.window.scroll(1);
					break;
				case KEY_UP:
					this.window.scroll(-1);
					break;
			}
			this.window.draw(this.x,this.y);
			this.scrollbar.draw(this.window.index,this.window.length,this.window.height);
		}
	}
	this.post=function(text,color,origin,chan)
	{
		if(text.indexOf('@')>=0) {
			if(user.compare_ars("SYSOP") || bbs.sys_status&SS_TMPSYSOP) {
				text=text.replace(/@/g,"?");
			}
		}
		var msg;
		if(origin) {
			msg="\r" + color + this.bg + origin + "\1h: " + color + this.bg + text + "\r\n";
		} else {
			msg="\r" + color + this.bg + text + "\r\n";
		}
		if(this.fullscreen) {
			console.putmsg(msg,P_SAVEATR); 
		} else {
			this.window.putmsg(false,false,msg,undefined,true); 
			this.draw();
		}
	}
	this.list=function(array,color) //DISPLAYS A TEMPORARY MESSAGE IN THE CHAT WINDOW (NOT STORED)
	{
		for(var i=0;i<array.length;i++) this.post(array[i],color);
	}
	this.notice=function(msg)
	{
		this.post(msg,notice_color);
	}
	this.clear=function()
	{
		clearBlock(this.x,this.y,this.columns,this.rows);
	}
	this.alert=function(msg)
	{
		this.post(msg,alert_color);
	}
	this.draw=function()
	{
		if(!this.fullscreen) {
			if(this.box) this.box.draw();
			this.window.draw(this.x,this.y);
			if(this.window.length>this.window.height) this.scrollbar.draw(this.window.index,this.window.length,this.window.height);
		}
	}
}
function Message(msg,level,origin,target)
{
	this.msg=msg;
	this.level=level;
	this.func="PRIVMSG";
	this.origin=origin;
	this.dest=target;
}
function ChannelAction(action,chan,origin) 
{
	this.func=action;
	this.chan=chan;
	this.origin=origin;
}
function Nick(nick,username)
{
	this.func="NICK";
	this.nickname=nick;
	this.hops=1;
	this.created=time();
	this.username=nick;
	this.realname=username;
	this.hostname=system.inet_addr;
	this.servername=system.inet_addr;
}
function InputLine()
{
	this.x=1;
	this.y=1;
	this.width=0;
	this.bg="";
	this.fg=input_color;
	this.buffer="";
	this.scope=flag_normal;
	this.box=false;
	
	this.clear=function() 
	{
		if(this.width>0) {
			console.gotoxy(this);
			console.putmsg(this.bg+format("%*s",this.width,""),P_SAVEATR);
		} else {
			console.write("\r");
			console.cleartoeol();
		}
	}
	this.init=function(x,y,w,bg,fg) 
	{
		if(x) this.x=x;
		if(y) this.y=y;
		if(w) this.width=w;
		if(bg) this.bg=bg;
		if(fg) this.fg=fg;
		if(this.box) this.initbox();
		this.reset();
	}
	this.initbox=function()
	{
		this.box=new Window(this.x-1,this.y-1,this.width+2,3);
		var color=this.fg;
		var subtitle=false;
		switch(this.scope) {
			case flag_global:
				color=global_color;
				subtitle="GLOBAL";
				break;
			case flag_private:
				color=private_color;
				subtitle="PRIVATE";
				break;
			case flag_normal:
			default:
				break;
		}
		this.box.init("\1n\1cINPUT",subtitle?color + subtitle:false);
	}
	this.submit=function()
	{
		if(strlen(this.buffer)<1) return false;
		var dest="";
		
		if(this.buffer[0]==";") {
			if(this.buffer.length>1 && (user.compare_ars("SYSOP") || bbs.sys_status&SS_TMPSYSOP)) {
				str_cmds(this.buffer.substr(1));
				this.reset();
				return false;
			}
		} else if(this.buffer.match(/^\/msg/i)) {
			var buff=this.buffer.substr(5);
			dest=getFirstWord(buff);
			this.buffer=removeSpaces(buff.substr(dest.length+1));
			this.scope=flag_private;
		} else if(this.scope==flag_global) {
			dest="#global";
		}
		
		var msg=new Message(this.buffer,flag_message,user.alias,dest);
		this.reset();
		this.clear();
		return msg;
	}
	this.toggle=function()
	{
		if(this.scope==flag_global) this.scope=flag_normal;
		else this.scope=flag_global;
		if(this.box) this.initbox();
		this.draw();
	}
	this.backspace=function()
	{
		if(this.buffer.length>0) {
			if(!this.width>0) {
				console.left();
				console.cleartoeol();
				this.buffer=this.buffer.substr(0,this.buffer.length-1);
			} else if(this.buffer.length<=this.width) {
				this.getxy();
				console.left();
				console.putmsg(" ",P_SAVEATR);
				this.buffer=this.buffer.substr(0,this.buffer.length-1);
			} else {
				this.buffer=this.buffer.substr(0,this.buffer.length-1);
				this.draw();
			}
			return true;
		} else {
			return false;
		}
	}
	this.reset=function()
	{
		this.buffer="";
		if(this.scope==flag_private) {
			this.scope=flag_normal;
			if(this.box) this.initbox();
		}
	}
	this.getxy=function()
	{
		console.gotoxy(this.x+this.buffer.length,this.y);
	}
	this.bufferKey=function(key)
	{
		if(!key) return false;
		if(this.width>0) {
			if(this.buffer.length>=this.width) {
				this.buffer+=key;
				this.draw();
				return;
			} else {
				this.getxy();
			}
		}
		this.buffer+=key;
		var color=this.fg;
		switch(this.scope) {
			case flag_global:
				color=global_color;
				break;
			case flag_private:
				color=private_color;
				break;
			case flag_normal:
			default:	
				break;
		}
		console.putmsg(color+this.bg,P_SAVEATR);
		console.write(key);
	}
	this.draw=function()
	{
		if(this.box) this.box.draw();
		if(this.buffer.length<1) {
			this.clear();
			return;
		}
		var color=this.fg;
		switch(this.scope) {
			case flag_global:
				color=global_color;
				break;
			case flag_private:
				color=private_color;
				break;
			case flag_normal:
			default:
				break;
		}
		if(this.width>0) {
			console.putmsg(color + this.bg,P_SAVEATR);
			console.gotoxy(this);
			if(this.buffer.length>this.width) {
				var overrun=(this.buffer.length-this.width);
				var truncated=this.buffer.substr(overrun);
				if(truncated.indexOf('@')>=0) truncated=truncated.replace(/@/g,"?");
				console.write(truncated);
			} else {
				console.write(printPadded(this.buffer,this.width));
			}
		} else {
			console.putmsg("\r" + color + this.bg,P_SAVEATR);
			console.write(this.buffer);
		}
	}
}

