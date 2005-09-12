// bullseye.js

// Bulletins written in Baja by Rob Swindell
// Translated to JS by Stehen Hurd

// $Id: bullseye.js,v 1.1 2005/09/12 18:53:17 deuce Exp $

// @format.tab-size 4, @format.use-tabs true

load("sbbsdefs.js");

// Load the configuration file

var i=0;
var b=0;
var filepos=0;
var total=0;
var mode=0;
var fname="";
var str="";

writeln("");
writeln("Synchronet BullsEye! Version 2.00 by Rob Swindell");
console.line_counter=0;
file=new File(system.text_dir+"bullseye.cfg");
if(!file.open("r", true)) {
	writeln("");
	writeln("!ERROR "+file.error+" opening "+system.text_dir+"bullseye.cfg");
	exit(1);
}
mode=file.readln();
filepos=file.position;

total=0;
// write(total+": "+mode);
while((str=file.readln())!=null) {
	// write(total+": "+str);
	total++;
}

// Display menu, list bulletins, display prompt, etc.

menu:
while(1) {
	console.printfile(system.text_dir+"bullseye.asc");
	write("\001n\r\n\001b\001hEnter number of bulletin to view or press (\001wENTER\001b) to continue: \001w");
	b=console.getnum(total);
	if(b<1)
		exit(0);
	file.position=filepos;
	i=0;
	while((str=file.readln())!=null) {
		i++;
		if(i==b) {
			write("\001n\001l");
			str=truncsp(str);
			fname=str;
			bbs.replace_text(563,"\001n\001h\001b{\001wContinue? Yes/No\001b} ");
			if(str.search(/\.htm/)!=-1)
				system.exec("?typehtml -color "+str);
			else
				console.printfile(str);
			log("Node "+bbs.node_num+" "+user.alias+" viewed bulletin #"+i+": "+fname);
			bbs.revert_text(563);
			console.aborted=false;
			continue menu;
		}
	}
	writeln();
	write("Invalid bulletin number: "+b);
}
