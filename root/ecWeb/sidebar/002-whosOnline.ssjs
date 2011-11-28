/* whosOnline.ssjs, from ecWeb v2 for Synchronet BBS 3.15+
   by Derek Mullin (echicken -at- bbs.electronicchicken.com) */
   
/* An asynchronous "Who's Online" listing that updates at a sysop-defined
   interval.  Written for the ecWeb sidebar, but could be used elsewhere. */

var update = 30000; // Milliseconds between updates

load("nodedefs.js");
if(http_request.query.hasOwnProperty("action") && http_request.query.action.toString() == "show") {

	print("<b>Who's online</b><br><br>");
	print("<table border=0 cellpadding=0 cellspacing=0 class='standardColor standardFont'>");
	for(n in system.node_list) {
		print("<tr><td>Node " + (parseInt(n) + 1) + ":&nbsp;</td>");
		if(system.node_list[n].status == 3) {
			print("<td>" + system.username(system.node_list[n].useron) + "</td></tr><tr><td>&nbsp;</td><td style=font-style:italic;>" + NodeAction[system.node_list[n].action] + "</td></tr>");
		} else {
			print("<td style=font-style:italic;>" + NodeStatus[system.node_list[n].status] + "</td></tr>");
		}
	}
	print("</table>");

} else {

	print("<div id='whosonline'></div>");
	print("<script type='text/javascript'>");
	print("function xhrwo() {");
	print("\tvar XMLReq = new XMLHttpRequest();");
	print("\tXMLReq.open('GET', './sidebar/002-whosOnline.ssjs?action=show', true);");
	print("\tXMLReq.send(null);");
	print("\tXMLReq.onreadystatechange = function() { if(XMLReq.readyState == 4) { document.getElementById('whosonline').innerHTML = XMLReq.responseText; } }");
	print("}");
	print("setInterval('xhrwo()', " + update + ");");
	print("xhrwo();");
	print("</script>");

}
