// $Id: graphic.js,v 1.63 2018/01/01 22:46:01 rswindell Exp $

/*
 * "Graphic" object
 * Allows a graphic to be stored in memory and portions of it redrawn on command
 */

/*
 * Class that represents a drawable object.
 * w = width (default of 80)
 * h = height (default of 24)
 * attr = default attribute (default of 7 ie: LIGHTGREY)
 * ch = default character (default of space)
 *
 * Instance variable data contains an array of array of Graphics.Cell objects
 *
 * Instance variables atcodes are slated for removal.
 */
function Graphic(w,h,attr,ch)
{
	if(ch==undefined)
		this.ch=' ';
	else
		this.ch=ch;
	if(attr==undefined)
		this.attribute=7;
	else
		this.attribute=attr;
	if(h==undefined)
		this.height=24;
	else
		this.height=h;
	if(w==undefined)
		this.width=80;
	else
		this.width=w;

	this.atcodes=true;

	this.data=new Array(this.width);
	for(var y=0; y<this.height; y++) {
		for(var x=0; x<this.width; x++) {
			if(y==0) {
				this.data[x]=new Array(this.height);
			}
			this.data[x][y]=new this.Cell(this.ch,this.attribute);
		}
	}
}

/*
 * Load sbbsdefs.js into Graphic.defs
 */
Graphic.prototype.defs = {};
load(Graphic.prototype.defs, "sbbsdefs.js");

/*
 * Load ansiterm_lib into Graphic.ansi
 */
Graphic.prototype.ansi = {};
load(Graphic.prototype.ansi, "ansiterm_lib.js");

/*
 * The Cell subclass which contains the attribute and character for a
 * single cell.
 */
Graphic.prototype.Cell = function(ch,attr)
{
	this.ch=ch;
	this.attr=attr;
};

/*
 * BIN property is the string representation of the graphic in a series of
 * char/attr pairs.
 */
Object.defineProperty(Graphic.prototype, "BIN", {
	get: function() {
		var bin = '';
		var x;
		var y;

		for (y=0; y<this.height; y++) {
			for (x=0; x<this.width; x++) {
				bin += this.data[x][y].ch;
				bin += ascii(this.data[x][y].attr);
			}
		}
		return bin;
	},
	set: function(bin) {
		var x;
		var y;
		var pos = 0;
		var blen = bin.length;

		for (y=0; y<this.height; y++) {
			for (x=0; x<this.width; x++) {
				if (blen >= pos+2)
					this.data[x][y] = new this.Cell(bin.charAt(pos), bin.charCodeAt(pos+1));
				else
					return;
				pos += 2;
			}
		}
	}
});

/*
 * ANSI property is the string representation of the graphic as ANSI
 * 
 * NOTE: Reading the ANSI property generates ANSI sequences which can put
 *       printable characters in the last column of the display (e.g. col 80)
 *       potentially resulting in extra blank lines in some terminals.
 *       To change this behavior, decrement the 'width' property first.
 */
Object.defineProperty(Graphic.prototype, "ANSI", {
	get: function() {
		var x;
		var y;
    	var lines=[];
    	var curattr=7;
		var ansi = '';
		var char;

		for(y=0; y<this.height; y++) {
			for(x=0; x<this.width; x++) {
				ansi += this.ansi.attr(this.data[x][y].attr, curattr);
            	curattr = this.data[x][y].attr;
            	char = this.data[x][y].ch;
            	/* Don't put printable chars in the last column */
//            	if(char == ' ' || (x<this.width-1))
                	ansi += char;
        	}
			ansi += '\r\n';
    	}
    	return ansi;
	},
	set: function(ans) {
		var attr = this.attribute;
		var saved = {};
		var x = 0;
		var y = 0;
		var std_cmds = {
			'm':function(params, obj) {
				var bg = attr & obj.defs.BG_LIGHTGRAY;
				var fg = attr & obj.defs.LIGHTGRAY;
				var hi = attr & obj.defs.HIGH;
				var bnk = attr & obj.defs.BLINK;

				if (params[0] === undefined || params[0] === '')
					params[0] = 0;

				while (params.length) {
					switch (parseInt(params[0], 10)) {
						case 0:
							bg = 0;
							fg = obj.defs.LIGHTGRAY;
							hi = 0;
							bnk = 0;
							break;
						case 1:
							hi = obj.defs.HIGH;
							break;
						case 40:
							bg = 0;
							break;
						case 41:
							bg = obj.defs.BG_RED;
							break;
						case 42: 
							bg = obj.defs.BG_GREEN;
							break;
						case 43:
							bg = obj.defs.BG_BROWN;
							break;
						case 44:
							bg = obj.defs.BG_BLUE;
							break;
						case 45:
							bg = obj.defs.BG_MAGENTA;
							break;
						case 46:
							bg = obj.defs.BG_CYAN;
							break;
						case 47:
							bg = obj.defs.BG_LIGHTGRAY;
							break;
						case 30:
							fg = obj.defs.BLACK;
							break;
						case 31:
							fg = obj.defs.RED;
							break;
						case 32:
							fg = obj.defs.GREEN;
							break;
						case 33:
							fg = obj.defs.BROWN;
							break;
						case 34:
							fg = obj.defs.BLUE;
							break;
						case 35:
							fg = obj.defs.MAGENTA;
							break;
						case 36:
							fg = obj.defs.CYAN;
							break;
						case 37:
							fg = obj.defs.LIGHTGRAY;
							break;
					}
					params.shift();
				}
				attr = bg + fg + hi + bnk;
			},
			'H':function(params, obj) {
				if (params[0] === undefined || params[0] === '')
					params[0] = 1;
				if (params[1] === undefined || params[1] === '')
					params[1] = 1;

				y = parseInt(params[0], 10) - 1;
				if (y < 0)
					y = 0;
				if (y >= obj.height)
					y = obj.height-1;

				x = parseInt(params[1], 10) - 1;
				if (x < 0)
					x = 0;
				if (x >= obj.width)
					x = obj.width-1;
			},
			'A':function(params) {
				if (params[0] === undefined || params[0] === '')
					params[0] = 1;
	
				y -= parseInt(params[0], 10);
				if (y < 0)
					y = 0;
			},
			'B':function(params, obj) {
				if (params[0] === undefined || params[0] === '')
					params[0] = 1;

				y += parseInt(params[0], 10);
				if (y >= obj.height)
					y = obj.height-1;
			},
			'C':function(params, obj) {
				if (params[0] === undefined || params[0] === '')
					params[0] = 1;

				x += parseInt(params[0], 10);
				if (x >= obj.width)
					x = obj.width - 1;
			},
			'D':function(params) {
				if (params[0] === undefined || params[0] === '')
					params[0] = 1;

				x -= parseInt(params[0], 10);
				if (x < 0)
					x = 0;
			},
			'J':function(params,obj) {
				if (params[0] === undefined || params[0] === '')
					params[0] = 0;

				if (parseInt(params[0], 10) == 2)
					obj.clear();
			},
			's':function(params) {
				saved={'x':x, 'y':y};
			},
			'u':function(params) {
				x = saved.x;
				y = saved.y;
			}
		};
		std_cmds.f = std_cmds.H;
		var line;
		var m;
		var paramstr;
		var cmd;
		var params;
		var ch;

		/* parse 'ATCODES'?? 
		ans = ans.replace(/@(.*)@/g,
			function (str, code, offset, s) {
				return bbs.atcode(code);
			}
		);
		*/
		while(ans.length > 0) {
			m = ans.match(/^\x1b\[([\x30-\x3f]*)([\x20-\x2f]*[\x40-\x7e])/);
			if (m !== null) {
				paramstr = m[1];
				cmd = m[2];
				if (paramstr.search(/^[<=>?]/) != 0) {
					params=paramstr.split(/;/);

					if (std_cmds[cmd] !== undefined)
						std_cmds[cmd](params,this);
				}
				ans = ans.substr(m[0].length);
   	            continue;
			}

			/* set character and attribute */
			ch = ans[0];
			ans = ans.substr(1);

	        /* Handle non-ANSI cursor positioning control characters */
			switch(ch) {
                case '\r':
                    x=0;
                    break;
                case '\n':
                    y++;
                    break;
                case '\t':
                    x += 8-(x%8);
                    break;
                case '\b':
                    if(x > 0)
						x--;
            		break;
                default:
			        this.data[x][y]=new this.Cell(ch,attr);
			        x++;
                    break;
            }
			/* validate position/scroll */
			if (x < 0)
				x = 0;
			if (y < 0)
				y = 0;
			if(x>=this.width) {
				x=0;
				y++;
			}
			while(y >= this.height) {
				this.scroll();
				y--;
			}
		}
	}
});

/*
 * Returns an HTML encoding of the graphic data.
 *
 * TODO: get rid of the ANSI phase as unnecessary
 */
Object.defineProperty(Graphic.prototype, "HTML", {
	get: function() {
        return html_encode(this.ANSI, /* ex-ascii: */true, /* whitespace: */false, /* ansi: */true, /* Ctrl-A: */false);
    }
});

/*
 * Resets the graphic to all this.ch/this.attr Cells
 */
Graphic.prototype.clear = function()
{
	this.data=new Array(this.width);
	for(var y=0; y<this.height; y++) {
		for(var x=0; x<this.width; x++) {
			if(y==0) {
				this.data[x]=new Array(this.height);
			}
			this.data[x][y]=new this.Cell(this.ch,this.attribute);
		}
	}
};

/*
 * Draws the graphic using the console object optionally delaying between
 * characters.
 *
 * TODO: Remove optional delay.  If the functionality is needed, maybe add
 *       a callback.
 */
Graphic.prototype.draw = function(xpos,ypos,width,height,xoff,yoff,delay)
{
	var x;
	var y;

	if(xpos==undefined)
		xpos=1;
	if(ypos==undefined)
		ypos=1;
	if(width==undefined)
		width=this.width;
	if(height==undefined)
		height=this.height;
	if(xoff==undefined)
		xoff=0;
	if(yoff==undefined)
		yoff=0;
	if(delay==undefined)
		delay=0;
	if(xoff+width > this.width || yoff+height > this.height) {
		alert("Attempt to draw from outside of graphic: "+xoff+":"+yoff+" "+width+"x"+height+" "+this.width+"x"+this.height);
		return(false);
	}
	if(xpos+width-1 > console.screen_columns || ypos+height-1 > console.screen_rows) {
		alert("Attempt to draw outside of screen: " + (xpos+width-1) + "x" + (ypos+height-1));
		return(false);
	}
	for(y=0;y<height; y++) {
		console.gotoxy(xpos,ypos+y);
		for(x=0; x<width; x++) {
			// Do not draw to the bottom left corner of the screen-would scroll
			if(xpos+x != console.screen_columns
					|| ypos+y != console.screen_rows) {
				console.attributes=this.data[x+xoff][y+yoff].attr;
				var ch=this.data[x+xoff][y+yoff].ch;
				if(ch == "\r" || ch == "\n" || !ch)
					ch=this.ch;
				console.write(ch);
			}
		}
		if(delay)
			mswait(delay);
	}
	return(true);
};

/*
 * Does a random draw of the graphic...
 * Maybe this should stay, maybe not.  TODO: Decide.
 */
Graphic.prototype.drawfx = function(xpos,ypos,width,height,xoff,yoff)
{
	var x;
	var y;

	if(xpos==undefined)
		xpos=1;
	if(ypos==undefined)
		ypos=1;
	if(width==undefined)
		width=this.width;
	if(height==undefined)
		height=this.height;
	if(xoff==undefined)
		xoff=0;
	if(yoff==undefined)
		yoff=0;
	if(xoff+width > this.width || yoff+height > this.height) {
		alert("Attempt to draw from outside of graphic: "+xoff+":"+yoff+" "+width+"x"+height+" "+this.width+"x"+this.height);
		return(false);
	}
	if(xpos+width-1 > console.screen_columns || ypos+height-1 > console.screen_rows) {
		alert("Attempt to draw outside of screen");
		return(false);
	}
	var placeholder=new Array(width);
	for(x=0;x<width;x++) {
		placeholder[x]=new Array(height);
		for(y=0;y<placeholder[x].length;y++) {
			placeholder[x][y]={'x':xoff+x,'y':yoff+y};
		}
	}
	var count=0;
	var interval=10;
	while(placeholder.length) {
		count++;
		if(count == interval) {
			count=0;
			mswait(15);
		}
		var randx=random(placeholder.length);
		if(!placeholder[randx] || !placeholder[randx].length) {
			placeholder.splice(randx,1);
			continue;
		}
		var randy=random(placeholder[randx].length);
		var position=placeholder[randx][randy];
		if(!position) 
			continue;
		if(position.x != console.screen_columns	|| position.y != console.screen_rows) {
			if(xpos+position.x >= console.screen_columns && ypos+position.y >= console.screen_rows) {
				placeholder[randx].splice(randy,1);
				continue;
			}
			console.gotoxy(xpos+position.x,ypos+position.y);
			console.attributes=this.data[position.x][position.y].attr;
			var ch=this.data[position.x][position.y].ch;
			if(ch == "\r" || ch == "\n" || !ch)
				ch=this.ch;
			console.write(ch);
		}
		placeholder[randx].splice(randy,1);
	}
	console.home();
	return(true);
};

/*
 * Loads a ANS or BIN file.
 * TODO: The ASC load is pretty much guaranteed to be broken.
 */
Graphic.prototype.load = function(filename)
{
	var file_type=file_getext(filename).substr(1);
	var f=new File(filename);
	var ch;
	var attr;
	var l;

	switch(file_type.toUpperCase()) {
	case "ANS":
		if(!(f.open("rb",true)))
			return(false);
		this.ANSI = f.read();
		f.close();
		break;
	case "BIN":
		if(!(f.open("rb",true)))
			return(false);
		this.BIN = f.read();
		f.close();
		break;
	case "ASC":
		if(!(f.open("r",true,4096)))
			return(false);
		var lines=f.readAll();
		f.close();
		for (l in lines)
			this.putmsg(undefined,undefined,l,true);
		break;
	default:
		throw("unsupported file type:" + filename);
	}
	return(true);
};

/*
 * The inverse of load()... saves a BIN representation to a file.
 */
Graphic.prototype.save = function(filename)
{
	var x;
	var y;
	var f=new File(filename);

	if(!(f.open("wb",true)))
		return(false);
	f.write(this.BIN);
	f.close();
	return(true);
};

/*
 * Moves the image in the graphic.
 * Scrolling up by one is needed for the putmsg implementation.
 *
 * TODO: Doesn't belong here, but lbshell.js uses it.
 */
Graphic.prototype.scroll = function(count)
{
	var x;
	var i;

	if (count === undefined)
		count = 1;
	if (count < 0)
		return false;
	for (x=0; x<this.width; x++) {
		for (i=0; i<count; i++) {
			this.data[x].shift();
			this.data[x].push(new this.Cell(this.ch, this.attribute));
		}
	}
	return true;
};

/*
 * Emulates console.putmsg() into a graphic object.
 * Supports @-codes.
 * Returns the number of times the graphic was scrolled.
 *
 * TODO: Doesn't belong here, but lbshell.js uses it.
 */
Graphic.prototype.putmsg = function(xpos, ypos, txt, attr, scroll)
{
	var curattr=attr;
	var ch;
	var x=xpos?xpos-1:0;
	var y=ypos?ypos-1:0;
	var p=0;
	var scrolls=0;

	if(curattr==undefined)
		curattr=this.attribute;
	/* Expand @-codes */
	if(txt==undefined || txt==null || txt.length==0) {
		return(0);
	}
	if(this.atcodes) {
		txt=txt.toString().replace(/@(.*)@/g,
			function (str, code, offset, s) {
				return bbs.atcode(code);
			}
		);
	}
	
	/* wrap text at graphic width */
	txt=word_wrap(txt,this.width);
	
	/* ToDo: Expand \1D, \1T, \1<, \1Z */
	/* ToDo: "Expand" (ie: remove from string when appropriate) per-level/per-flag stuff */
	/* ToDo: Strip ANSI (I betcha @-codes can slap it in there) */
	while(p<txt.length) {
		while(y>=this.height) {
			if(!scroll) {
				alert("Attempt to draw outside of screen");
				return false;
			}
			scrolls++;
			this.scroll();
			y--;
		}
		
		ch=txt[p++];
		switch(ch) {
			case '\1':		/* CTRL-A code */
				ch=txt[p++].toUpperCase();
				switch(ch) {
					case '\1':	/* A "real" ^A code */
						this.data[x][y].ch=ch;
						this.data[x][y].attr=curattr;
						x++;
						if(x>=this.width) {
							x=0;
							y++;
							log("next char: [" + txt[p] + "]");
							if(txt[p] == '\r') p++;
							if(txt[p] == '\n') p++;
						}
						break;
					case 'K':	/* Black */
						curattr=(curattr)&0xf8;
						break;
					case 'R':	/* Red */
						curattr=((curattr)&0xf8)|this.defs.RED;
						break;
					case 'G':	/* Green */
						curattr=((curattr)&0xf8)|this.defs.GREEN;
						break;
					case 'Y':	/* Yellow */
						curattr=((curattr)&0xf8)|this.defs.BROWN;
						break;
					case 'B':	/* Blue */
						curattr=((curattr)&0xf8)|this.defs.BLUE;
						break;
					case 'M':	/* Magenta */
						curattr=((curattr)&0xf8)|this.defs.MAGENTA;
						break;
					case 'C':	/* Cyan */
						curattr=((curattr)&0xf8)|this.defs.CYAN;
						break;
					case 'W':	/* White */
						curattr=((curattr)&0xf8)|this.defs.LIGHTGRAY;
						break;
					case '0':	/* Black */
						curattr=(curattr)&0x8f;
						break;
					case '1':	/* Red */
						curattr=((curattr)&0x8f)|(this.defs.RED<<4);
						break;
					case '2':	/* Green */
						curattr=((curattr)&0x8f)|(this.defs.GREEN<<4);
						break;
					case '3':	/* Yellow */
						curattr=((curattr)&0x8f)|(this.defs.BROWN<<4);
						break;
					case '4':	/* Blue */
						curattr=((curattr)&0x8f)|(this.defs.BLUE<<4);
						break;
					case '5':	/* Magenta */
						curattr=((curattr)&0x8f)|(this.defs.MAGENTA<<4);
						break;
					case '6':	/* Cyan */
						curattr=((curattr)&0x8f)|(this.defs.CYAN<<4);
						break;
					case '7':	/* White */
						curattr=((curattr)&0x8f)|(this.defs.LIGHTGRAY<<4);
						break;
					case 'H':	/* High Intensity */
						curattr|=this.defs.HIGH;
						break;
					case 'I':	/* Blink */
						curattr|=this.defs.BLINK;
						break;
					case 'N':	/* Normal (ToDo: Does this do ESC[0?) */
						curattr=7;
						break;
					case '-':	/* Normal if High, Blink, or BG */
						if(curattr & 0xf8)
							curattr=7;
						break;
					case '_':	/* Normal if blink/background */
						if(curattr & 0xf0)
							curattr=7;
						break;
					case '[':	/* CR */
						x=0;
						break;
					case ']':	/* LF */
						y++;
						break;
					default:	/* Other stuff... specifically, check for right movement */
						if(ch.charCodeAt(0)>127) {
							x+=ch.charCodeAt(0)-127;
							if(x>=this.width)
								x=this.width-1;
						}
				}
				break;
			case '\7':		/* Beep */
				break;
			case '\r':
				x=0;
				break;
			case '\n':
				y++;
				break;
			default:
				this.data[x][y]=new this.Cell(ch,curattr);
				x++;
				if(x>=this.width) {
					x=0;
					y++;
					if(txt[p] == '\r') p++;
					if(txt[p] == '\n') p++;
				}
		}
	}
	return(scrolls);
};

/*
 * Returns an array of base 64 encoded strings.
 * Argument: the maximum length of the encoded lines (default: 76 chars)
 *
 * TODO: Delete
 */
Graphic.prototype.base64_encode = function(max_line_len)
{
	if(!max_line_len)
		max_line_len = 76;
	return base64_encode(this.BIN).match(new RegExp('([\x00-\xff]{1,' + max_line_len + '})', 'g'));
};

/*
 * Like parseANSI() for b64 data
 *
 * TODO: Delete
 */
Graphic.prototype.base64_decode = function(rows)
{
	this.BIN = base64_decode(rows.join(""));
};

/* Leave as last line for convenient load() usage: */
Graphic;
