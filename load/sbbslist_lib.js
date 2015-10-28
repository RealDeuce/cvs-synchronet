// $Id: sbbslist_lib.js,v 1.5 2015/10/28 07:45:04 rswindell Exp $

// Synchronet BBS List (SBL) v4 Library

load("synchronet-json.js"); // For compatibility with older verisons of Synchronet/JavaScript-C without the JSON object

var list_fname = system.data_dir + 'sbbslist.json';

var sort_property = 'name';

// These max lengths are derived from the bbs_t structure definition in xtrn/sbl/sbldefs.h:
const max_len = {
	name:				25,
	phone_number:		25,		/* only the first 12 chars are backwards compatible with SBL v3 */
	location:			30,
	sysop_name:			25,
	created_by:			25,
	updated_by:			25,
	verified_by:		25,
	created_on:			28,
	updated_on:			28,
	verified_on:		28,
	service_address:	28,
	bbs_software:		15,
	since:				4,		/* just the year portion of the first_online date */
	nodes:				4,
	users:				5,
	subs:				5,
	dirs:				5,
	xtrns:				5,
	msgs:				8,
	files:				7,
	storage:			7,
	protocols:			60,
	tcp_services:		33,
	udp_services:		20,
	web_site:			60,
	networks:			60,
	description:		250,
};

function compare(a, b)
{
	var val1="";
	var val2="";

	val1 = property_sort_value(a, sort_property);
	val2 = property_sort_value(b, sort_property);

	if(typeof(val1) == "string")
		val1=val1.toLowerCase();
	if(typeof(val2) == "string")
		val2=val2.toLowerCase();

	if(val1>val2) return 1;
	if(val1<val2) return -1;
	return 0;
}

function verify_compare(a, b)
{
    var diff;

    if(a.entry.verified && a.entry.verified.on && b.entry.verified && b.entry.verified.on)
        diff = new Date(b.entry.verified.on.substr(0,10)).getTime() - new Date(a.entry.verified.on.substr(0,10)).getTime();

    if(diff) return diff;

    if(a.entry.autoverify !=undefined && b.entry.autoverify != undefined) {
        diff = b.entry.autoverify.successes - a.entry.autoverify.successes;

        if(diff) return diff;

        diff = a.entry.autoverify.attempts - b.entry.autoverify.attempts;
    }

    return diff;
}

function date_string(date)
{
	return new Date(date).toString().substr(4,11);
}

/* Some properties are nested within arrays of objects, lets simplify those properties here */
function property_value(bbs, property)
{
	var value="";

	switch(property) {
		case "service_address":
			if(bbs.service) {
				for(var i in bbs.service) {
					if(bbs.service[i].protocol != 'modem') {
						value = bbs.service[i].address;
						break;
					}
				}
			}
			break;
		case "phone_number":
			if(bbs.service) {
				for(var i in bbs.service) {
					if(bbs.service[i].protocol == 'modem') {
						value = bbs.service[i].address;
						break;
					}
				}
			}
			break;
		case "sysop_name":
			if(bbs.sysop && bbs.sysop.length)
				value = bbs.sysop[0].name;
			break;
		case "created_by":
			value = bbs.entry.created.by;
			break;
		case "created_on":
			value = date_string(bbs.entry.created.on);
			break;
		case "updated_by":
			if(bbs.entry.updated)
				value = bbs.entry.updated.by;
			break;
		case "updated_on":
			if(bbs.entry.updated)
				value = date_string(bbs.entry.updated.on);
			break;
		case "verified_by":
			if(bbs.entry.verified)
				value = bbs.entry.verified.by;
			break;
		case "verified_on":
			if(bbs.entry.verified)
				value = date_string(bbs.entry.verified.on);
			break;
		case "since":
			if(bbs.first_online)
				value = bbs.first_online.substring(0,4);
			break;
		case "nodes":
			if(bbs.terminal && bbs.terminal.nodes)
				value = bbs.terminal.nodes;
			break;
		case "users":
			if(bbs.total && bbs.total.users)
				value = bbs.total.users;
			break;
		case "subs":
			if(bbs.total && bbs.total.subs)
				value = bbs.total.subs;
			break;
		case "dirs":
			if(bbs.total && bbs.total.dirs)
				value = bbs.total.dirs;
			break;
		case "xtrns":
			if(bbs.total && bbs.total.xtrns)
				value = bbs.total.xtrns;
			break;
		case "msgs":
			if(bbs.total && bbs.total.msgs)
				value = bbs.total.msgs;
			break;
		case "files":
			if(bbs.total && bbs.total.files)
				value = bbs.total.files;
			break;
		case "storage":
			if(bbs.total && bbs.total.storage) {
				if(bbs.total.storage > 1024*1024*1024*1024)
					value = Math.ceil(bbs.total.storage / (1024*1024*1024*1024)) + "TB";
				else if(bbs.total.storage > 1024*1024*1024)
					value = Math.ceil(bbs.total.storage / (1024*1024*1024)) + "GB";
				else if(bbs.total.storage > 1024*1024)
					value = Math.ceil(bbs.total.storage / (1024*1024)) + "MB";
				else
					value = Math.ceil(bbs.total.storage / 1024) + "KB";
			}
			break;				
		case "description":
			value = bbs.description.join(" ");
			break;
		case "bbs_software":
			if(bbs.software && bbs.software.bbs)
				value = bbs.software.bbs;
			break;
		case "networks":
			if(bbs.network && bbs.network.length) {
				var networks = [];
				for(var i in bbs.network) {
					if(networks.indexOf(bbs.network[i].name) < 0)
						networks.push(bbs.network[i].name);
				}
				for(var i in networks) {
					if(i > 0) value += ", ";
					value += networks[i];
				}
			}
			break;
		case "protocols":
		{
			var protocols = [];
			if(bbs.service && bbs.service.length) {
				for(var i in bbs.service) {
					if(bbs.service[i].protocol == 'modem')
						continue;
					if(protocols.indexOf(bbs.service[i].protocol) < 0)
						protocols.push(bbs.service[i].protocol);
				}
			}
			if(bbs.entry 
				&& bbs.entry.autoverify 
				&& bbs.entry.autoverify.last_success
				&& bbs.entry.autoverify.last_success.other_services) {
				protocols = protocols.concat(bbs.entry.autoverify.last_success.other_services.tcp);
			}
			value = protocols.join();
			break;
		}
		case "tcp_services":
			if(bbs.entry 
				&& bbs.entry.autoverify 
				&& bbs.entry.autoverify.last_success
				&& bbs.entry.autoverify.last_success.other_services) {
				value = bbs.entry.autoverify.last_success.other_services.tcp;
			}
			break;
		case "udp_services":
			if(bbs.entry 
				&& bbs.entry.autoverify 
				&& bbs.entry.autoverify.last_success
				&& bbs.entry.autoverify.last_success.other_services) {
				value = bbs.entry.autoverify.last_success.other_services.udp;
			}
			break;
		default:
			if(bbs[property])
				value = bbs[property];
			break;
	}
	return value;
}

/* Some properties are nested within arrays of objects, lets simplify those properties here */
function property_sort_value(bbs, property)
{
	var value="";

	switch(property) {
		case "storage":
			if(bbs.total)
				return bbs.total.storage;
			break;
		case "created_on":
			return bbs.entry.created.on;
		case "updated_on":
			if(bbs.entry.updated)
				return bbs.entry.updated.on;
			break;
		case "verified_on":
			if(bbs.entry.verified)
				return bbs.entry.verified.on;
			break;
	}
	return property_value(bbs, property);
}

function sort(list, property)
{
	if(property=="verify")
		list.sort(verify_compare);
	else {	
		if(property)
			sort_property=property;
		list.sort(compare);
	}
}

function find(list, text)
{
	var new_list=[];
	var text=text.toUpperCase();

	for(var i in list)
		if(JSON.stringify(list[i]).toUpperCase().indexOf(text) >= 0)
			new_list.push(list[i]);
	return new_list;
}

function read_list()
{
    var f = new File(list_fname);
    log(LOG_INFO, "Opening list file: " + f.name);
    if(!f.open("r")) {
        log(LOG_ERR, "SBBSLIST: Error " + f.error + " opening " + f.name);
        return null;
    }

    var buf = f.read();
    f.close();
	truncsp(buf);
    var list = [];
	if(buf.length) {
		try {
			list=JSON.parse(buf);
		} catch(e) {
			log(LOG_ERR, "SBBSLIST: JSON.parse exception: " + e);
		}
	}
    return list;
}

function write_list(list)
{
    var out = new File(list_fname);
    log(LOG_INFO, "SBBSLIST: Opening / creating list file: " + list_fname);
    if(!out.open("w+")) {
        log(LOG_ERR, "SBBSLIST: Error " + out.error + " creating " + out.name);
        return false;
    }

    log(LOG_INFO, "SBBSLIST: Writing list file: " + out.name + " (" + list.length + " BBS entries)");
    out.write(JSON.stringify(list, null, 4));
    out.close();
    return true;
}

function remove_dupes(list)
{
    var new_list=[];
    var names=[];
    var address=[];
    var i;

    for(i in list)
        if(names.indexOf(list[i].name.toLowerCase()) < 0 && address.indexOf(list[i].service[0].address.toLowerCase()) < 0) {
            names.push(list[i].name.toLowerCase());
            address.push(list[i].service[0].address.toLowerCase());
            new_list.push(list[i]);
        }

    return new_list;
}

function imsg_capable_system(bbs)
{
	if(!bbs.entry.autoverify || !bbs.entry.autoverify.last_success)
		return false;
	var services = bbs.entry.autoverify.last_success.other_services;
	if(services.udp.indexOf("finger")<0	&& services.udp.indexOf("systat")<0)
		return false;
	if(services.tcp.indexOf("msp")<0 && services.tcp.indexOf("smtp")<0 && services.tcp.indexOf("submission")<0)
		return false;
	return true;
}

function system_index(list, name)
{
    name = name.toLowerCase();
    for(var i in list)
        if(list[i].name.toLowerCase() == name)
            return i;

    return -1;
}


function system_exists(list, name)
{
    return system_index(list, name) >= 0;
}

function new_system(name, nodes, stats)
{
	return {
		name: name, 
		entry:{}, 
		sysop:[], 
		service:[], 
		terminal:{ nodes: nodes, support: [] }, 
		network:[], 
		description:[], 
		total: stats
	};
}

function system_stats()
{
	return {
		users: system.stats.total_users, 
		subs: Object.keys(msg_area.sub).length, 
		dirs: Object.keys(file_area.dir).length, 
		xtrns: Object.keys(xtrn_area.prog).length, 
		storage:disk_size(system.data_dir, 1024*1024)*1024*1024, 
		msgs: system.stats.total_messages, 
		files: system.stats.total_files
	};
}

function add_system(list, bbs, by)
{
	bbs.entry.created = { by: by, on: new Date() };
	list.push(bbs);
}

function update_system(bbs, by)
{
	bbs.entry.updated = { by: by, on: new Date() };
}

function syncterm_list(list)
{
    var f = new File("syncterm.lst");
    if(!f.open("w")) {
		return false;
    }
    for(i in list) {
        for(j in list[i].service) {
            if(!list[i].service[j].protocol)
                continue;
            if(j > 0)
                f.writeln(format("[%-23.23s %6.6s]", list[i].name, list[i].service[j].protocol));
            else
                f.writeln(format("[%s]", list[i].name));
            f.writeln(format("ConnectionType=%s", list[i].service[j].protocol));
            f.writeln(format("Address=%s", list[i].service[j].address));
            f.writeln();
        }
    }
    f.close();
	return f.name;
}


/* Leave as last line for convenient load() usage: */
this;
