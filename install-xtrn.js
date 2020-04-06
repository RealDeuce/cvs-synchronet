// $Id: install-xtrn.js,v 1.4 2020/04/06 03:34:47 rswindell Exp $

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
// 		name 			= program name or description (40 chars max)
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
//		name 			= editor name or description (40 chars max)
//		cmd 			= command-line to execute (63 chars max)
//		type			= drop-file type (see XTRN_* in sbbsdefs.js)
//		settings 		= bit-flags (see XTRN_* in sbbsdefs.js)
//		ars				= access requirements string (40 chars max)
//
// [exec:<file>.js [args]]  ; execute file.js with these arguments
//		startup_dir		= directory to make current before execution
//
// [eval:<js-expression>]
//      cmd             = evaluate this string rather than the js-expression

// Additionally, each section can have the following optional keys that are
// only used by this script (i.e. not written to any configuration files):
//		note			= note to sysop displayed before installation
//      prompt          = confirmation prompt (or false if no prompting)
//		required		= if true, this item must be installed to continue
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

const REVISION = "$Revision: 1.4 $".split(' ')[1];
const ini_fname = "install-xtrn.ini";

load("sbbsdefs.js");

var options = {
	debug: false,
	overwrite: false
};

function aborted()
{
	if(js.terminated || (js.global.console && console.aborted))
		exit(1);
	return false;
}

function install_xtrn_item(cnf, type, desc, item)
{
	if (!item.code)
		return false;

	if (!item.name)
		item.name = item.code;
	
	function find_code(objs, code)
	{
		if (!options.overwrite) {
			for (var i=0; i < objs.length; i++)
				if (objs[i].code.toLowerCase() == code.toLowerCase())
					return i;
		}
		return -1;
	}
	
	if(find_code(cnf[type], item.code) >= 0) {
		if(options.auto
			|| deny(desc + " (" + item.code + ") already exists, continue"))
			return false;
	}
	
	while (!item.code && !aborted()
		|| (find_code(cnf[type], item.code) >= 0
			&& print(desc + " Internal Code (" + item.code + ") already exists!")))
		item.code = js.global.prompt(desc + " Internal code");

	if(aborted())
		return false;
	
	if (item.note)
		print(item.note);
	
	var prompt = "Install " + desc + ": " + item.name;
	if (item.prompt !== undefined)
		prompt = item.prompt;
	
	if (prompt && !confirm(prompt)) {
/**		
		if (item.required == true)
			return "Installation of " + item.name + " is required to continue";
**/
		return false;
	}
	
	if (type == "xtrn") {
		if (!xtrn_area.sec_list.length)
			return "No external program sections have been created";
		
		for (var i = 0; i < xtrn_area.sec_list.length; i++)
			print(format("%2u: ", i + 1) + xtrn_area.sec_list[i].name);

		var which;
		while ((!which || which > xtrn_area.sec_list.length) && !aborted())
			which = js.global.prompt("Install " + item.name  + " into which External Program Section");
		if(aborted())
			return false;
		which = parseInt(which, 10);
		if (!which)
			return false;
		
		item.sec = xtrn_area.sec_list[which - 1].number;
	}


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
	if (options.debug)
		print(JSON.stringify(cnf[type], null, 4));
	
	print(desc + " (" + item.name + ") installed successfully");
	return true;
}

function install(ini_fname)
{
	ini_fname = fullpath(ini_fname);
	var installed = 0;
	if(!options.auto) {
		var banner = "* Installing " + ini_fname + " use Ctrl-C to abort *";
		var line = "";
		for (var i = 0; i < banner.length; i++)
			line += "*";
		print(line);
		print(banner);
		print(line);
	}
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
	
	for (var t in types) {
		var list = ini_file.iniGetAllObjects("code", t + ":");
		for (var i = 0; i < list.length; i++) {
			var item = list[i];
			if (item.startup_dir === undefined)
				item.startup_dir = startup_dir;
			var result = install_xtrn_item(xtrn_cnf, types[t].struct, types[t].desc, item);
			if (typeof result !== 'boolean')
				return result;
			if (result === true)
				installed++;
			else if(item.required)
				return false;
		}
	}
	
	var list = ini_file.iniGetAllObjects("cmd", "exec:");
	for (var i = 0; i < list.length; i++) {
		var item = list[i];
		var js_args = item.cmd.split(/\s+/);
		var js_file = js_args.shift();
		
		if (file_getext(js_file).toLowerCase() != ".js")
			return "Only '.js' files may be executed: " + js_file;

		if (item.note)
			print(item.note);
		
		var prompt = "Execute: " + item.cmd;
		if (item.prompt !== undefined)
			prompt = item.prompt;
	
		if (prompt && !confirm(prompt)) {
			if (item.required == true)
				return prompt + " is required to continue";
			continue;
		}

		if (item.startup_dir === undefined)
			item.startup_dir = startup_dir;
		if (!item.args)
			items.args = "";
		var result = js.exec.apply(null
			,[js_file, item.startup_dir, {}].concat(js_args));
		if (result !== 0 && item.required)
			return "Error " + result + " executing " + item.cmd;
	}
	
	var list = ini_file.iniGetAllObjects("str", "eval:");
	for (var i = 0; i < list.length; i++) {
		var item = list[i];
		if (!item.cmd)
			item.cmd = item.str; // the str can't contain [], so allow cmd to override
		var prompt = "Evaluate: " + item.cmd;
		if (item.prompt !== undefined)
			prompt = item.prompt;
		if (prompt && !confirm(prompt)) {
			if (item.required == true)
				return prompt + " is required to continue";
			continue;
		}
		if (!eval(item.cmd)) {
			if (item.required == true)
				return "Truthful evaluation of '" + item.cmd + "' is required to continue";
		}
	}

	if (installed) {
		if (!options.debug && !cnflib.write("xtrn.cnf", undefined, xtrn_cnf))
			return "Failed to write xtrn.cnf";
		print("Installed " + installed + " items from " + ini_fname + " successfully");
	}

	return installed >= 1; // success
}

print(js.exec_file + " v" + REVISION);

// Command-line argument parsing
var ini_list = [];
for (var i = 0; i < argc; i++) {
	if (argv[i][0] == '-')
		options[argv[i].substr(1)] = true;
	else
		ini_list.push(argv[i]);
}

var xtrn_dirs = fullpath(system.ctrl_dir + "../xtrn/*");
if(!ini_list.length) {
	var dir_list = directory(xtrn_dirs);
	for(var d in dir_list) {
		var fname = file_getcase(dir_list[d] + ini_fname);
		if(fname)
			ini_list.push(fname);
	}
}

if(!ini_list.length) {
	if(options.auto) {
		alert("No install files (" + ini_fname + ") found in " + xtrn_dirs);
		exit(0);
	}
	var ini_path;
	while (!ini_path || !file_exists(ini_path)) {
		ini_path = prompt("Location of " + ini_fname);
		if (file_isdir(ini_path))
			ini_path = backslash(ini_path) + ini_fname;
	}
	ini_list.push(ini_path);
}

var installed = 0;
for(var i in ini_list) {
	var ini_path = ini_list[i];
	// Locate the .ini file
	if (file_isdir(ini_path))
		ini_path = backslash(ini_path) + ini_fname;
	if (!file_exists(ini_path)) {
		alert(ini_path + " does not exist");
		continue;
	}
		
	var result = install(ini_path);
	if(aborted())
		break;
	if (result === true)
		installed++;
	else if (typeof result !== 'boolean')
		alert(result);
}
print("Installed " + installed + " external programs.");
