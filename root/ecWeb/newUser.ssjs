/* newUser.ssjs, from ecWeb v2 for Synchronet BBS 3.15+
   by Derek Mullin (echicken -at- bbs.electronicchicken.com) */

/* New user registration via the web interface.  Produces a form based on the
   newuser question toggles, does basic validation of the form prior to
   submission, validates form data after submission, creates a new user
   record. */

load('webInit.ssjs');
load(webIni.webRoot + '/themes/' + webIni.theme + "/layout.ssjs");
load(webIni.webRoot + '/lib/captchaLib.ssjs');

openPage("New User Registration");
print("<span class=titleFont>New User Registration</span><br /><br />");

if(http_request.query.hasOwnProperty('action') && http_request.query.action == 'newUser' && user.alias.toUpperCase() == webIni.guestUser.toUpperCase()) {

	/* The various 'x not provided' errors should only crop up if somebody
	   messed with our newuser form or posted from another form. */
	var failString = '';
	var newUserObject = new Object();

	if(system.newuser_questions&UQ_ALIASES) {
		if(!http_request.query.hasOwnProperty('alias') || http_request.query.alias.toString().length < 1) {
			failString = '- Alias not provided<br />';
		} else if(system.trashcan('name', http_request.query.alias)) {
			failString += '- Invalid alias supplied<br />';
		} else if(system.matchuser(http_request.query.alias.toString())) {
			failString += '- Alias already in use<br />';
		} else {
			newUserObject.alias = http_request.query.alias.toString();
		}
	}

	if(!http_request.query.hasOwnProperty('password1') || !http_request.query.hasOwnProperty('password2') || http_request.query.password1.toString().toUpperCase() != http_request.query.password2.toString().toUpperCase() || http_request.query.password1.toString().length < 4) {
		failString += '- Invalid or mismatched passwords supplied<br />';
	} else {
		newUserObject.password = http_request.query.password1.toString().toUpperCase();
	}

	if(system.newuser_questions&UQ_REALNAME) {
		if(!http_request.query.hasOwnProperty('realName') || http_request.query.realName.toString().length < 1) {
			failString += '- Real name not provided<br />';
		} else if(system.trashcan('name', http_request.query.realName)) {
			failString += '- Invalid real name supplied<br />';
		} else if(system.newuser_questions&UQ_DUPREAL && system.matchuser(http_request.query.realName.toString())) {
			failString += '- Real name already in use<br />';
		} else {
			newUserObject.name = http_request.query.realName.toString();
		}
	}

	if(system.newuser_questions&UQ_HANDLE) {
		if(!http_request.query.hasOwnProperty('handle') || http_request.query.handle.toString().length < 1) {
			failString += '- Chat handle not provided<br />';
		} else if(system.trashcan('name', http_request.query.handle)) {
			failString += '- Invalid chat handle supplied<br />';
		} else if(system.newuser_questions&UQ_DUPHAND && system.matchuser(http_request.query.handle.toString())) {
			failString += '- Chat handle already in use<br />';
		} else {
			newUserObject.handle = http_request.query.handle.toString();
		}
	}

	if(system.newuser_questions&UQ_LOCATION) {
		if(!http_request.query.hasOwnProperty('location') || http_request.query.location.toString().length < 1) { 
			failString += '- Location not provided<br />';
		} else {
			newUserObject.location = http_request.query.location.toString();
		}
	}

	if(system.newuser_questions&UQ_ADDRESS) {
		if(!http_request.query.hasOwnProperty('streetAddress') || http_request.query.streetAddress.toString().length < 6) {
			failString += '- Address not provided<br />';
		} else {
			newUserObject.address = http_request.query.streetAddress.toString();
		}
	}

	if(system.newuser_questions&UQ_PHONE) {
		if(!http_request.query.hasOwnProperty('phone') || http_request.query.phone.length < 1) {
			failString += '- Phone number not provided<br />';
		} else if(system.trashcan('phone', http_request.query.phone)) {
			failString += '- Invalid phone number suplied<br />';
		} else {
			newUserObject.phone = http_request.query.phone.toString();
		}
	}

	if(system.newuser_questions&UQ_SEX) {
		if(!http_request.query.hasOwnProperty('sex') || (http_request.query.sex.toString() != 'm' && http_request.query.sex.toString() != 'f')) {
			failString += '- Sex not provided (lol)<br />';
		} else {
			newUserObject.gender = http_request.query.sex.toString().toUpperCase();
		}
	}

	if(system.newuser_questions&UQ_BIRTH) {
		if(!http_request.query.hasOwnProperty('birthDate') || http_request.query.birthDate.toString().match(/\d\d-\d\d-\d\d/) == null) {
			failString += '- Birth date not provided<br />';
		} else {
			newUserObject.birthdate = http_request.query.birthDate.toString();
		}
	}

	if(system.newuser_questions&UQ_COMPANY) {
		if(!http_request.query.hasOwnProperty('company') || http_request.query.company.length < 1) {
			failString += '- Company name not provided<br />';
		} else {
			// I don't know where this would go, either. Probably one of the
			// 'comment' properties of the user object. Probably doesn't matter.
		}
	}

	if(system.newuser_questions&UQ_NONETMAIL) {
	} else {
		if(!http_request.query.hasOwnProperty('netmail') || !http_request.query.netmail.toString().match(/\w+\@\w+/)) {
			failString += '- Invalid email/netmail address provided<br />';
		} else {
			newUserObject.netmail = http_request.query.netmail.toString();
		}
	}

	if(!http_request.query.hasOwnProperty('letters1') || !http_request.query.hasOwnProperty('letters2')) {
		failString += '- CAPTCHA missing<br />';
	} else if(md5_calc(http_request.query.letters1.toString().toUpperCase(), hex=true) != http_request.query.letters2.toString()) {
		failString += '- CAPTCHA mismatch<br />';
	}

	if(system.newuser_password != "" && (!http_request.query.hasOwnProperty('nup') || http_request.query.nup.toString().toUpperCase() != system.newuser_password.toUpperCase())) {
		failString += '- Incorrect or no newuser password supplied<br />';
	}

	if(failString.length > 0) {
		print("Your registration failed for the following reasons:<br /><br />" + failString);
	} else {
		var makeNewUser = system.new_user(newUserObject.alias);
		for(property in newUserObject) {
			if(property == 'alias') continue;
			if(property == 'password') {
				makeNewUser.security.password = newUserObject.password;
				continue;
			}
			makeNewUser[property] = newUserObject[property];
		}
		print("User account created.");
	}

} else if(user.alias.toUpperCase() == webIni.guestUser.toUpperCase()) {

	print("<form name=newUser id=newUser action=./newUser.ssjs method=post onsubmit='return validateNewUserForm()'>");
	print("<input type=hidden name=action value=newUser />");
	if(system.newuser_questions&UQ_ALIASES) print("Alias:<br /><input type=text size=30 name=alias id=alias /> <span id=aliasError></span><br /><br />");
	print("Password:<br /><input type=password size=30 name=password1 id=password1 /> <span id=password1Error></span><br /><br />");
	print("Password again:<br /><input type=password size=30 name=password2 id=password2 /> <span id=password2Error></span><br /><br />");
	if(system.newuser_questions&UQ_REALNAME) print("Real Name:<br /><input type=text size=30 name=realName id=realName /> <span id=realNameError></span><br /><br />");
	if(system.newuser_questions&UQ_HANDLE) print("Chat Handle:<br /><input type=text size=30 name=handle id=handle /> <span id=handleError></span><br /><br />");
	if(system.newuser_questions&UQ_LOCATION) print("Location:<br /><input type=text size=30 name=location id=location /> <span id=locationError></span><br /><br />");
	if(system.newuser_questions&UQ_ADDRESS) print("Street Address:<br /><input type=text size=30 name=streetAddress id=streetAddress /> <span id=streetAddressError></span><br /><br />");
	if(system.newuser_questions&UQ_PHONE) print("Phone Number:<br /><input type=text size=30 name=phone id=phone /> <span id=phoneError></span><br /><br />");
	if(system.newuser_questions&UQ_SEX) print("Sex: <input type=radio name=sex id=sex value=m />M <input type=radio name=sex id=sex value=f />F <span id=sexError></span><br /><br />"); // lol
	if(system.newuser_questions&UQ_BIRTH) print("Birthdate DD-MM-YY:<br /><input type=text size=8 name=birthDate id=birthDate /> <span id=birthDateError></span><br /><br />");
	if(system.newuser_questions&UQ_COMP) print("Computer:<br /><input type=text size=30 name=computer id=computer /> <span id=computerError></span><br /><br />");
	if(system.newuser_questions&UQ_COMPANY) print("Company:<br /><input type=text size=30 name=company id=company /> <span id=companyError></span><br /><br />");
	if(system.newuser_questions&UQ_NONETMAIL) {
	} else {
		print("Email/Netmail:<br /><input type=text size=30 name=netmail id=netmail /> <span id=netmailError></span><br /><br />");
	}
	insertCaptcha(); // Draws a CAPTCHA, inserts the hidden input 'letters2' (md5 sum of the CAPTCHA string) and text input 'letters1'
	if(system.newuser_password != "") print("Please supply the new user password below.<br /><input class='border font' type=password size=25 name=nup><br /><br />");
	print("<input type=submit value=Submit />");
	print("</form>");

} else {

	print("You're already logged in with a valid user account.  At least try logging out first.");

}

closePage();
