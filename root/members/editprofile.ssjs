/* $Id: editprofile.ssjs,v 1.6 2006/02/25 21:40:35 runemaster Exp $ */

load("../web/lib/template.ssjs");

var sub = '';

var is_sysop=false;

if(user.number==1 || user.security.level>=90)
	disabled="";
else
	disabled='disabled="disabled"';

template.title=system.name +" - Edit Your Profile";

usr = new Object;

usr.name = user.name;
if(usr.name==undefined)
	usr.name='';
usr.alias = user.alias;
if(usr.alias==undefined)
	usr.alias='';
usr.handle = user.handle;
if(usr.handle==undefined)
	usr.handle='';
usr.netmail = user.netmail;
if(usr.netmail==undefined)
	usr.netmail='';
usr.location = user.location;
if(usr.location==undefined)
	usr.location='';
usr.address = user.address;
if(usr.address==undefined)
	usr.address='';
usr.zipcode = user.zipcode;
if(usr.zipcode==undefined)
	usr.zipcode='';
usr.phone = user.phone;
if(usr.phone==undefined)
	usr.phone='';

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
	template.profile.push({html: '<p>Personal Information</p>' });
	template.profile.push({html: '<form action="/members/updateprofile.ssjs?edituser=' + user.number + '" method="post">' });
	template.profile.push({html: '<table class="userstats2" cellpadding="2" cellspacing="1">' });
	if(system.newuser_questions & UQ_REALNAME)
		template.profile.push({html: '<tr><td class="userstats" align="right">Real Name: </td><td class="userstats" align="left"><input type="text" name="name" size="25" maxlength="25" value="' + usr.name +  '" ' + disabled + ' /></td></tr>' });
	if(system.newuser_questions & UQ_ALIASES)
		template.profile.push({html: '<tr><td class="userstatsright" align="right">Alias: </td><td class="userstats" align="left"><input type="text" name="alias" size="25" maxlength="25" value="' + usr.alias + '" ' + disabled + ' /></td></tr>' });
	if(system.newuser_questions & UQ_HANDLE)
		template.profile.push({html: '<tr><td class="userstatsright" align="right">Chat Handle: </td><td class="userstats" align="left"><input type="text" name="handle" size="8" maxlength="8" value="' + usr.handle + '" /></td></tr>' });
	template.profile.push({html: '<tr><td class="userstatsright" align="right">E-Mail Address: </td><td class="userstats" align="left"><input type="text" name="email" size="50" maxlength="60" value="' + usr.netmail + '" /></td></tr>' });
	if(system.newuser_questions & UQ_ADDRESS)
		template.profile.push({html: '<tr><td class="userstatsright" align="right">Address: </td><td class="userstats" align="left"><input type="text" name="address" size="50" maxlength="50" value="' + usr.address + '" /></td></tr>' });
	if(system.newuser_questions & UQ_LOCATION)
		template.profile.push({html: '<tr><td class="userstatsright" align="right">City, State/Prov: </td><td class="userstats" align="left"><input type="text" name="location" size="50" maxlength="50" value="' + usr.location + '" /></td></tr>' });
	if(system.newuser_questions & UQ_ADDRESS)
		template.profile.push({html: '<tr><td class="userstatsright" align="right">Zip/Postal Code: </td><td class="userstats" align="left"><input type="text" name="zipcode" size="50" maxlength="50" value="' + usr.zipcode + '" /></td></tr>' });
	if(system.newuser_questions & UQ_PHONE)
		template.profile.push({html: '<tr><td class="userstatsright" align="right">Phone: </td><td class="userstats" align="left"><input type="text" name="phone" size="50" maxlength="50" value="' + usr.phone + '" /></td></tr>' });
	template.profile.push({html: '</table>' });
	template.profile.push({html: '<h3>Web Profile</h3>'});
	template.profile.push({html: '<p>Changes here are only visible in the Web Interface.<br /><br /></p>' });
	template.profile.push({html: '<table class="userstats2" cellpadding="2" cellspacing="1">' });
	template.profile.push({html: '<tr><td class="userstatsright" align="right">ICQ: </td><td class="userstats" align="left"><input type="text" name="icq" size="10" maxlength="10" value="' + usr.icq + '" /></td></tr>' });
	template.profile.push({html: '<tr><td class="userstatsright" align="right">MSN: </td><td class="userstats" align="left"><input type="text" name="msn" size="50" maxlength="50" value="' + usr.msn + '" /></td></tr>' });
	template.profile.push({html: '<tr><td class="userstatsright" align="right">Yahoo: </td><td class="userstats" align="left"><input type="text" name="yahoo" size="50" maxlength="50" value="' + usr.yahoo + '" /></td></tr>' });
	template.profile.push({html: '<tr><td class="userstatsright" align="right">AIM: </td><td class="userstats" align="left"><input type="text" name="aim" size="50" maxlength="50" value="' + usr.aim + '" /></td></tr>' });
	template.profile.push({html: '<tr><td class="userstatsright" align="right">Homepage: </td><td class="userstats" align="left"><input type="text" name="homepage" size="50" maxlength="50" value="' + usr.homepage + '" /></td></tr>' });
	template.profile.push({html: '<tr><td class="userstatsright" align="right">Hobbies: </td><td class="userstats" align="left"><input type="text" size="50" name="hobbies" value="' + usr.hobbies + '" /></td></tr>' });
	template.profile.push({html: '<tr><td class="userstatsright" align="right">Picture (Link): </td><td class="userstats" align="left"><input type="text" size="50" name="picture" value="' + usr.picture + '" /></td></tr>' });
	template.profile.push({html: '<tr><td class="userstatsright" align="right">Avatar (Link): </td><td class="userstats" align="left"><input type="text" size="50" name="avatar" value="' + usr.avatar + '" /></td></tr>' });
	template.profile.push({html: '<tr><td class="userstats">&nbsp;</td><td class="userstats"><br /><input type="submit" value="Update Profile" /></td></tr></table></form><br />' });	

if(do_header)
	write_template("header.inc");
if(do_topnav)
	load(topnav_html);
if(do_leftnav)
load(leftnav_html);
if(do_rightnav)
	write_template("rightnav.inc");
write_template("profile.inc");
if(do_footer)
	write_template("footer.inc");
