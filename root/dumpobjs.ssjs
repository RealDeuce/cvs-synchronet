// dumpobjs.ssjs

// $Id$

// Used for debugging (and possibly documenting) the Synchronet web server's JS objects

write("<html><body>");
function dump(obj, name)
{
	var i;

	for(i in obj) {
		if(obj.length!=undefined)
			write(name +'['+ i +'] = ');
		else
			write(name +'.'+ i +' = ');
		write(obj[i] + "<br>");
		if(typeof(obj[i])=="object")
			dump(obj[i], name +'.'+ i);
	}
}

dump(http_request,"http_request");
dump(http_reply,"http_reply");

write("</body></html>");
