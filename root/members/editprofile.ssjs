/* $Id: editprofile.ssjs,v 1.2 2006/01/09 03:27:22 runemaster Exp $ */

load("../web/lib/template.ssjs");

var sub = '';

template.title=system.name +" - Edit Your Profile";

usr = new Object;

usr.name = user.name;
usr.alias = user.alias;
usr.handle = user.handle;
usr.netmail = user.netmail;
usr.location = user.location;
usr.address = usr.address;
usr.zipcode = user.zipcode;

if(file_exists(prefs_dir +format("%04d.html_prefs",user.number))) {
	prefsfile=new File(prefs_dir + format("%04d.html_prefs",user.number));
	if(prefsfile.open("r",false)) {
				usr.icq = prefsfile.iniGetValue('Profile', 'ICQ');
				if(usr.icq==undefined)
					usr.icq='';
				usr.msn = prefsfile.iniGetValue('Profile', 'MSN');
				if(usr.msn==undefined)
					usr.msn='';
				usr.yahoo = prefsfile.iniGetValue('Profile', 'Yahoo');
				if(usr.yahoo==undefined)
					usr.yahoo='';
				usr.aim = prefsfile.iniGetValue('Profile', 'AIM');
				if(usr.aim==undefined)
					usr.aim='';
				usr.homepage = prefsfile.iniGetValue('Profile', 'Homepage');
				if(usr.homepage==undefined)
					usr.homepage='';
				usr.hobbies = prefsfile.iniGetValue('Profile', 'Hobbies');
				if(usr.hobbies==undefined)
					usr.hobbies='';
				usr.picture = prefsfile.iniGetValue('Profile', 'Picture');
				if(usr.picture==undefined)
					usr.picture='';
				usr.avatar = prefsfile.iniGetValue('Profile', 'Avatar');
				if(usr.avatar==undefined)
					usr.avatar='';
		prefsfile.close();
	}
}

template.profile = new Array;

	template.profile.push({html: '<h1>Edit Profile</h1>' });
	template.profile.push({html: '<p>Changes here mainly affect the Telnet side of the BBS</p>' });
	template.profile.push({html: '<form action="/members/updateprofile.ssjs" method="post">' });
	template.profile.push({html: '<table class="newuser" cellpadding="0" cellspacing="2">' });
	if(system.newuser_questions & UQ_REALNAME)
		template.profile.push({html: '<tr><td class="newuser" align="right">Real Name:</td><td class="newuser" align="left"><input type="text" name="name" size="25" maxlength="25" value="' + usr.name + '" /></td></tr>' });
	if(system.newuser_questions & UQ_ALIASES)
		template.profile.push({html: '<tr><td class="newuser" align="right">Alias:</td><td class="newuser" align="left"><input type="text" name="alias" size="25" maxlength="25" value="' + usr.alias + '" /></td></tr>' });
	if(system.newuser_questions & UQ_HANDLE)
		template.profile.push({html: '<tr><td class="newuser" align="right">Chat Handle:</td><td class="newuser" align="left"><input type="text" name="handle" size="8" maxlength="8" value="' + usr.handle + '" /></td></tr>' });
	template.profile.push({html: '<tr><td class="newuser" align="right">E-Mail Address:</td><td class="newuser" align="left"><input type="text" name="email" size="50" maxlength="60" value="' + usr.netmail + '" /></td></tr>' });
	if(system.newuser_questions & UQ_ADDRESS)
		template.profile.push({html: '<tr><td class="newuser" align="right">Address:</td><td class="newuser" align="left"><input type="text" name="address" size="50" maxlength="50" value="' + usr.address + '" /></td></tr>' });
	if(system.newuser_questions & UQ_LOCATION)
		template.profile.push({html: '<tr><td class="newuser" align="right">City, State/Prov:</td><td class="newuser" align="left"><input type="text" name="location" size="50" maxlength="50" value="' + usr.location + '" /></td></tr>' });
	if(system.newuser_questions & UQ_ADDRESS)
		template.profile.push({html: '<tr><td class="newuser" align="right">Zip/Postal Code:</td><td class="newuser" align="left"><input type="text" name="zipcode" size="50" maxlength="50" value="' + usr.zipcode + '" /></td></tr>' });
	template.profile.push({html: '</table>' });
	template.profile.push({html: '<h3>Web Profile</h3>'});
	template.profile.push({html: '<p>Changes here are only visible in the Web Interface.</p>' });
	template.profile.push({html: '<table class="newuser" cellpadding="0" cellspacing="2">' });
	template.profile.push({html: '<tr><td class="newuser" align="right">ICQ:</td><td class="newuser" align="left"><input type="text" name="icq" size="10" maxlength="10" value="' + usr.icq + '" /></td></tr>' });
	template.profile.push({html: '<tr><td class="newuser" align="right">MSN:</td><td class="newuser" align="left"><input type="text" name="msn" size="50" maxlength="50" value="' + usr.msn + '" /></td></tr>' });
	template.profile.push({html: '<tr><td class="newuser" align="right">Yahoo:</td><td class="newuser" align="left"><input type="text" name="yahoo" size="50" maxlength="50" value="' + usr.yahoo + '" /></td></tr>' });
	template.profile.push({html: '<tr><td class="newuser" align="right">AIM:</td><td class="newuser" align="left"><input type="text" name="aim" size="50" maxlength="50" value="' + usr.aim + '" /></td></tr>' });
	template.profile.push({html: '<tr><td class="newuser" align="right">Homepage:</td><td class="newuser" align="left"><input type="text" name="homepage" size="50" maxlength="50" value="' + usr.homepage + '" /></td></tr>' });
	template.profile.push({html: '<tr><td class="newuser" align="right">Hobbies:</td><td class="newuser" align="left"><input type="textarea" size="50" name="hobbies" value="' + usr.hobbies + '" /></td></tr>' });
	template.profile.push({html: '<tr><td class="newuser" align="right">Picture (Link):</td><td class="newuser" align="left"><input type="textarea" size="50" name="picture" value="' + usr.picture + '" /></td></tr>' });
	template.profile.push({html: '<tr><td class="newuser" align="right">Avatar (Link):</td><td class="newuser" align="left"><input type="textarea" size="50" name="avatar" value="' + usr.avatar + '" /></td></tr>' });
	template.profile.push({html: '<tr><td class="newuser">&nbsp;</td><td class="newuser"><br /><input type="submit" value="Update Profile" /></td></tr></table></form><br />' });	

write_template("header.inc");
load("../web/lib/topnav_html.ssjs");
load("../web/lib/leftnav_html.ssjs");

write_template("profile.inc");

write_template("footer.inc");
