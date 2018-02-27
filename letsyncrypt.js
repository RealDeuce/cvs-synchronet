require("acmev2.js", "ACMEv2");

/*
 * File names used...
 */
var ks_fname = backslash(system.ctrl_dir)+"letsyncrypt.key";
var setting_fname = backslash(system.ctrl_dir)+"letsyncrypt.ini";
var sks_fname = backslash(system.ctrl_dir)+"ssl.cert";
var sbbsini_fname = backslash(system.ctrl_dir)+"sbbs.ini";
var maincnf_fname = backslash(system.ctrl_dir)+"main.cnf";
var recycle_sem = backslash(system.ctrl_dir)+"recycle.web";

/*
 * Variables declarations
 */
var acme;
var auth;
var authz;
var cert;
var challenge;
var completed=0;
var csr;
var dir_path = "/directory";
var dnsnames=[];
var domain_list;
var fulfilled;
var i;
var identifiers = [];
var ks;
var key_id;
var maincnf = new File(maincnf_fname);
var new_host = "acme-staging-v02.api.letsencrypt.org";
var new_domain_hash = '';
var old_domain_hash;
var old_host;
var oldcert;
var order;
var rekey = false;
var renew = false;
var revoke = false;
var rsa;
var sks;
var sbbsini = new File(sbbsini_fname);
var settings = new File(setting_fname);
var syspass;
var tmp;
var token;
var tokens=[];
var webroot;
var webroots = {};

function days_remaining_at_least(days)
{
	var sks;
	var now;
	var cutoff;
	var cert;

	if (!file_exists(sks_fname))
		return true;
	sks = new CryptKeyset(sks_fname, CryptKeyset.KEYOPT.READONLY);
	try {
		cert = sks.get_public_key("ssl_cert");
	}
	catch(e1) {
		sks.close();
		return true;
	}
	sks.close();
	now = new Date();
	cutoff = cert.validto;
	cutoff.setDate(cutoff.getDate() - days);
	return now < cutoff;
}

function create_dnsnames(names) {
	var ext = '';
	var tmplen;
	var count;
	var name;

	for (name in names) {
		ext = names[name] + ext;
		ext = ACMEv2.prototype.asn1_len(names[name].length) + ext;
		ext = ascii(0x82) + ext;
	}
	ext = ACMEv2.prototype.asn1_len(ext.length) + ext;
	ext = ascii(0x30) + ext;
	return ext;
}

/*
 * Get the Web Root
 */
if (!sbbsini.open("r"))
	throw("Unable to open "+sbbsini.name);
webroot = backslash(sbbsini.iniGetValue("Web", "RootDirectory", "../web/root"));
sbbsini.close();

/*
 * Now read the settings and state.
 */
webroots[system.inet_addr] = webroot;
if (settings.open("r")) {
	domain_list = settings.iniGetObject("Domains");
	for (i in domain_list) {
		if (file_isdir(domain_list[i])) {
			webroots[i] = backslash(domain_list[i]);
		}
		else {
			log(LOG_ERR, "Web root for "+i+" is not a directory ("+domain_list[i]+")");
		}
	}
	old_domain_hash = settings.iniGetValue("State", "DomainHash", "<None>");
	old_host = settings.iniGetValue("State", "Host", "acme-staging-v02.api.letsencrypt.org");
	new_host = settings.iniGetValue(null, "Host", new_host);
	dir_path = settings.iniGetValue(null, "Directory", dir_path);

	settings.close();
}

for (i in Object.keys(webroots).sort())
	new_domain_hash += i+"/";
new_domain_hash = md5_calc(new_domain_hash);

/*
 * Parse arguments
 */
if (argv !== undefined) {
	if (argv.indexOf('--new-key') > -1)
		rekey = true;
	if (argv.indexOf('--revoke') > -1) {
		revoke = true;
		renew = true;
	}
	if (argv.indexOf('--force') > -1)
		renew = true;
}

/*
 * Renew if the config has changed
 */
if (old_host != new_host)
	renew = true;
else if (new_domain_hash != old_domain_hash)
	renew = true;
else if (!days_remaining_at_least(30))
	renew = true;

// Nothing to be done.
if (!renew && !rekey && !revoke)
	exit(0);

/*
 * Now read in the system password which must be used to encrypt the 
 * private key.
 * 
 * TODO: What happens when the system password changes?
 */
if (!maincnf.open("rb", true))
	throw("Unable to open "+maincnf.name);
maincnf.position = 186; // Indeed.
syspass = maincnf.read(40);
syspass = syspass.replace(/\x00/g,'');
maincnf.close();

/*
 * Now open/create the keyset and RSA signing key for
 * ACME.  Note that this key is not the one used for the
 * final certificate.
 */
ks = new CryptKeyset(ks_fname, file_exists(ks_fname) ? CryptKeyset.KEYOPT.NONE : CryptKeyset.KEYOPT.CREATE);

/*
 * The ACME key uses the service hostname as the label.
 */
try {
	rsa = ks.get_private_key(new_host, syspass);
}
catch(e2) {
	rsa = new CryptContext(CryptContext.ALGO.RSA);
	rsa.keysize=2048/8;
	rsa.label=new_host;
	rsa.generate_key();
	ks.add_private_key(rsa, syspass);
}

/*
 * We store the key ID in our ini file so we don't need an extra
 * round-trip each session to discover it.
 */
settings.open(settings.exists ? "r+" : "w+");
key_id = settings.iniGetValue("key_id", new_host, undefined);
acme = new ACMEv2({key:rsa, key_id:key_id, user_agent:'LetSyncrypt '+("$Revision: 1.23 $".split(' ')[1])});
acme.host = new_host;
acme.dir_path = dir_path;
if (acme.key_id === undefined) {
	acme.create_new_account({termsOfServiceAgreed:true,contact:["mailto:sysop@"+system.inet_addr]});
}
/*
 * After the ACMEv2 object is created, we will always have a key_id
 * Write it to our ini if it wasn't there already.
 */
if (key_id === undefined) {
	settings.iniSetValue("key_id", new_host, acme.key_id);
	key_id = acme.key_id;
}
settings.close();

if (rekey) {
	rsa = new CryptContext(CryptContext.ALGO.RSA);
	rsa.keysize=2048/8;
	rsa.label=new_host;
	rsa.generate_key();
	acme.change_key(rsa);
	try {
		ks.delete_key(new_host);
	}
	catch(dkerr) {}
	ks.add_private_key(rsa, syspass);
}

if (revoke) {
	sks = new CryptKeyset(sks_fname, CryptKeyset.KEYOPT.READONLY);
	oldcert = sks.get_public_key("ssl_cert");
	sks.close();
	acme.revoke(oldcert);
	renew=true;
}

if (renew) {
	/*
	 * Create the order, using system.inet_addr
	 */
	for (i in webroots)
		identifiers.push({type:"dns",value:i});
	order = acme.create_new_order({identifiers:identifiers});

	/*
	 * Find an http-01 authorization
	 */
	try {
		for (auth in order.authorizations) {
			fulfilled = false;
			authz = acme.get_authorization(order.authorizations[auth]);
			if (authz.status == 'valid') {
				completed++;
				continue;
			}
			for (challenge in authz.challenges) {
				if (authz.challenges[challenge].type=='http-01') {
					/*
					 * Create a place to store the challenge and store it there
					 */
					for (i in webroots) {
						if (!file_isdir(webroots[i]+".well-known/acme-challenge")) {
							if (!mkpath(webroots[i]+".well-known/acme-challenge"))
								throw("Unable to create "+webroots[i]+".well-known/acme-challenge");
							tmp = new File(webroots[i]+".well-known/acme-challenge/webctrl.ini");
							tmp.open("w");
							tmp.writeln("AccessRequirements=");
							tmp.close();
						}
						token = new File(backslash(webroots[i]+".well-known/acme-challenge")+authz.challenges[challenge].token);
						if (tokens.indexOf(token.name) < 0) {
							token.open("w");
							token.write(authz.challenges[challenge].token+"."+acme.thumbprint());
							tokens.push(token.name);
							token.close();
						}
					}
					acme.accept_challenge(authz.challenges[challenge]);
					fulfilled = true;
				}
			}
			/*
			 * Wait for server to confirm
			 */
			if (fulfilled) {
				while (!acme.poll_authorization(order.authorizations[auth])) {
					mswait(1000);
				}
				completed++;
			}
		}
	}
	catch (autherr) {
		for (i in tokens)
			file_remove(tokens[i]);
		throw(autherr);
	}
	if (!completed)
		throw("No challenges fulfilled!");

	for (i in tokens)
		file_remove(tokens[i]);

	/*
	 * Create CSR
	 */
	csr = new CryptCert(CryptCert.TYPE.CERTREQUEST);

	/*
	 * We want to use a new key since there's no reason to
	 * keep using the old one, and changing the key often
	 * is good for security.
	 */

	rsa = new CryptContext(CryptContext.ALGO.RSA);
	rsa.keysize=2048/8;
	rsa.label="ssl_cert";
	rsa.generate_key();

	csr.subjectpublickeyinfo=rsa;
	csr.oganizationname=system.name;
	csr.commonname=system.inet_addr;
	for (i in webroots)
		dnsnames.push(i);
	csr.add_extension("2.5.29.17", false, create_dnsnames(dnsnames));
	csr.sign(rsa);
	csr.check();
	order = acme.finalize_order(order, csr);

	while (order.status !== 'valid') {
		order = acme.poll_order(order);
		if (order.status == 'invalid')
			throw("Order "+order.Location+" invalid!");
		mswait(1000);
	}

	cert = acme.get_cert(order);
	cert.label = "ssl_certchain";

	/*
	 * Now delete/create the keyset with the key and cert
	 */
	for (i=0; i < 10; i++) {
		if (file_remove(sks_fname))
			break;
		mswait(100);
	}
	if (i == 10)
		throw("Unable to delete file "+sks_fname);

	sks = new CryptKeyset(sks_fname, CryptKeyset.KEYOPT.CREATE);
	sks.add_private_key(rsa, syspass);
	sks.add_public_key(cert);
	sks.close();

	/*
	 * Recycle webserver
	 */
	file_touch(recycle_sem);

	/*
	 * Save the domain hash and any other state information.
	 * If the certificate was from the staging server, note that, so when
	 * we move to non-staging, we can update automatically.
	 */
	if (settings.open(settings.exists ? "r+" : "w+")) {
		settings.iniSetValue("State", "DomainHash", new_domain_hash);
		settings.iniSetValue("State", "Host", new_host);
		settings.iniRemoveKey("State", "Staging");
		settings.close();
	}
	else {
		// SO CLOSE!
		log(LOG_ERR, "!ERROR! Unable to save state after getting certificate");
		log(LOG_ERR, "!ERROR! THIS IS VERY BAD");
		throw("Unable to open "+settings.name+" to save state information!");
	}
}
