// $Id: install-xtrn.js,v 1.1 2020/03/26 09:55:22 rswindell Exp $

// Installer for Synchronet External Programs

// This script parses a .ini file (default filename is install-xtrn.ini)
// and installs the external programs defined within into the Synchronet BBS
// configuration file: ctrl/xtrn.cnf.  The programs defined within this file
// can by online programs (a.k.a. Doors), editors, or events.
//
// This script is intended for use by sysops using JSexec, for example:
//   $ jsexec install-xtrn ../xtrn/minesweeper
//
// This script can aso be invoked using the ;EXEC sysop command while logged
// into the terminal server, for example:
//   ;exec ?install-xtrn ../xtrn/minesweeper
//
// The .ini sections and keys supported (zero or more of each may be included):
//
// [prog:<code>]
// 		name 			= Program name or description (40 chars max)
//		cmd 			= command-line to execute (63 chars max)
//		clean_cmd 		= clean-up command-line, if needed (63 chars max)
//		settings 		= bit-flags (see XTRN_* in sbbsdefs.js)
//		ars				= access requirements string (40 chars max)
//		execution_ars	= execution requirements string (40 chars max)
//		type			= drop-file type (see XTRN_* in sbbsdefs.js)
//		event			= event-type (see EVENT_* in sbbsdefs.j)
//		cost			= cost to run, in credits
//		startup_dir		= directory to make current before execution
//		textra			= extra time (minutes) to allow to run this program
//		max_time		= maximum time (minutes) allowed to run this program
//
// [event:<code>]
//		cmd 			= command-line to execute (63 chars max)
//		days			= bit-field representing days of the week to execute
//		time			= time of day to run this event
//		node_num		= node number to run this event
//		settings		= bit-flags (see XTRN_* in sbbsdefs.js)
//		startup_dir		= directory to make current before execution
//		freq			= frequency of execution
//		mdays			= days of month (if non-zero) for execution
//		months			= bit-field representing which months to execute
//
// [editor:<code>]
//		name 			= Editr name or description (40 chars max)
//		cmd 			= command-line to execute (63 chars max)
//		type			= drop-file type (see XTRN_* in sbbsdefs.js)
//		settings 		= bit-flags (see XTRN_* in sbbsdefs.js)
//		ars				= access requirements string (40 chars max)

// Additionally, each object can have the following optional keys that are only
// used by this script
//		note			= Note to sysop displayed before installation
//		required		= If true, this item must be installed to continue
//
// Notes:
//
// - The startup_dir will default to the location of the .ini file if this
//   key is not defined within the .ini file.
//
// - The only required values are the <code> (internal code, 8 chars max) and
//   cmd; all other keys will have functional default values if not defined in
//   the .ini file.

"use strict";

load("sbbsdefs.js");
const ini_fname = "install-xtrn.ini"
var debug = false;

function install_xtrn_item(cnf, type, desc, item)
{
	if (!item.code)
		return false;

	if (!item.name)
		item.name = item.code;
	
	if (item.note)
		print(item.note);
	
	if (!confirm("Install " + desc + ": " + item.name)) {
		if (item.required == true)
			return "Installation of " + item.name + " is required to continue";
		return false;
	}
	
	if(type == "xtrn") {
		if (!xtrn_area.sec_list.length)
			return "No external program sections have been created";
		
		for (var i = 0; i < xtrn_area.sec_list.length; i++)
			print(format("%2u: ", i + 1) + xtrn_area.sec_list[i].name);

		var which;
		while (!which || which > xtrn_area.sec_list.length)
			which = prompt("Install " + item.name  + " into which External Program Section");
		which = parseInt(which, 10);
		if (!which)
			return false;
		
		item.sec = xtrn_area.sec_list[which - 1].number;
	}
	
	function find_code(objs, code)
	{
		for(var i=0; i < objs.length; i++)
			if(objs[i].code.toLowerCase() == code.toLowerCase())
				return i;
		return -1;
	}
	
	while (!item.code 
		|| (find_code(cnf[type], item.code) >= 0
			&& print(desc + " Internal Code (" + item.code + ") already exists!")))
		item.code = prompt(desc + " Internal code");

	try {
		item.code = item.code.toUpperCase();
		if (item.settings)
			item.settings = eval(item.settings);
		if (item.event)
			item.event = eval(item.event);
		if (item.type)
			item.type = eval(item.type);
	} catch(e) {
		return e;
	}
	cnf[type].push(item);
	if(debug)
		print(JSON.stringify(cnf[type], null, 4));
	
	print(desc + " (" + item.name + ") installed successfully");
	return true;
}

function main(ini_fname)
{
	var installed = 0;
	var banner = "* Installing " + ini_fname + " use Ctrl-C to abort *";
	var line = "";
	for (var i = 0; i < banner.length; i++)
		line += "*";
	print(line);
	print(banner);
	print(line);
	
	var ini_file = new File(ini_fname);
	if (!ini_file.open("r"))
		return ini_file.name + " open error " + ini_file.error;

	var cnflib = load({}, "cnflib.js");
	var xtrn_cnf = cnflib.read("xtrn.cnf");
	if (!xtrn_cnf)
		return "Failed to read xtrn.cnf";
	
	var startup_dir = ini_fname.substr(0, Math.max(ini_fname.lastIndexOf("/"), ini_fname.lastIndexOf("\\"), 0));

	const types = {
		prog:	{ desc: "External Program", 	struct: "xtrn" },
		event:	{ desc: "External Timed Event", struct: "event" },
		editor:	{ desc: "External Editor",		struct: "xedit" }
	};
	
	for(var t in types) {
		var list = ini_file.iniGetAllObjects("code", t + ":");
		for (var i = 0; i < list.length; i++) {
			if (list[i].startup_dir === undefined)
				list[i].startup_dir = startup_dir;
			var result = install_xtrn_item(xtrn_cnf, types[t].struct, types[t].desc, list[i]);
			if (typeof result !== 'boolean')
				return result;
			if (result === true)
				installed++;
		}
	}
	
	if (installed) {
		if (!debug && !cnflib.write("xtrn.cnf", undefined, xtrn_cnf))
			return "Failed to write xtrn.cnf";
		print("Installed " + installed + " items from " + ini_fname + " successfully");
	}

	return installed >= 1; // success
}

// Command-line argument parsing
var ini_path;
for (var i = 0; i < argc; i++) {
	switch (argv[i]) {
		case "-debug":
			debug = true;
			break;
		default:
			ini_path = argv[i];
			break;
	}
}
// Locate the .ini file
if (file_isdir(ini_path))
	ini_path = backslash(ini_path) + ini_fname;
while (!ini_path || !file_exists(ini_path)) {
	ini_path = prompt("Location of " + ini_fname);
	if (file_isdir(ini_path))
		ini_path = backslash(ini_path) + ini_fname;
}
		
var result = main(ini_path);
if (result !== true) {
	if (typeof result !== 'boolean')
		alert(result);
	exit(1);
}
