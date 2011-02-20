/*	$id: $ 

	Lightbar Menu Tree 
	by Matt Johnson (MCMLXXIX) 2011
	
	usage example:
	
	// create a new tree in the top left corner; 40 columns 40 rows
	var tree=new Tree(1,1,40,10);
	
	// add an item with a hotkey of "i" and a return value of 1
	tree.addItem("|item one",1);

	// add a hidden item with no hotkey and a return value of 2
	tree.addItem("item two",2,item_status.hidden);

	// add a disabled item with a hotkey of "t" and a return value of "meh"
	tree.addItem("i|tem three","meh",item_status.disabled);

	// create a subtree with a hotkey of "m"
	var subtree=tree.addTree("|menu title");
	
	// add items to the subtree the same as before
	subtree.addItem("|logoff",bbs.hangup);

	// add an item with a hotkey of "e" and 
	// a return value of the result of function "getval"
	tree.addItem("it|em four",getval);
	
	// draw the tree 
	tree.draw();
		
*/

/* states for setting tree item visibility */
var item_status={
	enabled:1,
	disabled:0,
	hidden:-1
}

/* tree item color settings */
var tree_settings={
	// non-current item foreground
	fg:LIGHTGRAY,
	// non-current item/empty space background 
	bg:BG_BLACK,
	// current item foreground
	lfg:LIGHTCYAN,
	// current item background
	lbg:BG_BLUE,
	// disabled item foreground
	dfg:DARKGRAY,
	// hotkey foreground
	kfg:YELLOW,
	// tree branch foreground
	tfg:DARKGRAY,
	// tree heading foreground
	hfg:WHITE,
	// tree heading background
	hbg:BG_BLACK,
	// tree expansion foreground
	xfg:YELLOW
}

/* main tree object */
function Tree(xpos,ypos,w,h) {
	var x = xpos;
	var y = ypos;
	var width=w;
	var height=h;
	
	/*	only the top-level
		tree will have a set of coordinates,
		so we can assume it is the top
		if any are supplied here */
	if(x || y || width || height) 
		this.open=true;
	else
		this.open=false;
		
	/* 	these properties will only exist in a sub-tree */
	//	this.parent_index;
	//	this.parent;
	//	this.hotkey;
	
	this.status=item_status.enabled;
	this.text="";
	this.index=0;
	this.items=[];

	/* add a menu item */
	this.addItem=function(text,handle_command,status) {
		var item=new Tree_Item(text,handle_command,status);
		item.parent=this;
		item.parent_index=this.items.length;
		this.items.push(item);
		return item;
	}
	
	/* add a sub-tree */
	this.addTree=function(text) {
		var tree=new Tree();
		tree.text=text;
		tree.parent=this;
		tree.parent_index=this.items.length;
		this.items.push(tree);
		return tree;
	}
	
	/* toggle tree expansion */
	this.toggle=function() {
		this.open = (this.open ? false : true ); /* It's motherfucked. */
		this.index=-1;
	}
	
	/* display tree */
	this.draw=function(offset) {
		/* initialize list at top level tree only */
		if(this.depth == 0) {
			offset=" ";
			this.list=[];
			this.list.index=0;
		}
		/* if this is not the top tree, draw tree text */
		if(this.depth > 0) 
			this.post_tree(offset);
		/* if this tree is open, draw its items */
		if(this.open) 
			this.post_items(offset);
		/* draw list at top level tree only */
		if(this.depth == 0)
			this.draw_list();
	}
	
	/* display truncated item list */
	this.draw_list=function() {
		console.gotoxy(x,y);
		console.pushxy();
		var end=parseInt(this.list.index+(this.height/2),10);
		if(end > this.list.length-1)
			end=this.list.length-1;
		var index=end-this.height+1;
		if(index < 0) 
			index = 0;
		var l=0;
		for(;l<this.height && index < this.list.length;l++) {
			var str=this.list[index];
			console.putmsg(format("%-*s",this.width+(str.length-console.strlen(str)),str));
			console.popxy();
			console.down();
			console.pushxy();
			index++;
		}
		for(;l < this.height;l++) {
			console.putmsg(format("%*s",this.width,""));
			console.popxy();
			console.down();
			console.pushxy();
		}
	}
	
	/* draw tree items */
	this.post_items=function(offset) {
		var spacer="";
		for(var i=0;i<this.items.length;i++) {
			if(this.parent && this.parent.depth > 0) {
				if(this.parent_index == this.parent.items.length - 1)
					spacer=" ";
				else
					spacer="\xB3";
			}
			this.items[i].draw(offset+spacer);
		}
	}
	
	/* draw tree title (as item in parent tree) */
	this.post_tree=function(offset) {
		var str="";
		var curr=false;
		var bg="\1n" + getColor(tree_settings.bg);
		var fg=getColor(tree_settings.fg);
		/* if this is the current menu and it is either not open,
			or we arent highlighting one of its items */
		if(this.current && (this.index < 0 || !this.open)) {
			bg=getColor(tree_settings.lbg);
			fg=getColor(tree_settings.lfg);
			curr=true;
		}
		else if(this.status == item_status.disabled) {
			fg=getColor(tree_settings.dfg);
		}
		str+=bg;
		/* draw tree branches */
		if(this.depth > 1) 
			str+=getColor(tree_settings.tfg) + offset;
		/* draw tree expansion character */
		str+=getColor(tree_settings.xfg);
		/* draw tree status character */
		if(this.open) 
			str+="-";
		else
			str+="+";
		/* restore item colors */
		str+=bg + fg;
		/* draw text */
		var c=0;	
		for(;c<this.text.length && (c+this.depth+1)<this.width;c++) {
			if(this.text[c]=="|") {
				if(this.status == item_status.disabled) 
					str+=this.text[++c];
				else {
					str+=getColor(tree_settings.kfg) + this.text[++c] + bg + fg;
					this.hotkey=this.text[c].toUpperCase();
				}
			}
			else {
				str+=this.text[c];
			}
		}
		this.putmsg(str,curr);
	}
	
	/* post a string to the window */
	this.putmsg=function(str,current) {
		if(this.list) {
			if(current)
				this.list.index=this.list.length;
			this.list.push(str);
		}
		else
			this.parent.putmsg(str,current)
	}
	
	/* handle a menu command */
	this.handle_command=function(cmd) {
		/* if this menu tree is empty */
		if(this.depth == 0 && !this.items.length)
			return false;
		/* initialize return value */
		var retval=false;
		if(retval == false) {
			/* check to see if the current tree item is a subtree */
			if(this.open && this.items[this.index] && typeof this.items[this.index].handle_command == "function") 
				/* if so, pass control to the next subtree */
				retval=this.items[this.index].handle_command(cmd);
		}
		/* otherwise let this menu handle the command */
		if(retval == false) {
			/* process the command */
			retval=this.process_command(cmd);
			/* update current item's index if it is a tree */
			if(this.items[this.index] && typeof this.items[this.index].update_index == "function") 
				this.items[this.index].update_index(cmd);
		}
		if(retval == false) {
			/* attempt a character match on tree items */
			retval=this.match_hotkey(cmd);
		}
		if(this.depth == 0)
			this.draw();
		/* return whatever retval contains */
		return retval;
	}
	
	/* update this tree's index based on the command used to enter it */
	this.update_index=function(cmd) {
		if(!this.open) 
			this.index=-1;
		else {
			switch(cmd.toUpperCase()) {
			case KEY_UP:
				this.index=this.items.length-1;
				break;
			case KEY_DOWN:
				this.index=-1;
				break;
			}
		}
		/* update current item's index if it is a tree */
		if(this.items[this.index] && typeof this.items[this.index].update_index == "function") 
			this.items[this.index].update_index(cmd);
	}
	
	/* process a command key (called internally from handle_command only) */
	this.process_command=function(cmd) {
		/* if we are at the tree heading, 
			for status toggle */
		switch(cmd.toUpperCase()) {
		case '+':
			if(this.open)
				return true;
		case '\r':
			if(this.index < 0) {
				this.toggle();
				return true;
			}
			/* if the command has been sent here for processing
				and the current item could not handle it,
				then the current item's handle_command property
				is the return value */
			if(typeof this.items[this.index].handle_command != "function") 
				return(this.items[this.index].handle_command);
			break;
		case KEY_DOWN:
			if(this.depth > 0 && (!this.open || this.index == this.items.length -1)) 
				return false;
			this.next_index(1);
			return true;
		case KEY_END:
			if(this.depth > 0 && !this.open) 
				return false;
			while(this.index < this.items.length -1)
				this.next_index(1);
			return false;
		case KEY_UP:
			if(this.depth > 0 && (!this.open || this.index == -1)) 
				return false;
			this.next_index(-1);
			return true;
		case KEY_HOME:
			if(this.depth > 0 && !this.open) 
				return false;
			while(this.index > 0)
				this.next_index(-1);
			return false;
		case '-':
		case '\b':
			if(this.depth > 0) {
				if(this.open) {
					this.toggle();
					return true;
				}
				return false;
			}
			break;
		}
		return false;
	}
	
	/* scan tree for possible hotkey matches */
	this.match_hotkey=function(cmd) {
		
		var pattern=new RegExp(cmd,"i");
		var stop=this.index>0?this.index-1:this.items.length-1;
		for(var i=this.index+1;;i++) {
			if(i == -1)
				continue;
			if(i >= this.items.length) 
				i=0;
			if(this.items[i].status==item_status.enabled && this.items[i].text.search(pattern) >= 0) {
				this.current=i;
				return true;
			}
			if(i == stop) 
				break;
		}	
		return false;
	}
	
	/* return the next valid menu index in the direction supplied */
	this.next_index=function(i) {
		var start=this.index;
		while(1) {
			this.index += i;
			if(this.index < -1)
				this.index = this.items.length-1;
			else if(this.index >= this.items.length) 
				this.index = -1;
			if(this.depth == 0 && this.index == -1) 
				continue;
			if(this.index == start)
				break;
			if(this.index == -1 || this.items[this.index].status == item_status.enabled)
				break;
		}
		return true;
	}

	/* calculate tree depth */
	this.depth getter=function() {
		if(!this.parent)
			return(0);
		else
			return(this.parent.depth+1);
	}
	
	/* trace back to main tree to see if this tree is the current item */
	this.current getter=function() {
		if(!this.parent)
			return true;
		else if(this.parent_index == this.parent.index)
			if(this.parent.current)
				return true;
		else
			return false;
	}
	
	/* trace back to main tree and set this menu current */
	this.current setter=function(i) {
		this.index=i;
		if(this.parent)
			this.parent.current=this.parent_index;
	}

	/* return tree width */
	this.width getter=function() {
		if(!width)
			return this.parent.width;
		else
			return width;
	}
	
	/* set tree width */
	this.width setter=function(w) {
		if(width)
			width=w;
		else {
			log(this.text + ": " + this.depth);
			this.parent.width=w;
		}
	}
	
	/* return tree height */
	this.height getter=function() {
		if(!height)
			return this.parent.height;
		else
			return height;
	}
	
	/* set tree height */
	this.height setter=function(h) {
		if(height)
			height=h;
		else
			this.parent.height=h;
	}
	
}

/* tree item object */
function Tree_Item(text,handle_command,status) {
	this.text="";
	this.status=item_status.enabled;
	
	/* 
	this.parent_index;
	this.parent;
	this.hotkey;
	*/
	
	if(text != undefined)
		this.text=text;
	if(status != undefined)
		this.status=status;
		
	/* 	item command action, to be called
		within the context of the parent script:
		item.action.apply(this,arg1,arg2,etc..); */
	this.handle_command=handle_command;
	
	/* draw the current item text into the master list */
	this.draw=function(offset) {
		var str="";
		var curr=false;
		/* if this item is hidden, do not draw */
		if(this.status == item_status.hidden) 
			return false;
		/* set color attributes */
		var bg="\1n" + getColor(tree_settings.bg);
		var fg=getColor(tree_settings.fg);
		/* if this is the current tree and the current tree index */
		if(this.parent.current && this.parent_index == this.parent.index) {
			bg=getColor(tree_settings.lbg);
			fg=getColor(tree_settings.lfg);
			curr=true;
		}
		/* if this item is disabled */
		else if(this.status == item_status.disabled) {
			fg=getColor(tree_settings.dfg);
		}
		/* draw line offset */
		str+=bg + getColor(tree_settings.tfg)+offset;
		/* draw tree branches */
		if(this.parent.depth > 0) {
			if(this.parent_index == this.parent.items.length-1)
				str+="\xC0";
			else 
				str+="\xC3";
		}
		/* restore item colors */
		str+=bg+fg;
		/* draw text */
		var c=0;
		for(;c<this.text.length && (c+offset.length+1)<this.parent.width;c++) {
			if(this.text[c]=="|") {
				if(this.status == item_status.disabled) 
					str+=this.text[++c];
				else {
					str+=getColor(tree_settings.kfg) + this.text[++c] + bg + fg;
					this.hotkey=this.text[c].toUpperCase();
				}
			}
			else {
				str+=this.text[c];
			}
		}
		/* push the item string into the master display list */
		this.parent.putmsg(str,curr);
	}
}








