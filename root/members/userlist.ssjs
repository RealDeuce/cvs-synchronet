// userlist.js

// A sample user listing script for Synchronet v3.1+

// $Id$

load("sbbsdefs.js");
load("html_inc/template.ssjs");

if(system.lastuser==undefined)	/* v3.10 */
	lastuser=system.stats.total_users;
else							/* v3.11 */
	lastuser=system.lastuser;
var u = new User(1);
template = new Object;
template.users = new Array;
template.title = "User List";

for(i=1;i<=lastuser;i++) {
	usr=new Object;
	u.number=i;
	if(u.settings&USER_DELETED)
		continue;
	usr.alias=u.alias.toString();
	usr.location=u.location.toString();
	usr.connection=u.connection.toString();
	usr.logon=strftime("%m/%d/%y",u.logontime);
	template.users.push(usr);
}
template.users.sort(alphasort);

write_template("header.inc");
write_template("userlist.inc");
write_template("footer.inc");

function alphasort (a,b)
{
	var au=a.alias.toUpperCase();
	var bu=b.alias.toUpperCase();
	if(au<bu)
		return -1;
	if(bu>au)
		return 1;
	return 0;
}
