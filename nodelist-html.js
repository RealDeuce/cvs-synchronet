// nodelist-html.js

// Synchronet Service for sending a periodic HTML node list

var refresh_rate=10	/* seconds */
var start=new Date();

load("nodedefs.js");

var include_age_gender=true;
var include_location=false;

// Parse arguments
for(i=0;i<argc;i++)
	switch(argv[i].toLowerCase()) {
		case "-r":
			refresh_rate=Number(argv[++i]);
			break;
		case "-n":
			include_age_gender=false;
			break;
		case "-l":
			include_location=true;
			break;
	}

// Write a string to the client socket
function write(str)
{
	client.socket.send(str);
}

function writeln(str)
{
	write(str + "\r\n");
}

// Get HTTP Request
while(client.socket.data_waiting) {
	request = client.socket.recvline(128 /*maxlen*/, 3 /*timeout*/);

	if(request==null) 
		break;

//	log(format("client request: '%s'",request));
}

// HTML Header
writeln("<html>");
writeln("<head>");
writeln(format("<title>%s BBS - Node List</title>",system.name));
writeln(format("<meta http-equiv=refresh content=%d>",refresh_rate));
writeln("</head>");

writeln("<body bgcolor=teal text=white link=yellow vlink=lime alink=white>");
writeln("<font face=Arial,Helvetica,sans-serif>");

// Login Button - Modified by RuneMaster of RuneKeep BBS
writeln("<table border=0 width=100%>");
writeln("<tr>");
writeln("<td align=left>");
writeln("<font color='lime'>");
writeln(format("<h1><i>%s BBS - Node List</i></h1>",system.name));
writeln("</font>");
writeln("</td>");
writeln("<td align=right>");
writeln("<form>");
writeln("<input type=button value='Login' onClick='location=\"telnet://"
        + system.inetaddr + "\";'>");
writeln("</form>");
writeln("</td>");
writeln("</tr>");
writeln("</table>");

// Table
writeln("<table border=0 width=100%>");

// Header
writeln("<thead>");
writeln("<tr bgcolor=white>");
font_color = "<font color=black>";

write(format("<th align=center width=7%>%sNode",font_color));
write(format("<th align=center width=20%>%sUser",font_color));
write(format("<th align=left>%sAction/Status",font_color));
if(include_location) 
	write(format("<th align=left>%sLocation",font_color));
if(include_age_gender) {
	write(format("<th align=center width=7%>%sAge",font_color));
	write(format("<th align=center width=10%>%sGender\r\n",font_color));
}
write(format("<th align=center width=10%>%sTime\r\n",font_color));
writeln("</thead>");

writeln("<tbody>");
var user = new User(1);
for(n=0;n<system.node_list.length;n++) {
	write("<tr>");
	write(format("<td align=right><font size=-1>%d",n+1));
	if(system.node_list[n].status==NODE_INUSE) {
		user.number=system.node_list[n].useron;
		if(system.node_list[n].action==NODE_XTRN && system.node_list[n].aux)
			action=format("running %s",user.curxtrn);
		else
			action=format(NodeAction[system.node_list[n].action]
				,system.node_list[n].aux);
		write(format(
			"<td align=center><a href=mailto:%s>%s</a>"
			,user.email
			,user.alias
			));
		write(format(
			"<td><font color=yellow>%s"
			,action
			));
		if(include_location)
			write(format(
				"<td align=left>%s"
				,user.location
				));
		if(include_age_gender) 
			write(format(
				"<td align=center>%d<td align=center>%s"
				,user.age
				,user.gender
				));
		t=time()-user.logontime;
        if(t<0) t=0;
		write(format(
			"<td align=center>%u:%02u:%02u"
			,Math.floor(t/(60*60))
			,Math.floor(t/60)%60
			,t%60
			));
	} else {
		action=format(NodeStatus[system.node_list[n].status],system.node_list[n].aux);
		write(format("<td><td>%s",action));
	}
	write("\r\n");
}
writeln("</tbody>");
writeln("</table>");

writeln("<p><font color=silver><font size=-2>");
writeln(format("Auto-refresh in %d seconds",refresh_rate));
write(format("<br>Dynamically generated in %lu milliseconds "
	  ,new Date().valueOf()-start.valueOf()));
write("by <a href=http://www.synchro.net>" + system.version_notice + "</a>");
writeln("<br>" + system.timestr());

writeln("</body>");
writeln("</html>");
sleep(1000);
/* End of nodelist-html.js */
