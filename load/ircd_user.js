// $Id$
//
// ircd_unreg.js
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details:
// http://www.gnu.org/licenses/gpl.txt
//
// Synchronet IRC Daemon as per RFC 1459, link compatible with Bahamut 1.4
//
// Copyright 2003-2004 Randolph Erwin Sommerfeld <sysop@rrx.ca>
//
// ** Handle registered clients.
//

////////// Constants / Defines //////////
const USER_REVISION = "$Revision$".split(' ')[1];

const USERMODE_NONE		=(1<<0); // NONE
const USERMODE_OPER		=(1<<1); // o
const USERMODE_INVISIBLE	=(1<<2); // i
const USERMODE_WALLOPS		=(1<<3); // w
const USERMODE_CHATOPS		=(1<<4); // b
const USERMODE_GLOBOPS		=(1<<5); // g
const USERMODE_SERVER		=(1<<6); // s
const USERMODE_CLIENT		=(1<<7); // c
const USERMODE_REJECTED		=(1<<8); // r
const USERMODE_KILL		=(1<<9); // k
const USERMODE_FLOOD		=(1<<10); // f
const USERMODE_SPY		=(1<<11); // y
const USERMODE_DEBUG		=(1<<12); // d
const USERMODE_ROUTING		=(1<<13); // n
const USERMODE_HELP		=(1<<14); // h
const USERMODE_NOTHROTTLE	=(1<<15); // F

USERMODE_CHAR = new Array();
USERMODE_CHAR["o"] = USERMODE_OPER;
USERMODE_CHAR["i"] = USERMODE_INVISIBLE;
USERMODE_CHAR["w"] = USERMODE_WALLOPS;
USERMODE_CHAR["b"] = USERMODE_CHATOPS;
USERMODE_CHAR["g"] = USERMODE_GLOBOPS;
USERMODE_CHAR["s"] = USERMODE_SERVER;
USERMODE_CHAR["c"] = USERMODE_CLIENT;
USERMODE_CHAR["r"] = USERMODE_REJECTED;
USERMODE_CHAR["k"] = USERMODE_KILL;
USERMODE_CHAR["f"] = USERMODE_FLOOD;
USERMODE_CHAR["y"] = USERMODE_SPY;
USERMODE_CHAR["d"] = USERMODE_DEBUG;
USERMODE_CHAR["n"] = USERMODE_ROUTING;
USERMODE_CHAR["h"] = USERMODE_HELP;
USERMODE_CHAR["F"] = USERMODE_NOTHROTTLE;

// Most umodes aren't propagated across the network. Define the ones that are.
USERMODE_BCAST = new Array;
USERMODE_BCAST["o"] = true;
USERMODE_BCAST["i"] = true;
USERMODE_BCAST["h"] = true;

// FIXME: Services modes are broadcast but not displayed to the user.
USERMODE_SERVICES = new Array;

// Various permissions that can be set on an O:Line
const OLINE_CAN_REHASH		=(1<<0);	// r
const OLINE_CAN_RESTART		=(1<<1);	// R
const OLINE_CAN_DIE		=(1<<2);	// D
const OLINE_CAN_GLOBOPS		=(1<<3);	// g
const OLINE_CAN_WALLOPS		=(1<<4);	// w
const OLINE_CAN_LOCOPS		=(1<<5);	// l
const OLINE_CAN_LSQUITCON	=(1<<6);	// c
const OLINE_CAN_GSQUITCON	=(1<<7);	// C
const OLINE_CAN_LKILL		=(1<<8);	// k
const OLINE_CAN_GKILL		=(1<<9);	// K
const OLINE_CAN_KLINE		=(1<<10);	// b
const OLINE_CAN_UNKLINE		=(1<<11);	// B
const OLINE_CAN_LGNOTICE	=(1<<12);	// n
const OLINE_CAN_GGNOTICE	=(1<<13);	// N
// Synchronet IRCd doesn't have umode +A	RESERVED
// Synchronet IRCd doesn't have umode +a	RESERVED
const OLINE_CAN_UMODEC		=(1<<16);	// c
const OLINE_CAN_CHATOPS		=(1<<19);	// s
const OLINE_CHECK_SYSPASSWD	=(1<<20);	// S
const OLINE_CAN_DEBUG		=(1<<21);	// x
const OLINE_IS_GOPER		=(1<<22);	//  "big O"

// Bits used for walking the complex WHO flags.
const WHO_AWAY			=(1<<0);	// a
const WHO_CHANNEL		=(1<<1);	// c
const WHO_REALNAME		=(1<<2);	// g
const WHO_HOST			=(1<<3);	// h
const WHO_IP			=(1<<4);	// i
const WHO_CLASS			=(1<<5);	// l
const WHO_UMODE			=(1<<6);	// m
const WHO_NICK			=(1<<7);	// n
const WHO_OPER			=(1<<8);	// o
const WHO_SERVER		=(1<<9);	// s
const WHO_TIME			=(1<<10);	// t
const WHO_USER			=(1<<11);	// u
const WHO_FIRST_CHANNEL		=(1<<12);	// C
const WHO_MEMBER_CHANNEL	=(1<<13);	// M
const WHO_SHOW_IPS_ONLY		=(1<<14);	// I

// Bits used for walking complex LIST flags.
const LIST_CHANMASK		=(1<<0);	// a
const LIST_CREATED		=(1<<1);	// c
const LIST_MODES		=(1<<2);	// m
const LIST_TOPIC		=(1<<3);	// o
const LIST_PEOPLE		=(1<<4);	// p
const LIST_TOPICAGE		=(1<<5);	// t
const LIST_DISPLAY_CHAN_MODES	=(1<<6);	// M

////////// Objects //////////
function IRC_User(id) {
	////////// VARIABLES
	// Bools/Flags that change depending on connection state.
	this.flagged_for_quit = false;	// QUIT later?
	this.local = true;	// are we a local socket?
	this.pinged = false;	// sent PING?
	this.server = false;	// No, we're not a server.
	this.uline = false;	// Are we services?
	// Variables containing user/server information as we receive it.
	this.away = "";
	this.channels = new Array;
	this.connecttime = time();
	this.created = 0;
	this.flags = 0;
	this.hops = 0;
	this.hostname = "";
	this.idletime = time();
	this.invited = "";
	this.ircclass = 0;
	this.mode = 0;
	this.nick = "";
	this.parent = 0;
	this.realname = "";
	this.servername = servername;
	this.talkidle = time();
	this.uprefix = "";
	this.id = id;
	// Variables (consts, really) that point to various state information
	this.socket = "";
	////////// FUNCTIONS
	// Functions we use to control clients (specific)
	this.quit = User_Quit;
	this.work = User_Work;
	// Socket functions
	this.ircout=ircout;
	this.originatorout=originatorout;
	this.rawout=rawout;
	// Output helper functions (shared)
	this.bcast_to_channel=IRCClient_bcast_to_channel;
	this.bcast_to_channel_servers=IRCClient_bcast_to_channel_servers;
	this.bcast_to_list=IRCClient_bcast_to_list;
	this.bcast_to_servers=IRCClient_bcast_to_servers;
	this.bcast_to_servers_raw=IRCClient_bcast_to_servers_raw;
	this.bcast_to_uchans_unique=IRCClient_bcast_to_uchans_unique;
	this.do_admin=IRCClient_do_admin;
	this.do_connect=IRCClient_do_connect;
	this.do_info=IRCClient_do_info;
	this.do_join=IRCClient_do_join;
	this.do_links=IRCClient_do_links;
	this.do_msg=IRCClient_do_msg;
	this.do_part=IRCClient_do_part;
	this.do_stats=IRCClient_do_stats;
	this.do_summon=IRCClient_do_summon;
	this.do_trace=IRCClient_do_trace;
	this.do_users=IRCClient_do_users;
	this.do_whois=IRCClient_do_whois;
	this.global=IRCClient_global;
	this.globops=IRCClient_globops;
	this.lusers=IRCClient_lusers;
	this.motd=IRCClient_motd;
	this.names=IRCClient_names;
	this.part_all=IRCClient_part_all;
	this.server_notice=IRCClient_server_notice;
	this.services_msg=IRCClient_services_msg;
	this.trace_all_opers=IRCClient_trace_all_opers;
	// WHO
	this.do_basic_who=IRCClient_do_basic_who;
	this.do_complex_who=IRCClient_do_complex_who;
	this.do_who_usage=IRCClient_do_who_usage;
	this.match_who_mask=IRCClient_match_who_mask;
	// LIST
	this.do_basic_list=IRCClient_do_basic_list;
	this.do_complex_list=IRCClient_do_complex_list;
	this.do_list_usage=IRCClient_do_list_usage;
	// Global functions
	this.check_nickname=IRCClient_check_nickname;
	this.check_timeout=IRCClient_check_timeout;
	this.get_usermode=IRCClient_get_usermode;
	this.netsplit=IRCClient_netsplit;
	this.onchanwith=IRCClient_onchanwith;
	this.rmchan=IRCClient_RMChan;
	this.setusermode=IRCClient_setusermode;
	this.set_chanmode=IRCClient_set_chanmode;
	// Numerics
	this.numeric=IRCClient_numeric;
	this.numeric200=IRCClient_numeric200;
	this.numeric201=IRCClient_numeric201;
	this.numeric202=IRCClient_numeric202;
	this.numeric203=IRCClient_numeric203;
	this.numeric204=IRCClient_numeric204;
	this.numeric205=IRCClient_numeric205;
	this.numeric206=IRCClient_numeric206;
	this.numeric208=IRCClient_numeric208;
	this.numeric261=IRCClient_numeric261;
	this.numeric321=IRCClient_numeric321;
	this.numeric322=IRCClient_numeric322;
	this.numeric331=IRCClient_numeric331;
	this.numeric332=IRCClient_numeric332;
	this.numeric333=IRCClient_numeric333;
	this.numeric351=IRCClient_numeric351;
	this.numeric352=IRCClient_numeric352;
	this.numeric353=IRCClient_numeric353;
	this.numeric382=IRCClient_numeric382;
	this.numeric391=IRCClient_numeric391;
	this.numeric401=IRCClient_numeric401;
	this.numeric402=IRCClient_numeric402;
	this.numeric403=IRCClient_numeric403;
	this.numeric411=IRCClient_numeric411;
	this.numeric412=IRCClient_numeric412;
	this.numeric440=IRCClient_numeric440;
	this.numeric441=IRCClient_numeric441;
	this.numeric442=IRCClient_numeric442;
	this.numeric445=IRCClient_numeric445;
	this.numeric446=IRCClient_numeric446;
	this.numeric451=IRCClient_numeric451;
	this.numeric461=IRCClient_numeric461;
	this.numeric462=IRCClient_numeric462;
	this.numeric481=IRCClient_numeric481;
	this.numeric482=IRCClient_numeric482;
	// Getters
	this.nuh getter = function() {
		return(this.nick + "!" + this.uprefix + "@" + this.hostname);
	};
}

////////// Command Parser //////////
function User_Work() {
	var cmdline;
	var cmd;
	var command;

	if (!this.socket.is_connected) {
		this.quit("Connection reset by peer");
		return 0;
	}

	cmdline=this.socket.recvline(4096,0)

	if (!cmdline)
		return 0;

	Global_CommandLine = cmdline;

	// Only accept up to 512 bytes from clients as per RFC1459.
	cmdline = cmdline.slice(0,512);
	// Kludge for broken clients.
	if ((cmdline[0] == "\r") || (cmdline[0] == "\n"))
		cmdline = cmdline.slice(1);
	if (debug)
		log(format("[%s<-%s]: %s",servername,this.nick,cmdline));
	cmd = cmdline.split(" ");
	if (cmdline[0] == ":") {
		// Silently ignore NULL originator commands.
		if (!cmd[1])
			return 0;
		// if :<originator> doesn't match nick of originating
		// socket, drop silently per RFC.
		if (cmd[0].slice(1).toUpperCase() != this.nick.toUpperCase())
			return 0;
		command = cmd[1].toUpperCase();
		cmdline = cmdline.slice(cmdline.indexOf(" ")+1);
		// resplit cmd[]
		cmd = cmdline.split(" ");
	} else {
		command = cmd[0].toUpperCase();
	}

	// Ignore possible numerics from clients.
	if (command.match(/^[0-9]+/))
		return 0;

	this.idletime = time();

	switch (command) {
		// RFC1459 states that we must reply to a PING as fast as
		// possible, which is why this is on top.
		case "PING":
			if (!cmd[1]) {
				this.numeric(409,":No origin specified.");
				break;
			}
			if (cmd[2]) {
				if (cmd[2][0] == ":")
					cmd[2] = cmd[2].slice(1);
				var dest_server = searchbyserver(cmd[2]);
				if (!dest_server) {
					this.numeric402(cmd[2]);
					break;
				}
				if (dest_server != -1) {
					dest_server.rawout(":" + this.nick + " PING " + this.nick + " :" + cmd[2]);
					break;
				}
			}
			this.ircout("PONG " + servername + " :" + cmd[1]);
			break;
		case "ADMIN":
			if (cmd[1]) {
				if (cmd[1][0] == ":")
					cmd[1] = cmd[1].slice(1);
				var dest_server = searchbyserver(cmd[1]);
				if (!dest_server) {
					this.numeric402(cmd[1]);
					break;
				}
				if (dest_server != -1) {
					dest_server.rawout(":" + this.nick + " ADMIN :" + dest_server.nick);
					break;
				}
			}
			this.do_admin();
			break;
		case "AWAY":
			if (cmd[1] && (cmd[1] != ":") ) {
				this.away=IRC_string(cmdline).slice(0,80);
				this.numeric(306, ":You have been marked as being away.");
				this.bcast_to_servers("AWAY :" + this.away);
			} else {
				this.away="";
				this.numeric(305, ":You are no longer marked as being away.");
				this.bcast_to_servers("AWAY");
			}
			break;
		case "CHATOPS":
			if (!((this.mode&USERMODE_OPER) &&
			      (this.flags&OLINE_CAN_CHATOPS))) {
				this.numeric481();
				break;
			}
			umode_notice(USERMODE_CHATOPS,"ChatOps","from " +
				this.nick + ": " + IRC_string(cmdline));
			server_bcast_to_servers(":" + this.nick + " CHATOPS :"+
				IRC_string(cmdline));
			break;
		case "CONNECT":
			if (!((this.mode&USERMODE_OPER) &&
			      (this.flags&OLINE_CAN_LSQUITCON) )) {
				this.numeric481();
				break;
			}
			if (!cmd[1]) {
				this.numeric461("CONNECT");
				break;
			}
			if (cmd[3]) {
				var dest_server = searchbyserver(cmd[3]);
				if (!dest_server) {
					this.numeric402(cmd[3]);
					break;
				} else if (dest_server != -1) {
					if (!(this.flags&OLINE_CAN_GSQUITCON)) {
						this.numeric481();
						break;
					}
					dest_server.rawout(":" + this.nick + " CONNECT " + cmd[1] + " " + cmd[2] + " " + dest_server.nick);
					break;
				}
			}
			this.do_connect(cmd[1],cmd[2]);
			break;
		case "DEBUG":
			if (!((this.mode&USERMODE_OPER) &&
			      (this.flags&OLINE_CAN_DEBUG))) {
				this.numeric481();
				break;
			}
			if (!cmd[1]) {
				this.server_notice("Usage:");
				this.server_notice("  DEBUG D       - Toggle DEBUG mode on/off");
				this.server_notice("  DEBUG E <exp> - Evaluate a Javascript expression.");
				this.server_notice("  DEBUG Y <val> - Set yield frequency to <val>");
				this.server_notice("  DEBUG U       - Dump all users stored in mem");
				this.server_notice("  DEBUG C       - Dump all channels stored in mem");
				break;
			}
			switch (cmd[1][0].toUpperCase()) {
				case "C":
					for (mychan in Channels) {
						this.server_notice(Channels[mychan].nam+","+Channels[mychan].mode+","+Channels[mychan].users);
					}
					break;
				case "D":
					if (debug) {
						debug=false;
						umode_notice(USERMODE_OPER,"Notice","Debug mode disabled by " + this.nuh);
					} else {
						debug=true;
						umode_notice(USERMODE_OPER,"Notice","Debug mode enabled by " + this.nuh);
					}
					break;
				case "E":
					cmd.shift();
					cmd.shift();
					var exp = cmd.join(" ");
					umode_notice(USERMODE_DEBUG,"Debug",
						"Oper " + this.nick +
						" is using EVAL: " + exp);
					try {
						this.server_notice("Result: " +
							eval(exp));
					} catch(e) {
						this.server_notice("!" + e);
					}
					break;
				case "U":
					for (myuser in Users) {
						var usr = Users[myuser];
						this.server_notice(usr.nick+","+usr.local+","+usr.parent+","+usr.id);
					}
					break;
				case "Y":
					if (cmd[2]) {
						umode_notice(USERMODE_DEBUG,"Debug","branch.yield_freq set to " + cmd[2] + " by " + this.nuh);
						branch.yield_freq = parseInt(cmd[2]);
					}
					break;
				default:
					this.server_notice("Unknown DEBUG flag.");
			}
			break;
		case "DIE":
			if (!((this.mode&USERMODE_OPER) &&
			      (this.flags&OLINE_CAN_DIE))) {
				this.numeric481();
				break;
			}
			if (diepass && !cmd[1]) {
				this.numeric461("DIE");
				break;
			} else if (diepass && (cmd[1] != diepass)) {
				this.server_notice("Invalid DIE password.");
				break;
			}
			log("!ERROR! Shutting down the ircd as per " + this.nuh);
			js.terminated = true;
			break;
		case "ERROR":
			break; // silently ignore
		case "GLOBOPS":
			if (!((this.mode&USERMODE_OPER) &&
			      (this.flags&OLINE_CAN_GLOBOPS))) {
				this.numeric481();
				break;
			}
			if (!cmd[1]) {
				this.numeric461("GLOBOPS");
				break;
			}
			this.globops(IRC_string(cmdline));
			break;
		case "INFO":
			if (cmd[1]) {
				if (cmd[1][0] == ":")
					cmd[1] = cmd[1].slice(1);
				var dest_server = searchbyserver(cmd[1]);
				if (!dest_server) {
					this.numeric402(cmd[1]);
					break;
				}
				if (dest_server != -1) {
					dest_server.rawout(":" + this.nick + " INFO :" + dest_server.nick);
					break;
				}
			}
			this.do_info();
			break;
		case "INVITE":
			if (!cmd[2]) {
				this.numeric461("INVITE");
				break;
			}
			var chanid = Channels[cmd[2].toUpperCase()];
			if (!chanid) {
				this.numeric403(cmd[2]);
				break;
			}
			if (!chanid.modelist[CHANMODE_OP][this.id]) {
				this.numeric482(chanid.nam);
				break;
			}
			var nickid = Users[cmd[1].toUpperCase()];
			if (!nickid) {
				this.numeric401(cmd[1]);
				break;
			}
			if (nickid.channels[cmd[2].toUpperCase()]) {
				this.numeric(443, nickid.nick + " " + chanid.nam + " :is already on channel.");
				break;
			}
			this.numeric("341", nickid.nick + " " + chanid.nam);
			nickid.originatorout("INVITE " + nickid.nick + " :" + chanid.nam,this);
			nickid.invited=chanid.nam.toUpperCase();
			break;
		case "ISON":
			if (!cmd[1])
				break; // drop silently
			if (cmd[1][0] == ":")
				cmd[1] = cmd[1].slice(1);
			var isonstr = ":";
			var ison_nick_id;
			cmd.shift(); // get rid of command
			for(ison in cmd) {
				ison_nick_id = Users[cmd[ison].toUpperCase()];
				if (ison_nick_id) {
					if (isonstr != ":")
						isonstr += " ";
					isonstr += ison_nick_id.nick;
				}
			}
			this.numeric("303", isonstr);
			break;
		case "JOIN":
			if (!cmd[1]) {
				this.numeric461("JOIN");
				break;
			}
			if (cmd[1][0] == ":")
				cmd[1]=cmd[1].slice(1);
			var the_channels = cmd[1].split(",");
			var the_keys = "";
			if (cmd[2])
				the_keys = cmd[2].split(",");
			var key_counter = 0;
			for(jchan in the_channels) {
				var regexp = "^[0]{1,}$"; //0 is a special case
				if(the_channels[jchan].match(regexp)) {
					this.part_all();
				} else {
					if (Channels[the_channels[jchan].toUpperCase()] && the_keys[key_counter] && (Channels[the_channels[jchan].toUpperCase()].mode&CHANMODE_KEY)) {
						this.do_join(the_channels[jchan].slice(0,max_chanlen),the_keys[key_counter])
						key_counter++;
					} else {
						this.do_join(the_channels[jchan].slice(0,max_chanlen),"");
					}
				}
			}
			break;
		case "KICK":
			if (!cmd[2]) {
				this.numeric461("KICK");
				break;
			}
			var chanid = Channels[cmd[1].toUpperCase()];
			if (!chanid) {
				this.numeric403(cmd[1]);
				break;
			}
			if (!chanid.modelist[CHANMODE_OP][this.id]) {
				this.numeric482(chanid.nam);
				break;
			}
			var nickid = Users[cmd[2].toUpperCase()];
			if (!nickid) {
				nickid = search_nickbuf(cmd[2]);
				if (!nickid) {
					this.numeric401(cmd[2]);
					break;
				}
			}
			if (!nickid.channels[chanid.nam.toUpperCase()]) {
				this.numeric(441, nickid.nick + " " + chanid.nam + " :They aren't on that channel!");
				break;
			}
			var kick_reason;
			if (cmd[3])
				kick_reason = IRC_string(cmdline).slice(0,max_kicklen);
			else
				kick_reason = this.nick;
			var str = "KICK " + chanid.nam + " " + nickid.nick + " :" + kick_reason;
			this.bcast_to_channel(chanid, str, true);
			this.bcast_to_servers(str);
			nickid.rmchan(chanid);
			break;
		case "KILL":
			if (!(this.mode&USERMODE_OPER) ||
			    !(this.flags&OLINE_CAN_LKILL)) {
				this.numeric481();
				break;
			}
			if (!cmd[2]) {
				this.numeric461("KILL");
				break;
			}
			if (cmd[1].match(/[.]+/)) {
				this.numeric(483, ":You can't kill a server!");
				break;
			}
			if (cmd[2] == ":") {
				this.numeric(461, "KILL :You MUST specify a reason for /KILL.");
				break;
			}
			var reason = IRC_string(cmdline);
			var kills = cmd[1].split(",");
			var target;
			for(kill in kills) {
				target = Users[kills[kill].toUpperCase()];
				if (!target)
					target = search_nickbuf(kills[kill]);
				if (!target) {
					this.numeric401(kills[kill]);
					continue;
				}
				if (target.local) {
					target.quit("Local kill by " + this.nick + " (" + reason + ")");
				} else {
					if (!(this.flags&OLINE_CAN_GKILL)) {
						this.numeric481();
						continue;
					}
					var trg_srv = searchbyserver(target.servername);
					if (trg_srv && trg_srv.uline) {
						this.numeric(483, ":You may not KILL clients on a U:Lined server.");
						continue;
					}
					server_bcast_to_servers(":" + this.nick + " KILL " + target.nick + " :" + reason);
					target.quit("Killed (" + this.nick + " (" + reason + "))",true);
				}
			}
			break;
		case "KLINE":
			if (!((this.mode&USERMODE_OPER) &&
			      (this.flags&OLINE_CAN_KLINE))) {
				this.numeric481();
				break;
			}
			if (!cmd[2]) {
				this.numeric461("KLINE");
				break;
			}
			var kline_mask = create_ban_mask(cmd[1],true);
			if (!kline_mask) {
				this.server_notice("Invalid K:Line mask.");
				break;
			}
			if (isklined(kline_mask)) {
				this.server_notice("K:Line already exists!");
				break;
			}
			KLines.push(new KLine(kline_mask,IRC_string(cmdline),"k"));
			umode_notice(USERMODE_OPER,"Notice", this.nick +
				" added temporary 99 min. k-line for [" +
				kline_mask + "] [0]");
			scan_for_klined_clients();
			break;
		case "UNKLINE":
			if (!((this.mode&USERMODE_OPER) &&
			      (this.flags&OLINE_CAN_UNKLINE))) {
				this.numeric481();
				break;
			}
			if (!cmd[1]) {
				this.numeric461("UNKLINE");
				break;
			}
			var kline_mask = create_ban_mask(cmd[1],true);
			if (!kline_mask) {
				this.server_notice("Invalid K:Line mask.");
				break;
			}
			if (!isklined(kline_mask)) {
				this.server_notice("No such K:Line.");
				break;
			}
			remove_kline(kline_mask);
			umode_notice(USERMODE_OPER,"Notice", this.nick +
				" has removed the K-Line for: [" +
				kline_mask + "] (1 matches)");
			break;
		case "LINKS":
			if (!cmd[1]) { // *
				this.do_links();
				break;
			} else if (cmd[2]) { // <remote-server> <mask>
				var dest_server = searchbyserver(cmd[1]);
				if (!dest_server) {
					this.numeric402(cmd[1]);
					break;
				}
				if (dest_server != -1) {
					dest_server.rawout(":" + this.nick + " LINKS " + dest_server.nick + " " + cmd[2]);
					break;
				}
			} else if (cmd[1]) { // <mask>
				this.do_links(cmd[1]);
				break;
			}
			break;
		case "LIST":
			if (!cmd[1]) {
				this.do_basic_list("*");
				break;
			}
			if (cmd[1] == "?") {
				this.do_list_usage();
				break;
			}
			if (!cmd[2] && (cmd[1][0]!="+") && (cmd[1][0]!="-")) {
				this.do_basic_list(cmd[1]);
				break;
			}
			this.do_complex_list(cmd);
			break;
		case "LOCOPS":
			if (!((this.mode&USERMODE_OPER) &&
			      (this.flags&OLINE_CAN_LOCOPS))) {
				this.numeric481();
				break;
			}
			umode_notice(USERMODE_OPER,"LocOps","from " +
				this.nick + ": " + IRC_string(cmdline));
			break;
		case "LUSERS":
			this.lusers();
			break;
		case "MODE":
			var chan;

			if (!cmd[1])
				break; // silently ignore
			if (!cmd[2]) {
				// information only
				if ((cmd[1][0] == "#") || (cmd[1][0] == "&")) {
					var chan = Channels[cmd[1].toUpperCase()];
					if (chan) {
						var fullmodes = false;
						if (this.channels[chan.nam.toUpperCase()])
							fullmodes = true;
						this.numeric(324, chan.nam + " " + chan.chanmode(fullmodes));
						this.numeric(329, chan.nam + " " + chan.created);
						break;
					} else {
						this.numeric401(cmd[1]);
						break;
					}
				} else {
					// getting my umode
					if (cmd[1].toUpperCase() ==
					    this.nick.toUpperCase())
						this.numeric(221, this.get_usermode());
					else if (Users[cmd[1].toUpperCase()])
						this.numeric(502, ":Can't view mode for other users.");
					else
						this.numeric401(cmd[1]);
					break;
				}
			} else {
				if ((cmd[1][0] == "#") || (cmd[1][0] == "&")) {
					var chan = Channels[cmd[1].toUpperCase()];
					if (!chan) {
						this.numeric403(cmd[1]);
						break;
					}
					cmd.shift();
					cmd.shift();
					var modeline = cmd.join(" ");
					this.set_chanmode(chan,modeline,false);
				} else if (cmd[1].toUpperCase() == this.nick.toUpperCase()) {
					this.setusermode(cmd[2]);
				} else {
					this.numeric(502, ":Can't change mode for other users.");
				}
			}
			break;
		case "MOTD":
			if (cmd[1]) {
				if (cmd[1][0] == ":")
					cmd[1] = cmd[1].slice(1);
				var dest_server = searchbyserver(cmd[1]);
				if (!dest_server) {
					this.numeric402(cmd[1]);
					break;
				}
				if (dest_server != -1) {
					dest_server.rawout(":" + this.nick + " MOTD :" + dest_server.nick);
					break;
				}
			}
			umode_notice(USERMODE_SPY,"Spy","MOTD requested by " +
				this.nick + " (" + this.uprefix + "@" +
				this.hostname + ") [" + this.servername + "]");
			this.motd();
			break;
		case "NAMES":
			var numnicks;
			var tmp;
			var chan;
			var Client;

			if (!cmd[1]) {
				for(tc in Channels) {
					chan = Channels[tc];
					if (!(tc.mode&CHANMODE_SECRET) &&
					    !(tc.mode&CHANMODE_PRIVATE))
						this.names(chan);
				}
				numnicks = 0;
				tmp = "";
				for (thisClient in Users) {
					Client = Users[thisClient];
					if (!true_array_len(Client.channels) &&
					    !(Client.mode&USERMODE_INVISIBLE)) {
						if (numnicks)
							tmp += " ";
						tmp += Client.nick;
						numnicks++;
						if (numnicks >= 59) {
							this.numeric(353,"* * :"+tmp);
							numnicks = 0;
							tmp = "";
						}
					}
				}
				if (numnicks)
					this.numeric(353,"* * :"+tmp);
				this.numeric(366, "* :End of /NAMES list.");
			} else {
				chans = cmd[1].split(',');
				for (nc in chans) {
					if ((chans[nc][0] == "#") ||
					    (chans[nc][0] == "&")) {
						chan = Channels[chans[nc].toUpperCase()];
						if (chan) {
							this.names(chan);
						} else {
							this.numeric401(chans[nc]);
							break;
						}
					} else {
						this.numeric403(chans[nchan]);
						break;
					}
				}
				this.numeric(366, chan.nam + " :End of /NAMES list.");
			}
			break;
		case "NICK":
			var the_nick;

			if (!cmd[1]) {
				this.numeric(431, ":No nickname given.");
				break;
			}
			the_nick = IRC_string(cmd[1]).slice(0,max_nicklen);
			if(this.check_nickname(the_nick) > 0) {
				var str="NICK " + the_nick;
				this.bcast_to_uchans_unique(str);
				this.originatorout(str,this);
				this.created = time();
				this.bcast_to_servers(str + " :" + this.created);
				push_nickbuf(this.nick,the_nick);
				// move our Users entry over.
				Users[the_nick.toUpperCase()] = this;
				delete Users[this.nick.toUpperCase()];
				// finalize
				this.nick = the_nick;
			}
			break;
		case "NOTICE":
			if (!cmd[1]) {
				this.numeric411("NOTICE");
				break;
			}
			var my_ircstr = IRC_string(cmdline);
			if ( !cmd[2] || ( !cmd[3] && (
			     (cmd[2] == ":") && (my_ircstr == "")
			   ) ) ) {
				this.numeric412();
				break;
			}
			var targets = cmd[1].split(',');
			for (nt in targets) {
				this.do_msg(targets[nt],"NOTICE",my_ircstr);
			}
			break;
		case "OPER":
			if (!cmd[2]) {
				this.numeric461(command);
				break;
			}
			if (this.mode&USERMODE_OPER) {
				this.server_notice("You're already an IRC operator.");
				break;
			}
			var oper_success = false;
			for (ol in OLines) {
				if(((cmd[1].toUpperCase() ==
				    OLines[ol].nick.toUpperCase()) &&
				   (IRC_match(this.uprefix + "@" +
				    this.hostname,OLines[ol].hostmask)) &&
				   (cmd[2] == OLines[ol].password) &&
				   !(OLines[ol].flags&OLINE_CHECK_SYSPASSWD))
				||
				   ((OLines[ol].flags&OLINE_CHECK_SYSPASSWD) &&
				    system.check_syspass(cmd[2]) )
				) {
					oper_success=true;
					this.ircclass = OLines[ol].ircclass;
					this.flags = OLines[ol].flags;
					break;
				}
			}
			if (!oper_success) {
				this.numeric(491, ":No O:Lines for your host.  Attempt logged.");
				umode_notice(USERMODE_OPER,"Notice","Failed OPER attempt by " + this.nick + " (" + this.uprefix + "@" + this.hostname + ")");
				break;
			}
			// otherwise we succeeded.
			this.numeric(381, ":You are now an IRC operator.");
			this.mode|=USERMODE_OPER;
			this.rawout(":" + this.nick + " MODE " + this.nick + " +o");
			umode_notice(USERMODE_SERVER,"Notice",
				this.nick + " (" + this.uprefix +
				"@" + this.hostname + ") " +
				"is now operator (O)");
			if (OLines[ol].flags&OLINE_IS_GOPER)
				this.bcast_to_servers("MODE "+ this.nick +" +o");
			break;
		case "PART":
			var the_channels;

			if (!cmd[1]) {
				this.numeric461("PART");
				break;
			}
			the_channels = cmd[1].split(",");
			for(pchan in the_channels) {
				this.do_part(the_channels[pchan]);
			}
			break;
		case "PASS":
		case "USER":
			this.numeric462();
			break;
		case "PONG":
			if (cmd[2]) {
				var dest_server = searchbyserver(cmd[2]);
				if (!dest_server) {
					this.numeric402(cmd[2]);
					break;
				}
				if (dest_server != -1) {
					dest_server.rawout(":" + this.nick + " PONG " + cmd[1] + " " + dest_server.nick);
					break;
				}
			}
			this.pinged = false;
			break;
		case "PRIVMSG":
			if (!cmd[1]) {
				this.numeric411("PRIVMSG");
				break;
			}
			var my_ircstr = IRC_string(cmdline);
			if ( !cmd[2] || ( !cmd[3] && (
			     (cmd[2] == ":") && (my_ircstr == "")
			   ) ) ) {
				this.numeric412();
				break;
			}
			var targets = cmd[1].split(',');
			for (pm in targets) {
				this.do_msg(targets[pm],"PRIVMSG",my_ircstr);
			}
			this.talkidle = time();
			break;
		case "QUIT":
			this.quit(IRC_string(cmdline));
			break;
		case "REHASH":
			if (!((this.mode&USERMODE_OPER) &&
			      (this.flags&OLINE_CAN_REHASH))) {
				this.numeric481();
				break;
			}
			if (cmd[1]) {
				switch(cmd[1].toUpperCase()) {
					case "TKLINES":
						this.numeric382("temp klines");
						umode_notice(USERMODE_SERVER,"Notice",this.nick + " is clearing temp klines while whistling innocently");
						for (kl in KLines) {
							if(KLines[kl].type ==
							   "k")
								delete KLines[kl];
						}
						break;
					case "GC":
						if (js.gc!=undefined) {
							this.numeric382("garbage collecting");
							umode_notice(USERMODE_SERVER,"Notice",this.nick + " is garbage collecting while whistling innocently");
							js.gc();
						}
						break;
					case "AKILLS":
						this.numeric382("akills");
						umode_notice(USERMODE_SERVER,"Notice",this.nick + " is rehashing akills");
						for (kl in KLines) {
							if(KLines[kl].type ==
							   "A")
								delete KLines[kl];                                              
						}
						break;
					default:
						break;
				}
			} else {
				this.numeric382("Rehashing.");
				umode_notice(USERMODE_SERVER,"Notice",this.nick +
					" is rehashing Server config file while " +
					"whistling innocently");
				read_config_file();
			}
			break;
		case "RESTART":
			if (!((this.mode&USERMODE_OPER) &&
			      (this.flags&OLINE_CAN_RESTART))) {
				this.numeric481();
				break;
			}
			if (restartpass && !cmd[1]) {
				this.numeric461("RESTART");
				break;
			} else if (restartpass && (cmd[1] != restartpass)) {
				this.server_notice("Invalid RESTART password.");
				break;
			}
			var rs_str = "Aieeeee!!!  Restarting server...";
			umode_notice(USERMODE_SERVER,"Notice",rs_str);
			terminate_everything(rs_str);
			break;
		case "SQUIT":
			if (!((this.mode&USERMODE_OPER) &&
			      (this.flags&OLINE_CAN_LSQUITCON))) {
				this.numeric481();
				break;
			}
			if(!cmd[1])
				break;
			var sq_server = searchbyserver(cmd[1]);
			if(!sq_server) {
				this.numeric402(cmd[1]);
				break;
			}
			var reason = IRC_string(cmdline);
			if (!reason)
				reason = this.nick;
			if (sq_server == -1) {
				this.quit(reason);
				break;
			}
			if (!sq_server.local) {
				if (!(this.flags&OLINE_CAN_GSQUITCON)) {
					this.numeric481();
					break;
				}
				sq_server.rawout(":" + this.nick + " SQUIT " + sq_server.nick + " :" + reason);
				break;
			}
			umode_notice(USERMODE_ROUTING,"Routing","from " +
				servername + ": Received SQUIT " + cmd[1] +
				" from " + this.nick + "[" + this.uprefix +
				"@" + this.hostname + "] (" + reason + ")");
			sq_server.quit(reason);
			break;
		case "STATS":
			if(!cmd[1])
				break;
			if (cmd[2]) {
				if (cmd[2][0] == ":")
					cmd[2] = cmd[2].slice(1);
				var dest_server = searchbyserver(cmd[2]);
				if (!dest_server) {
					this.numeric402(cmd[2]);
					break;
				}
				if (dest_server != -1) {
					dest_server.rawout(":" + this.nick + " STATS " + cmd[1][0] + " :" + dest_server.nick);
					break;
				}
			}
			this.do_stats(cmd[1][0]);
			break;
		case "SUMMON":
			if(!cmd[1]) {
				this.numeric411("SUMMON");
				break;
			}
			if (cmd[2]) {
				if (cmd[2][0] == ":")
					cmd[2] = cmd[2].slice(1);
				var dest_server = searchbyserver(cmd[1]);
				if (!dest_server) {
					this.numeric402(cmd[2]);
					break;
				}
				if (dest_server != -1) {
					dest_server.rawout(":" + this.nick + " SUMMON " + cmd[1] + " :" + dest_server.nick);
					break;
				}
			}
			if(!enable_users_summon) {
				this.numeric445();
				break;
			}
			this.do_summon(cmd[1]);
			break;
		case "TIME":
			if (cmd[1]) {
				if (cmd[1][0] == ":")
					cmd[1] = cmd[1].slice(1);
				var dest_server = searchbyserver(cmd[1]);
				if (!dest_server) {
					this.numeric402(cmd[1]);
					break;
				}
				if (dest_server != -1) {
					dest_server.rawout(":" + this.nick + " TIME :" + dest_server.nick);
					break;
				}
			}
			this.numeric391();
			break;
		case "TOPIC":
			if (!cmd[1]) {
				this.numeric461("TOPIC");
				break;
			}
			var chanid = Channels[cmd[1].toUpperCase()];
			if (!chanid) {
				this.numeric403(cmd[1]);
				break;
			}
			if (!this.channels[chanid.nam.toUpperCase()]) {
				this.numeric442(chanid.nam);
				break;
			}
			if (cmd[2]) {
				if (!(chanid.mode&CHANMODE_TOPIC) ||
				     chanid.modelist[CHANMODE_OP][this.id]) {
					var tmp_topic = IRC_string(cmdline).slice(0,max_topiclen);
					if (tmp_topic == chanid.topic)
						break;
					chanid.topic = tmp_topic;
					chanid.topictime = time();
					chanid.topicchangedby = this.nick;
					var str = "TOPIC " + chanid.nam + " :" + chanid.topic;
					this.bcast_to_channel(chanid, str, true);
					this.bcast_to_servers("TOPIC " + chanid.nam + " " + this.nick + " " + chanid.topictime + " :" + chanid.topic);
				} else {
					this.numeric482(chanid.nam);
				}
			} else { // we're just looking at one
				if (chanid.topic) {
					this.numeric332(chanid);
					this.numeric333(chanid);
				} else {
					this.numeric331(chanid);
				}
			}
			break;
		case "TRACE":
			if (cmd[1]) {
				this.do_trace(cmd[1]);
			} else { // no args? pass our servername as the target
				this.do_trace(servername);
			}
			break;
		case "USERS":
			if (cmd[1]) {
				if (cmd[1][0] == ":")
					cmd[1] = cmd[1].slice(1);
				var dest_server = searchbyserver(cmd[1]);
				if (!dest_server) {
					this.numeric402(cmd[1]);
					break;
				}
				if (dest_server != -1) {
					dest_server.rawout(":" + this.nick + " USERS :" + dest_server.nick);
					break;
				}
			}
			if (!enable_users_summon) {
				this.numeric446();
				break;
			}
			this.do_users();
			break;
		case "USERHOST":
			var uhnick;
			var uh;
			var uhstr = "";
			var uh_argcount;

			if (!cmd[1]) {
				this.numeric461("USERHOST");
				break;
			}

			if (cmd.length > 6)
				uh_argcount = 6;
			else
				uh_argcount = cmd.length;

			for (uh=1 ; uh < uh_argcount ; uh++) {
				uhnick = Users[cmd[uh].toUpperCase()];
				if (uhnick) {
					if (uhstr)
						uhstr += " ";
					uhstr += uhnick.nick;
					if (uhnick.mode&USERMODE_OPER)
						uhstr += "*";
					uhstr += "=";
					if (uhnick.away)
						uhstr += "-";
					else
						uhstr += "+";
					uhstr += uhnick.uprefix;
					uhstr += "@";
					uhstr += uhnick.hostname;
				}
			}
			this.numeric(302, ":" + uhstr);
			break;
		case "VERSION":
			if (cmd[1]) {
				if (cmd[1][0] == ":")
					cmd[1] = cmd[1].slice(1);
				var dest_server = searchbyserver(cmd[1]);
				if (!dest_server) {
					this.numeric402(cmd[1]);
					break;
				}
				if (dest_server != -1) {
					dest_server.rawout(":" + this.nick + " VERSION :" + dest_server.nick);
					break;
				}
			}
			this.numeric351();
			break;
		case "WALLOPS":
			if (!((this.mode&USERMODE_OPER) &&
			      (this.flags&OLINE_CAN_WALLOPS))) {
				this.numeric481();
				break;
			}
			if (!cmd[1]) {
				this.numeric461("WALLOPS");
				break;
			}
			wallopers(":" + this.nuh + " WALLOPS :" + IRC_string(cmdline));
			server_bcast_to_servers(":" + this.nick + " WALLOPS :" + IRC_string(cmdline));
			break;
		case "WHO":
			if (!cmd[1]) {
				this.do_who_usage();
				break;
			}
			if (cmd[1] == "?") {
				this.do_who_usage();
				break;
			}
			if (cmd[2] || (cmd[1][0] == "-")||(cmd[1][0] == "+"))
				this.do_complex_who(cmd);
			else
				this.do_basic_who(cmd[1]);
			break;
		case "WHOIS":
			if (!cmd[1]) {
				this.numeric(431, ":No nickname given.");
				break;
			}
			if (cmd[2]) {
				var dest_server = searchbyserver(cmd[2]);
				if (!dest_server) {
					this.numeric402(cmd[2]);
					break;
				}
				if (dest_server != -1) {
					dest_server.rawout(":" + this.nick + " WHOIS " + cmd[1] + " " + dest_server.nick);
					break;
				}
			}
			var wi_nicks = cmd[1].split(",");
			for (wi_nick in wi_nicks) {
				var wi = Users[wi_nicks[wi_nick].toUpperCase()];
				if (wi)
					this.do_whois(wi);
				else
					this.numeric401(wi_nicks[wi_nick]);
			}
			this.numeric(318, wi_nicks[0]+" :End of /WHOIS list.");
			break;
		case "WHOWAS":
			if (!cmd[1]) {
				this.numeric(431, ":No nickname given.");
				break;
			}
			var firstnick="";
			var aWhoWas;
			for (aWhoWas=whowas_pointer;aWhoWas>=0;aWhoWas--) {
				var wwh = WhoWasHistory[aWhoWas];
				if (wwh && (wwh.nick.toUpperCase() ==
				    cmd[1].toUpperCase())) {
					this.numeric(314,wwh.nick + " " + wwh.uprefix + " " + wwh.host + " * :" + wwh.realname);
					this.numeric(312,wwh.nick + " " + wwh.server + " :" + wwh.serverdesc);
					if (!firstnick)
						firstnick = wwh.nick;
				}
			}
			for (aWhoWas=whowas_buffer;aWhoWas>=whowas_pointer;aWhoWas--) {
				var wwh = WhoWasHistory[aWhoWas];
				if (wwh && (wwh.nick.toUpperCase() ==
				    cmd[1].toUpperCase())) {
					this.numeric(314,wwh.nick + " " + wwh.uprefix + " " + wwh.host + " * :" + wwh.realname);
					this.numeric(312,wwh.nick + " " + wwh.server + " :" + wwh.serverdesc);
					if (!firstnick)
						firstnick = wwh.nick;
				}
			}
			if (!firstnick) {
				this.numeric(406,cmd[1] + " :There was no such nickname.");
				firstnick = cmd[1];
			}
			this.numeric(369,firstnick+" :End of /WHOWAS command.");
			break;
		// Services Helper Commands...
		case "CS":
		case "CHANSERV":
			if (!cmd[1]) {
				this.numeric412();
				break;
			}
			var str = cmdline.slice(cmdline.indexOf(" ")+1);
			if (str[0] == ":")
				str = str.slice(1);
			this.services_msg("ChanServ",str);
			break;
		case "NS":
		case "NICKSERV":
			if (!cmd[1]) {
				this.numeric412();
				break;
			}
			var str = cmdline.slice(cmdline.indexOf(" ")+1);
			if (str[0] == ":")
				str = str.slice(1);
			this.services_msg("NickServ",str);
			break;
		case "MS":
		case "MEMOSERV":
			if (!cmd[1]) {
				this.numeric412();
				break;
			}
			var str = cmdline.slice(cmdline.indexOf(" ")+1);
			if (str[0] == ":")
				str = str.slice(1);
			this.services_msg("MemoServ",str);
			break;
		case "OS":
		case "OPERSERV":
			if (!cmd[1]) {
				this.numeric412();
				break;
			}
			var str = cmdline.slice(cmdline.indexOf(" ")+1);
			if (str[0] == ":")
				str = str.slice(1);
			this.services_msg("OperServ",str);
			break;
		case "HELP":
		case "HS":
		case "HELPSERV":
			var str;
			if (!cmd[1]) {
				str = "HELP";
			} else {
				str = cmdline.slice(cmdline.indexOf(" ")+1);
				if (str[0] == ":")
					str = str.slice(1);
			}
			this.services_msg("HelpServ",str);
			break;
		case "IDENTIFY":
			if (!cmd[1]) {
				this.numeric412();
				break;
			}
			var str = cmdline.slice(cmdline.indexOf(" ")+1);
			if (str[0] == ":")
				str = str.slice(1);
			var services_target;
			if (cmd[1][0] == "#")
				services_target = "ChanServ";
			else
				services_target = "NickServ";
			this.services_msg(services_target,"IDENTIFY " + str);
			break;
		default:
			this.numeric("421", command + " :Unknown command.");
			break;
	}
}

function User_Quit(str,suppress_bcast,is_netsplit,origin) {
	if (!str)
		str = this.nick;

	var tmp = "QUIT :" + str;
	this.bcast_to_uchans_unique(tmp);
	for(thisChannel in this.channels) {
		this.rmchan(this.channels[thisChannel]);
	}
	if (whowas_pointer >= whowas_buffer)
		whowas_pointer = 0;
	if (!this.parent)
		ww_serverdesc = serverdesc;
	else
		ww_serverdesc = Servers[this.parent.toLowerCase()].info;
	WhoWasHistory[whowas_pointer] = new WhoWas(this.nick,this.uprefix,this.hostname,this.realname,this.servername,ww_serverdesc);
	whowas_pointer++;
	if (!suppress_bcast)
		this.bcast_to_servers(tmp);

	if (this.local) {
		if(server.client_remove!=undefined)
			server.client_remove(this.socket);
		this.rawout("ERROR :Closing Link: [" + this.uprefix + "@" + this.hostname + "] (" + str + ")");
		umode_notice(USERMODE_CLIENT,"Client","Client exiting: " +
			this.nick + " (" + this.uprefix + "@" + this.hostname +
			") [" + str + "] [" + this.ip + "]");
		if (this.socket!=undefined)
			this.socket.close();
	}

	delete Local_Sockets[this.id];
	delete Local_Sockets_Map[this.id];
	delete Local_Users[this.id];
	delete Users[this.nick.toUpperCase()];
	delete this;
	rebuild_socksel_array = true;
}

