/*****************************************************************************/
/* SSJS templating library                                                   */
/* %%name%% is replaced with the HTML encoded value of template.name         */
/* ^^name^^ is replaced with the URI encoded value of template.name          */
/* @@name@@ is replaced with the value if template.name                      */
/*                                                                           */
/* @@name:sname@@ is replaced with the value of template.name.sname          */
/* (^^ and %% are also supported)                                            */
/*                                                                           */
/* @@JS:js_expression@@ is replaced with the return value of js_expression   */
/* (^^ and %% are also supported)                                            */
/*                                                                           */
/* <<REPEAT name>>                                                           */
/* ... @@name:sname@@ ...                                                    */
/* <<END REPEAT name>>                                                       */
/*                                                                           */
/* Iterates over the array/object template.name and replaces name:sname with */
/* the value of template.name.sname.                                         */
/* (^^ and %% are also supported)                                            */
/*                                                                           */
/*****************************************************************************/

/* $Id$ */

template=new Object;
load("sbbsdefs.js");    // UFLAG_G
load("../web/lib/html_themes.ssjs");
if(http_request.virtual_path=="/nodelist.ssjs")
    load("../web/lib/main_nodelist.ssjs"); // Who's Online Listing
http_reply.fast=true;
    
load("../web/lib/leftnav_nodelist.ssjs"); // Left Side Navigation Node Listing

template.Theme_CSS_File=Themes[CurrTheme].css;

function write_template(filename)  {
	var fname='../web/templates/'+Themes[CurrTheme].dir+'/'+filename;
	if(!file_exists(fname)) {
		fname='../web/templates/'+Themes[DefaultTheme].dir+'/'+filename;
		if(!file_exists(fname))
			fname='../web/templates/'+Themes["Default"].dir+'/'+filename;
	}
	var inc=new File(fname);
	if(!inc.open("r",true,1024)) {
		horrible_error("!Error " + inc.error + " opening template file: "+ fname);
	}
	var file='';
	while(! inc.eof)  {
		file=file += inc.read(1024);
	}
	inc.close();
	file=file.replace(/([\x00-\xff]*?)<<REPEAT (.*?)>>([\x00-\xff]*?)<<END REPEAT \2>>/g,
		function(matched, start, objname, bit, offset, s) {
			var ret='';
			start=parse_regular_bit(start,"",template);
			start=start.replace(/\<\!-- Magical Synchronet ([\^%@])-code --\>/g,'$1');
			write(start);
			for(obj in template[objname]) {
				var thisbit=parse_regular_bit(bit,objname,template[objname][obj]);
				thisbit=parse_regular_bit(thisbit,"",template);
				thisbit=thisbit.replace(/\<\!-- Magical Synchronet ([\^%@])-code --\>/g,'$1');
				write(thisbit);
			}
			return("");
		});
	file=parse_regular_bit(file, "", template);
	file=file.replace(/\<\!-- Magical Synchronet ([\^%@])-code --\>/g,'$1');
	write(file);
}
function parse_regular_bit(bit, objname, obj) {
	if(objname=="JS") {
		return(bit);
	}
	if(objname=='') {
		bit=bit.replace(/([%^@])\1([^^%@\r\n]*?)\:([^:%^@\r\n]*?)\1\1/g,
			function (matched, start, objname, prop, offset, s) {
				var res=matched;
				if(template[objname]!=undefined)
					res=escape_match(start, template[objname][prop]);
				return(res);
			});
		bit=bit.replace(/([%^@])\1([^:^%@\r\n]*?)\1\1/g,
			function (matched, start, exp, offset, s) {
				var res=escape_match(start, template[exp]);
				return(res);
			});
		bit=bit.replace(/([%^@])\1JS:([\x00-\xff]*?)\1\1/g,
			function (matched, start, exp, offset, s) {
				var res=escape_match(start, eval(exp));
				return(res);
			});
		bit=bit.replace(/([%^@])\1(.*?)\1\1/g,'');
	}
	else {
		bit=bit.replace(new RegExp('([%^@])\\1'+regex_escape(objname)+':([^^%@\r\n]*?)\\1\\1',"g"),
			function (matched, start, exp, offset, s) {
				var res=matched;
				if(obj[exp]!=undefined)
					res=escape_match(start, obj[exp]);
				return(res);
			});
	}
	return bit;
}

function escape_match(start, exp, end)  {
    if(exp==undefined)
        exp='';
    exp=exp.toString();
    if(start=="%")
        exp=html_encode(exp,false,false,false,false);
    if(start=="^")
        exp=encodeURIComponent(exp);
    exp=exp.replace(/\@/g,'<!-- Magical Synchronet @-code -->');
    exp=exp.replace(/\^/g,'<!-- Magical Synchronet ^-code -->');
    exp=exp.replace(/\%/g,'<!-- Magical Synchronet %-code -->');
    return(exp);
}

function error(message)  {
    template.title="Error";
    template.err=message;
    write_template("header.inc");
    write_template("error.inc");
    write_template("footer.inc");
    exit();
}

function horrible_error(message) {
    write("<html><head><title>ERROR</title></head><body><p>"+message+"</p></body></html>");
    exit();
}

function regex_escape(str)
{
    str=str.replace(/([\\\^\$\*\+\?\.\(\)\{\}\[\]])/g,"\\$1");
    return(str);
}

if(user.number==0)
    template.user_greeting="Welcome, Guest.";
else
    if(!(user.security.restrictions&UFLAG_G))
        template.user_greeting="Welcome, "+user.alias+ ".<br /> You last visited on " +strftime("%A, %B %d, %Y",user.stats.laston_date);
    else
        template.user_greeting="Welcome, "+user.alias+ ".";

/* FTP link */

 if(user.number || system.matchuser("Guest")) {
    template.ftp_url="ftp://";
    if(user.number && !(user.security.restrictions&UFLAG_G))
        template.ftp_url=template.ftp_url + user.alias + ":" + user.security.password + "@";

    var host = http_request.host;
    if(!host || !host.length)
        host = system.host_name;
   var port = host.indexOf(':');
    if(port>=0)
        host=host.slice(0,port);
    else
        host=host;
    template.ftp_url=template.ftp_url + host + "/00index.html?$" + new Date().valueOf().toString(36);
}
