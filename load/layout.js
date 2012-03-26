/* $Id$ */
/* Window-style layout library for Synchronet 3.15+ 
 * 
 * NOTE: frame.js is required to use this library
 * if a Frame() object is not supplied to a Layout()
 * on creation, one will be created internally, however
 * it is recommended that you create an external frame object
 * as a basis for any other libraries that use frames.
 * 
 * NOTE: There is no real need to manually draw the contents of a layout
 * as long as either Frame.cycle() or Layout.cycle() are part of your
 * loop. If you use "blocking" operations with a layout or frame, 
 * then use Layout.draw() or Frame.draw() to update your views.
 * 
 * Layout methods:
 * 
 * 	addView(title,x,y,width,height,frame) 
 * 	getViewByTitle(title)
 * 	draw()
 * 	cycle()
 * 	getcmd(cmd)
 * 
 * Layout properties:
 * 
 * 	x
 * 	y
 * 	width
 * 	height
 * 	frame
 * 	colors = {
 * 		border_bg,
 * 		border_fg,
 * 		title_bg,
 *		title_fg, 
 *		tab_bg,
 *		tab_fg,
 *		view_bg,
 *		view_fg
 *  }
 * 
 * LayoutView methods:
 * 
 * 	addTab(title,frame)
 * 	getTabByTitle(title)
 * 	draw()
 * 	cycle()
 * 	getcmd(cmd)
 * 
 * LayoutView properties:
 * 
 * 	x
 * 	y
 * 	width
 * 	height
 * 	title
 * 	frame
 * 	colors (a reference to the parent layout's "colors" property)
 *  
 * LayoutView toggles: 
 * 
 * NOTE: changing these values will resize and reinitialize 
 * the view's components
 * 
 * 	show_border = (true|false)
 * 	show_title = (true|false)
 * 	show_tabs = (true|false)
 * 
 * 
 * 
 */

/* main layout object, intended to contain child layout view objects */ 
function Layout(frame) {
	
	/* private properties */
	var properties={
		views:[],
		index:0
	};
	var frames={
		main:undefined
	}
	
	/* read-only properties */
	this.__defineGetter__("x",function() {
		return frames.main.x;
	});
	this.__defineGetter__("y",function() {
		return frames.main.y;
	});
	this.__defineGetter__("width",function() {
		return frames.main.width;
	});
	this.__defineGetter__("height",function() {
		return frames.main.height;
	});
	this.__defineGetter__("frame",function() {
		return frames.main;
	});
	
	/* settings */
	this.__defineGetter__("current",function() {
		return properties.views[properties.index];
	});
	this.__defineSetter__("current",function(index) {
		properties.index=index;
		properties.views[properties.index].active=true;
	});
	
	/* public properties */
	this.colors={
		title_bg:BG_BLUE,
		title_fg:YELLOW,
		inactive_title_bg:BG_BLACK,
		inactive_title_fg:LIGHTGRAY,
		tab_bg:BG_GREEN,
		tab_fg:LIGHTGREEN,
		inactive_tab_bg:BG_LIGHTGRAY,
		inactive_tab_fg:BLACK,
		view_bg:BG_BLACK,
		view_fg:GREEN,
		border_bg:BG_BLACK,
		border_fg:LIGHTGRAY
	};

	/* public methods */
	this.open=function() {
		this.current=0;
		frames.main.open();
		for each(var v in properties.views)
			v.open();
	}
	this.addView=function(title,x,y,w,h) {
		var f = new Frame(x,y,w,h,undefined,frames.main);
		var v = new LayoutView(title,f,this);
		properties.views.push(v);
		return v;
	}
	this.draw=function() {
		for each(var view in properties.views)
			view.draw();
	}
	this.cycle=function() {
		for each(var v in properties.views)
			v.cycle();
	}
	this.getViewByName=function(title) {
		for each(var v in properties.views) {
			if(v.title.toUpperCase() == title.toUpperCase())
				return v;
		}
	}
	
	/* default command handler for views */
	this.getcmd=function(cmd) {
		if(!cmd) 
			return false;
		switch(cmd.toUpperCase()) {
		case "\t":
			if(properties.views.length > 1) {
				properties.views[properties.index].active = false;
				properties.index++;
				if(properties.index >= properties.views.length)
					properties.index = 0;
				properties.views[properties.index].active = true;
			}
			return true;
		default:
			if(properties.views.length > 0)
				return properties.views[properties.index].getcmd(cmd);
			break;
		}
		return false;
	}
	
	/* constructor */
	function init(frame) {
		if(frame instanceof Frame)
			frames.main = frame;
		else
			frames.main = new Frame();
	}
	init.call(this,frame);
}

/* layout view object, holds view tab objects
 * NOTE: this can be instantiated independently from Layout, 
 * but must be supplied a frame with the desired dimensions,
 * normally instantiated via Layout.addView() */
function LayoutView(title,frame,parent) {
	/* private properties */
	var properties={
		title:undefined,
		index:0,
		tabs:[],
	}
	var relations={
		parent:undefined
	}
	var settings={
		show_title:true,
		show_tabs:true,
		show_border:true,
		active:false
	}
	var frames={
		main:undefined,
		border:undefined,
		title:undefined,
		tabs:undefined,
		content:undefined
	};
	
	/* read-only properties */
	this.__defineGetter__("x",function() {
		return frames.main.x;
	});
	this.__defineGetter__("y",function() {
		return frames.main.y;
	});
	this.__defineGetter__("width",function() {
		return frames.main.width;
	});
	this.__defineGetter__("height",function() {
		return frames.main.height;
	});
	this.__defineGetter__("colors",function() {
		return relations.parent.colors;
	});
	this.__defineGetter__("title",function() {
		return properties.title;
	});
	this.__defineGetter__("frame",function() {
		return frames.main;
	});
	this.__defineGetter__("tabs",function() {
		return properties.tabs;
	});
	
	/* settings */
	this.__defineGetter__("show_title",function() {
		return settings.show_title;
	});
	this.__defineSetter__("show_title",function(bool) {
		if(typeof bool !== "boolean")
			return false;
		settings.show_title = bool;
		updateViewFrames();
		return true;
	});
	this.__defineGetter__("show_tabs",function() {
		return settings.show_tabs;
	});
	this.__defineSetter__("show_tabs",function(bool) {
		if(typeof bool !== "boolean")
			return false;
		settings.show_tabs = bool;
		updateViewFrames();
		return true;
	});
	this.__defineGetter__("show_border",function() {
		return settings.show_border;
	});
	this.__defineSetter__("show_border",function(bool) {
		if(typeof bool !== "boolean")
			return false;
		settings.show_border = bool;
		updateViewFrames();
		return true;
	});
	this.__defineGetter__("current",function() {
		return properties.tabs[properties.index];
	});
	this.__defineSetter__("current",function(index) {
		if(properties.tabs[index]) {
			if(properties.index == index)
				return false;
			if(properties.tabs[properties.index])
				properties.tabs[properties.index].active=false;
			properties.index=index;
			properties.tabs[properties.index].active=true;
			setTabs();
			return true;
		}
		return false;
	});
	this.__defineSetter__("active",function(bool) {
		if(typeof bool !== "boolean") 
			return false;
		settings.active = bool;
		if(settings.active)
			frames.title.attr = this.colors.title_bg+this.colors.title_fg;
		else
			frames.title.attr = this.colors.inactive_title_bg + this.colors.inactive_title_fg;
		setTitle();
		return true;
	});
	
	/* public methods */
	this.open=function() {
		for each(var t in properties.tabs) 
			if(typeof t.open == "function")
				t.open();
		this.current=0;
		setViewFrames();
	}
	this.draw=function() {
		frames.main.draw();
	}
	this.cycle=function() {
		for each(var t in properties.tabs)
			if(typeof t.cycle == "function")
				t.cycle();
	}
	this.addTab=function(title,type,content) {
		/* use this view's location and dimensions as 
		starting point for new tabs */
		var x = frames.content.x;
		var y = frames.content.y;
		var w = frames.content.width;
		var h = frames.content.height;
		var attr = this.colors.view_bg + this.colors.view_fg;
		var f = new Frame(x,y,w,h,attr,frames.content);
		var t = new ViewTab(title,f,this);	
		f.open();
		setContent(t,type,content);
		properties.tabs.push(t);
		if(this.current)
			this.current.active=true;
		setTabs();
		return t;
	}
	this.getTab=function(title_or_index) {
		if(isNaN(title_or_index)) {
			for each(var t in properties.tabs) {
				if(t.title.toUpperCase() == title.toUpperCase())
					return t;
			}
		}
		else {
			return properties.tabs[title_or_index];
		}
	}
	this.delTab=function(title_or_index) {
		var tab = false;
		if(isNaN(title_or_index)) {
			for(var t=0;t<properties.tabs.length;t++) {
				if(properties.tabs[t].title.toUpperCase() == title.toUpperCase()) {
					tab = properties.tabs[t];
					properties.tabs.splice(t,1);
				}
			}
		}
		else if(properties.tabs[title_or_index]) {
			tab = properties.tabs[title_or_index];
			properties.tabs.splice(title_or_index,1);
		}
		if(tab) {
			tab.frame.delete();
			while(!properties.tabs[properties.index] && properties.index > 0)
				properties.index--;
			if(this.current)
				this.current.active=true;
			setTabs();
		}
		return tab;
	}
	this.getcmd=function(cmd) {
		if(!cmd) 
			return false;
		switch(cmd.toUpperCase()) {
		case KEY_LEFT:
			if(properties.tabs.length > 1) {
				properties.tabs[properties.index].active = false;
				properties.index--;
				if(properties.index < 0)
					properties.index = properties.tabs.length-1;
				properties.tabs[properties.index].active = true;
				setTabs();
			}
			break;
		case KEY_RIGHT:
			if(properties.tabs.length > 1) {
				properties.tabs[properties.index].active = false;
				properties.index++;
				if(properties.index >= properties.tabs.length)
					properties.index = 0;
				properties.tabs[properties.index].active = true;
				setTabs();
			}
			break;
		default:
			if(properties.tabs.length > 0)
				return properties.tabs[properties.index].getcmd(cmd);
			break;
		}
	}
	
	/* private methods */
	function setContent(tab,type,content) {
		if(!type)
			return false;
		switch(type.toUpperCase()) {
		case "TREE":
			if(typeof Tree == "undefined")
				load("tree.js");
			if(content instanceof Tree)  {
				tab.tree = content;
				tab.tree.frame = tab.frame;
				tab.tree.refresh();
			}
			else {
				tab.tree = new Tree(tab.frame);
				tab.tree.refresh();
			}
			tab.getcmd = function(cmd) {
				return this.tree.getcmd.call(tab.tree,cmd);
			}
			break;
		case "CHAT":
			if(typeof JSONChat == "undefined")
				load("json-chat.js");
			if(content instanceof JSONChat) 
				tab.chat = content;
			else
				tab.chat = new JSONChat();
			tab.getcmd = function(cmd) {
				switch(cmd.toUpperCase()) {
				case KEY_UP:
					return this.frame.scroll(0,-1);
				case KEY_DOWN:
					return this.frame.scroll(0,1);
				default:
					return this.chat.submit.call(this.chat,this.title,cmd);
				}
			}
			tab.cycle = function() {
				var chan = this.chat.channels[this.title.toUpperCase()];
				if(!chan) {
					this.parent.delTab(this.title);
					return false;
				}
				while(chan.messages.length > 0) {
					var msg = chan.messages.shift();
					var str = "";
					if(msg.nick)
						var str = getColor(this.chat.settings.NICK_COLOR) + msg.nick.name + "\1n: " + 
						getColor(this.chat.settings.TEXT_COLOR) + msg.str;
					else
						var str = getColor(this.chat.settings.NOTICE_COLOR) + msg.str;
					this.frame.putmsg(str + "\r\n");
				}
			}
			properties.chat = tab.chat;
			break;
		case "GRAPHIC":
			//ToDo
			break;
		default:
			return false;
		}
		return true;
	}
	function updateViewFrames() {
		var x = frames.main.x;
		var y = frames.main.y;
		var w = frames.main.width;
		var h = frames.main.height;
		var colors = relations.parent.colors;
		
		/* delete all existing content frames */
		if(frames.border)
			frames.border.delete();
		if(frames.title) 
			frames.title.delete();
		if(frames.tabs) 
			frames.tabs.delete();
		if(frames.content) 
			frames.content.delete();
		/* recreate any frames set to be displayed */
		if(settings.show_border) {
			var attr = colors.border_bg + colors.border_fg;
			frames.border = new Frame(x,y,w,h,attr,frames.main);
			x++;
			y++;
			w-=2;
			h-=2;
		}
		if(settings.show_title) {
			var attr = colors.inactive_title_bg + colors.inactive_title_fg;
			frames.title = new Frame(x,y,w,1,attr,frames.main);
			y++;
			h--;
		}
		if(settings.show_tabs) {
			var attr = colors.inactive_tab_bg + colors.inactive_tab_fg;
			frames.tabs = new Frame(x,y,w,1,attr,frames.main);
			y++;
			h--;
		}

		frames.content = new Frame(x,y,w,h,undefined,frames.main); 
	}
	function setViewFrames() {
		if(settings.show_border)
			setBorder();
		if(settings.show_title)
			setTitle();
		if(settings.show_tabs) 
			setTabs();
	}
	function setBorder() {
		var f = frames.border;
		var h = "\xC4";
		var v = "\xB3";
		var tl = "\xDA";
		var tr = "\xBF";
		var bl = "\xC0";
		var br = "\xD9";
		var hline = "";
		for(var x=0;x<frames.main.width-2;x++)
			hline+=h;
		var vline = format(v + "%*s" + v,frames.main.width-2,"");
		f.home();
		f.putmsg(tl + hline + tr + "\r\n");
		for(var y=0;y<frames.main.height-2;y++)
			f.putmsg(vline + "\r\n");
		f.putmsg(bl + hline + br);
	}
	function setTabs() {
		var f = frames.tabs;
		var w = f.width;
		var max_width=w-2;
		var left_arrow = "<";
		var right_arrow = ">";
		var colors = relations.parent.colors;
		var inact_color = "\1n" + getColor(colors.inactive_tab_bg) + getColor(colors.inactive_tab_fg);
		var act_color = "\1n" + getColor(colors.tab_bg) + getColor(colors.tab_fg);
		var arrow_color = "\1r\1h";

		/* populate string with current tab highlight */
		var tab_str="";
		if(properties.tabs.length > 0) {
			tab_str+=act_color + properties.tabs[properties.index].title + inact_color;
			
			/* if there are items below the current index */
			var t=properties.index-1;
			while(t >= 0 && console.strlen(tab_str) < max_width) {
				if(console.strlen(tab_str)+properties.tabs[t].title.length+1 > max_width) 
					break;
				tab_str=properties.tabs[t].title + " " + tab_str;
				t--;
			}
			if(t >= 0) 
				left_arrow = arrow_color + left_arrow;
				
			/* if there are items above the current index */
			t=properties.index+1;
			while(t < properties.tabs.length && console.strlen(tab_str) < max_width) {
				if(console.strlen(tab_str)+properties.tabs[t].title.length+1 > max_width) 
					break;
				tab_str=
					tab_str + " " +	properties.tabs[t].title;
				t++;
			}
			if(t < properties.tabs.length) 
				right_arrow = arrow_color + right_arrow;
		}
		tab_str+=format("%-*s",max_width-console.strlen(tab_str),"");
		f.home();
		f.putmsg(left_arrow + tab_str + right_arrow);
	}
	function setTitle() {
		frames.title.clear();
		frames.title.center(properties.title);
	}
	
	/* constructor */
	function init(title,frame,parent) {
		properties.title = title;
		if(frame instanceof Frame)
			frames.main = frame;
		else
			frames.main = new Frame();
		relations.parent = parent;
		updateViewFrames();
		setViewFrames();
	}
	init.call(this,title,frame,parent);
}

/* view tab object, meant to inhabit a layout view.
 * will generally occupy the same space as other view tabs
 * within a given view, cannot be effectively instantiated
 * on its own, but rather through ViewFrame.addTab() */
function ViewTab(title,frame,parent) {
	/* private properties */
	var properties={
		title:undefined,
		content:undefined
	}
	var settings={
		active:false
	}
	var frames={
		main:undefined
	}
	var relations={
		parent:undefined
	}

	/* read-only properties */
	this.__defineGetter__("x",function() {
		return frames.main.x;
	});
	this.__defineGetter__("y",function() {
		return frames.main.y;
	});
	this.__defineGetter__("width",function() {
		return frames.main.width;
	});
	this.__defineGetter__("height",function() {
		return frames.main.height;
	});
	this.__defineGetter__("colors",function() {
		return relations.parent.colors;
	});
	this.__defineGetter__("title",function() {
		return properties.title;
	});
	this.__defineGetter__("frame",function() {
		return frames.main;
	});
	this.__defineGetter__("parent",function() {
		return relations.parent;
	});
	
	/* settings */
	this.__defineSetter__("active",function(bool) {
		if(typeof bool !== "boolean") 
			return false;
		settings.active = bool;
		if(settings.active) 
			frames.main.top();
		return true;
	});

	/* default command handler 
	 * (can be overridden for specialized tabs) */
	this.getcmd = function(cmd) {
		if(!cmd)
			return false;
		frames.main.putmsg(cmd);
		return true;
	}
	
	/* constructor */
	function init(title,frame,parent) {
		properties.title = title;
		frames.main = frame;
		relations.parent = parent;
	}
	init.call(this,title,frame,parent);
}
