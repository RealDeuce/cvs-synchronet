/* testbuild.js */

/* JSexec script for periodic Synchronet test builds */

/* $Id$ */

load("sbbsdefs.js");

var date_str = strftime("%b-%d-%y");	/* mmm-dd-yy */

var temp_dir = backslash(system.temp_path) + "sbbs-" + date_str;

if(!file_isdir(temp_dir) && !mkdir(temp_dir)) {
	log(LOG_ERR,"!Failed to create temp directory: " + temp_dir);
	exit(1);
}
log(LOG_INFO,"Using temp directory: " + temp_dir);

putenv("CVSROOT=:pserver:anonymous@cvs.synchro.net:/cvsroot/sbbs");

var platform = system.platform.toLowerCase();
var make = (platform=="win32" ? "make":"gmake");
var msdev = '"C:\\Program Files\\Microsoft Visual Studio\\Common\\MSDev98\\Bin\\msdev"';
var build_output = "build_output.txt";

if(platform=="win32") {
	archive="sbbs_src.zip";
	archive_cmd="pkzip25 -exclude=*output.txt -add -dir " + archive;
} else {
	archive="sbbs_src.tgz";
	archive_cmd="tar --exclude=*output.txt -czvf " + archive + " *";
}

var builds
	=	[/* sub-dir */		/* cmd-line */
		["",				"cvs co src-sbbs3"],
		["",				"cvs co lib-" + platform + ".debug"],
		["",				"cvs co lib-" + platform + ".release"],
		["",				archive_cmd],
		["src/sbbs3",		make + " DEBUG=1"],
		["src/sbbs3",		make + " RELEASE=1"],
		["src/sbbs3/scfg",	make + " DEBUG=1"],
		["src/sbbs3/scfg",	make + " RELEASE=1"],
	];

/* Platform-specific (or non-ported) projects */
if(platform=="win32") {
	/* Requires Visual C++ */
	builds.push(["src/sbbs3",			msdev + " sbbs3.dsw /MAKE ALL /OUT "+ build_output]);
	/* Requires C++Builder */
	builds.push(["src/sbbs3/ctrl",		"makelibs.bat"]);
	builds.push(["src/sbbs3/ctrl",		"bpr2mak sbbsctrl.bpr & make -f sbbsctrl.mak"]);
	builds.push(["src/sbbs3/chat",		"bpr2mak chat.bpr     & make -f chat.mak"]);
} else {	/* Unix */
	builds.push(["src/sbbs3/install",	"gmake"]);
	builds.push(["src/sbbs3/umonitor",	"gmake"]);
	builds.push(["src/sbbs3/uedit",		"gmake"]);
}

chdir(temp_dir);

var system_description=system.local_host_name + " - " + system.os_version;

var file = new File("README.TXT");
if(file.open("wt")) {
	file.writeln(format("Synchronet-%s C/C++ Source Code Archive (%s)\n"
		,system.platform, system.datestr()));
	file.writeln("This archive contains a snap-shot of all the source code and library files"
	file.writeln("necessary for a successful " + system.platform + " build of the following"
	file.writeln("Synchronet projects as of " + system.datestr() + ":");
	file.writeln();
	file.writeln(format("%-20s %s", "Project Directory", "Build Command"));
	for(i in builds) {
		if(builds[i][0].length)
			file.writeln(format("%-20s %s", builds[i][0], builds[i][1]));
	}
	file.writeln();
	file.writeln(format("Builds verified on %s by %s", system.timestr(), system_description));
	file.writeln();
	file.writeln("For more details, see http://synchro.net/docs/source.html");
	if(platform!="win32")
		file.writeln("and http://synchro.net/docs/sbbsunix.txt");
	file.close();
}

var file = new File("FILE_ID.DIZ");
if(file.open("wt")) {
	file.writeln(format("Synchronet-%s C/C++ source code archive",system.platform));
	file.writeln(format("(%s)",system.datestr()));
	file.close();
}

var start = time();

for(i in builds) {
	var sub_dir = builds[i][0];
	var build_dir = temp_dir + "/" + sub_dir;
	var subject = system.platform + " build failure in " + sub_dir;

	if(sub_dir.length)
		log(LOG_INFO, "Build sub-directory: " + sub_dir);
	if(!chdir(build_dir)) {
		semd_email(subject, log(LOG_ERR,"!FAILED to chdir to: " + build_dir));
		exit(1);
	}

	builds[i].start = time();

	var cmd_line = builds[i][1];
	if(cmd_line.indexOf(build_output)<0)
		cmd_line += " 2> " + build_output;
	log(LOG_INFO, "Executing: " + cmd_line);
	var retval=system.exec(cmd_line);
	if(retval) {
		send_email(subject, 
			log(LOG_ERR,"!ERROR " + retval + " executing: '" + cmd_line + "' in " + sub_dir) 
			+ "\n\n" + file_contents(build_output));
		exit(1);
	}

	builds[i].end = time();
}

var body = "System: " + system_description + "\n\n";

for(i in builds) {
	body += elapsed_time(builds[i].end-builds[i].start) + " - ";
	body += builds[i][0] + "\t" + builds[i][1];
	body += "\n";
}

body += "-----\n";
body += elapsed_time(time()-start) + " - total\n";

send_email(system.platform + " builds successful", lfexpand(body));

chdir(temp_dir);
var dest = file_area.dir["sbbs"].path+archive;
log(LOG_INFO,format("Copying %s to %s",archive,dest));
if(!file_copy(archive,dest))
	log(LOG_ERR,format("!ERROR copying %s to %s",archive,dest));

/* end */

function elapsed_time(t)
{
	return format("%02u:%02u", t/60, t%60);
}

function file_contents(fname)
{
	var file = new File(fname);
	if(!file.open("rt")) {
		return log(LOG_ERR, "!ERROR " + errno_str + " opening " + fname);
	}
	var msgtxt = lfexpand(file.read(file.length));
	file.close();
	delete file;

	return(msgtxt);
}

function send_email(subject, body)
{
	var msgbase = new MsgBase("mail");
	if(msgbase.open()==false) {
		log("!ERROR " + msgbase.last_error);
		return(false);
	}

	var hdr = { 
		to: "sysop", 
		to_ext: 1, 
		from: "Synchronet testbuild.js", 
		subject: subject
	};

	if(!msgbase.save_msg(hdr, body))
		log(LOG_ERR, "!ERROR " + msgbase.last_error + " saving mail message");
	else
		log(LOG_INFO, "E-mail sent.");

	msgbase.close();
}