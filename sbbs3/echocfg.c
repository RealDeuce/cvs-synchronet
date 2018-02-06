/* SBBSecho configuration utility 											*/

/* $Id: echocfg.c,v 3.27 2018/01/12 23:06:09 rswindell Exp $ */

/****************************************************************************
 * @format.tab-size 4		(Plain Text/Source Code File Header)			*
 * @format.use-tabs true	(see http://www.synchro.net/ptsc_hdr.html)		*
 *																			*
 * Copyright Rob Swindell - http://www.synchro.net/copyright.html			*
 *																			*
 * This program is free software; you can redistribute it and/or			*
 * modify it under the terms of the GNU General Public License				*
 * as published by the Free Software Foundation; either version 2			*
 * of the License, or (at your option) any later version.					*
 * See the GNU General Public License for more details: gpl.txt or			*
 * http://www.fsf.org/copyleft/gpl.html										*
 *																			*
 * Anonymous FTP access to the most recent released source is available at	*
 * ftp://vert.synchro.net, ftp://cvs.synchro.net and ftp://ftp.synchro.net	*
 *																			*
 * Anonymous CVS access to the development source and modification history	*
 * is available at cvs.synchro.net:/cvsroot/sbbs, example:					*
 * cvs -d :pserver:anonymous@cvs.synchro.net:/cvsroot/sbbs login			*
 *     (just hit return, no password is necessary)							*
 * cvs -d :pserver:anonymous@cvs.synchro.net:/cvsroot/sbbs checkout src		*
 *																			*
 * For Synchronet coding style and modification guidelines, see				*
 * http://www.synchro.net/source.html										*
 *																			*
 * You are encouraged to submit any modifications (preferably in Unix diff	*
 * format) via e-mail to mods@synchro.net									*
 *																			*
 * Note: If this box doesn't appear square, then you need to fix your tabs.	*
 ****************************************************************************/

/* Portions written by Allen Christiansen 1994-1996 						*/

#include <stdio.h>

#undef JAVASCRIPT

/* XPDEV Headers */
#include "gen_defs.h"

#define __COLORS
#include "ciolib.h"
#include "uifc.h"
#include "sbbs.h"
#include "sbbsecho.h"

char **opt;

sbbsecho_cfg_t cfg;
uifcapi_t uifc;

void bail(int code)
{

	if(uifc.bail!=NULL)
		uifc.bail();
	exit(code);
}

/* These correlate with the LOG_* definitions in syslog.h/gen_defs.h */
static char* logLevelStringList[] 
	= {"Emergency", "Alert", "Critical", "Error", "Warning", "Notice", "Informational", "Debugging", NULL};

void global_settings(void)
{
	static int global_opt;

	while(1) {
		int i = 0;
		char str[128];
		char duration[64];
		sprintf(opt[i++], "%-25s %s", "Mailer Type"
			,cfg.flo_mailer ? "Binkley/FLO":"ArcMail/Attach");
		sprintf(opt[i++], "%-25s %s", "Log Level", logLevelStringList[cfg.log_level]);
		sprintf(opt[i++], "%-25s %s", "Log Timestamp Format", cfg.logtime);
		sprintf(opt[i++], "%-25s %s", "Strict Packet Passwords", cfg.strict_packet_passwords ? "Enabled" : "Disabled");
		sprintf(opt[i++], "%-25s %s", "Use FTN Domain/Zone Map", cfg.use_ftn_domains ? "Enabled" : "Disabled");
		sprintf(opt[i++], "%-25s %s", "BSY Mutex File Timeout", duration_to_vstr(cfg.bsy_timeout, duration, sizeof(duration)));
		sprintf(opt[i++], "%-25s %s", "BSO Lock Attempt Delay", duration_to_vstr(cfg.bso_lock_delay, duration, sizeof(duration)));
		sprintf(opt[i++], "%-25s %lu", "BSO Lock Attempt Limit", cfg.bso_lock_attempts);
		sprintf(opt[i++], "%-25s %u", "Config File Backups", cfg.cfgfile_backups);
		opt[i][0] = 0;
		uifc.helpbuf=
			"~ Global Settings ~\n"
			"\n"
			"`Mailer Type` should normally be set to `Binkley/FLO` to enable SBBSecho's\n"
			"    \"Binkley-Style Outbound\" operating mode (a.k.a. `BSO` or `FLO` mode).\n"
			"    If you are using an `Attach`, `ArcMail`, or `FrontDoor` style FidoNet\n"
			"    mailer, then set this setting to `ArcMail/Attach`, but know that most\n"
			"    modern FidoNet mailers are Binkley-Style and therefore that mode of\n"
			"    operation in SBBSecho is more widely tested and supported.\n"
			"\n"
			"`Log Level` should normally be set to `Informational` but if you're\n"
			"    experiencing problems with SBBSecho and would like more verbose log\n"
			"    output, set this to `Debugging`. If you want less verbose logging,\n"
			"    set to higher-severity levels to reduce the number of log messages.\n"
			"\n"
			"`Log Timestmap Format` defines the format of the date/time-stamps added\n"
			"    along with each log message to the log file (e.g. sbbsecho.log).\n"
			"    The timestamp format is defined using standard C `strftime` notation.\n"
			"    The default format is: `" DEFAULT_LOG_TIME_FMT "`\n"
			"    For SBBSecho v2 timestamp format, use `%m/%d/%y %H:%M:%S`\n"
			"\n"
			"`Strict Packet Passwords`, when enabled, requires that Packet Passwords\n"
			"    must match the password for the linked-node from which the packet\n"
			"    was received, even if that linked-node has no password configured.\n"
			"    If you wish to revert to the SBBSecho v2 behavior with less strict\n"
			"    enforcement of matching packet passwords, disable this option.\n"
			"    Default: Enabled\n"
			"\n"
			"`Use FTN Domain/Zone Map`, when enabled, uses the configuration file\n"
			"    `ctrl/ftn_domains.ini` to determine the mapping between FTN domains\n"
			"    and FTN zones and the relevant BSO outbound directory.\n"
			"    Default: Disabled\n"
			"\n"
			"`BSY Mutex File Timeout` determines the maximum age of an existing\n"
			"    mutex file (`*.bsy`) before SBBSecho will act as though the mutex\n"
			"    file was not present.  This setting applies to the global\n"
			"    `sbbsecho.bsy` file as well as the BSO lock (`*.bsy`) files for\n"
			"    individual nodes.\n"
			"    Default: 12 hours\n"
			"\n"
			"`BSO Lock Attempt Delay` determines the amount of time between BSO\n"
			"    node lock attempts (via `*.bsy` files in the relevant outbound\n"
			"    directory).\n"
			"    Default: 10 seconds\n"
			"\n"
			"`BSO Lock Attempt Limit` determines the maximum number of BSO node lock\n"
			"    attempts before SBBSecho will give-up and move on to another node\n"
			"    to process mail.  This value multiplied by the `BSO Lock Attempt\n"
			"    Delay` should be much less than the `BSY Mutex File Timeout` value.\n"
			"    Default: 60 attempts\n"
			"\n"
			"`Config File Backups` determines the number of automatic backups of your\n"
			"    SBBSecho configuration file (e.g. `sbbsecho.ini`) that will be\n"
			"    maintained by SBBSecho Config and SBBSecho AreaFix.\n"
			;

		int key = uifc.list(WIN_BOT|WIN_L2R|WIN_ACT|WIN_SAV, 0, 0, 0, &global_opt,0, "Global Settings", opt);

		switch(key) {

			case -1:
				return;

			case 0:
				cfg.flo_mailer = !cfg.flo_mailer;
				break;

			case 1:
	uifc.helpbuf=
	"~ Log Level ~\n"
	"\n"
	"Select the minimum severity of log entries to be logged to the log file.\n"
	"The default/normal setting is `Informational`.";
				int i = cfg.log_level;
				i = uifc.list(WIN_MID|WIN_SAV,0,0,0,&i,0,"Log Level",logLevelStringList);
				if(i>=0 && i<=LOG_DEBUG)
					cfg.log_level=i;
				break;

			case 2:
				uifc.input(WIN_MID|WIN_SAV,0,0
					,"Log Timestamp Format", cfg.logtime, sizeof(cfg.logtime)-1, K_EDIT);
				break;

			case 3:
				cfg.strict_packet_passwords = !cfg.strict_packet_passwords;
				break;

			case 4:
				cfg.use_ftn_domains = !cfg.use_ftn_domains;
				break;

			case 5:
				duration_to_vstr(cfg.bsy_timeout, duration, sizeof(duration));
				if(uifc.input(WIN_MID|WIN_SAV, 0, 0, "BSY Mutex File Timeout", duration, 10, K_EDIT) > 0)
					cfg.bsy_timeout = (ulong)parse_duration(duration);
				break;

			case 6:
				duration_to_vstr(cfg.bso_lock_delay, duration, sizeof(duration));
				if(uifc.input(WIN_MID|WIN_SAV, 0, 0, "Delay Between BSO Lock Attempts", duration, 10, K_EDIT) > 0)
					cfg.bso_lock_delay = (ulong)parse_duration(duration);
				break;

			case 7:
				sprintf(str, "%lu", cfg.bso_lock_attempts);
				if(uifc.input(WIN_MID|WIN_SAV, 0, 0, "Maximum BSO Lock Attempts", str, 5, K_EDIT|K_NUMBER) > 0)
					cfg.bso_lock_attempts = atoi(str);
				break;

			case 8:
				sprintf(str, "%u", cfg.cfgfile_backups);
				if(uifc.input(WIN_MID|WIN_SAV, 0, 0, "Configuration File Backups", str, 5, K_EDIT|K_NUMBER) > 0)
					cfg.cfgfile_backups = atoi(str);
				break;
		}
	}
}

static bool new_node(unsigned new_nodenum)
{
	nodecfg_t* nodecfg = realloc(cfg.nodecfg, sizeof(nodecfg_t)*(cfg.nodecfgs+1));
	
	if(nodecfg == NULL)
		return false;

	cfg.nodecfg = nodecfg;
	for(unsigned int i=cfg.nodecfgs; i > new_nodenum; i--)
		memcpy(&cfg.nodecfg[i], &cfg.nodecfg[i-1], sizeof(nodecfg_t));

	cfg.nodecfgs++;
	memset(&cfg.nodecfg[new_nodenum], 0, sizeof(nodecfg_t));
	return true;
}

static bool new_arcdef(unsigned new_arcnum)
{
	arcdef_t * arcdef = realloc(cfg.arcdef, sizeof(arcdef_t)*(cfg.arcdefs+1));

	if(arcdef == NULL)
		return false;

	cfg.arcdef = arcdef;

	for(unsigned j=cfg.arcdefs;j>new_arcnum;j--)
		memcpy(&cfg.arcdef[j],&cfg.arcdef[j-1], sizeof(arcdef_t));
	cfg.arcdefs++;
	memset(&cfg.arcdef[new_arcnum], 0, sizeof(arcdef_t));
	return true;
}

static bool new_list(unsigned new_listnum)
{
	echolist_t *listcfg = realloc(cfg.listcfg, sizeof(echolist_t)*(cfg.listcfgs+1));

	if(listcfg == NULL)
		return false;
	cfg.listcfg = listcfg;
	for(unsigned j=cfg.listcfgs;j>new_listnum;j--)
		memcpy(&cfg.listcfg[j],&cfg.listcfg[j-1], sizeof(echolist_t));
	cfg.listcfgs++;
	memset(&cfg.listcfg[new_listnum],0,sizeof(echolist_t));
	return true;
}

int main(int argc, char **argv)
{
	char str[256],*p;
	int i,j,k,x,dflt,nodeop=0,packop=0,listop=0;
	echolist_t savlistcfg;
	nodecfg_t savnodecfg;
	arcdef_t savarcdef;
	BOOL door_mode=FALSE;
	int		ciolib_mode=CIOLIB_MODE_AUTO;
	unsigned int u;
	char	sysop_aliases[256];
	sbbsecho_cfg_t orig_cfg;

	ZERO_VAR(savlistcfg);
	ZERO_VAR(savnodecfg);
	ZERO_VAR(savarcdef);

	fprintf(stderr,"\nSBBSecho Configuration  Version %u.%02u  Copyright %s "
		"Rob Swindell\n\n",SBBSECHO_VERSION_MAJOR, SBBSECHO_VERSION_MINOR, __DATE__+7);

	memset(&cfg,0,sizeof(cfg));
	str[0]=0;
	for(i=1;i<argc;i++) {
		if(argv[i][0]=='-')
			switch(toupper(argv[i][1])) {
                case 'D':
					printf("NOTICE: The -d option is deprecated, use -id instead\n");
					SLEEP(2000);
                    door_mode=TRUE;
                    break;
                case 'L':
                    uifc.scrn_len=atoi(argv[i]+2);
                    break;
                case 'E':
                    uifc.esc_delay=atoi(argv[i]+2);
                    break;
				case 'I':
					switch(toupper(argv[i][2])) {
						case 'A':
							ciolib_mode=CIOLIB_MODE_ANSI;
							break;
						case 'C':
							ciolib_mode=CIOLIB_MODE_CURSES;
							break;
						case 0:
							printf("NOTICE: The -i option is deprecated, use -if instead\n");
							SLEEP(2000);
						case 'F':
							ciolib_mode=CIOLIB_MODE_CURSES_IBM;
							break;
						case 'X':
							ciolib_mode=CIOLIB_MODE_X;
							break;
						case 'W':
							ciolib_mode=CIOLIB_MODE_CONIO;
							break;
						case 'D':
		                    door_mode=TRUE;
		                    break;
						default:
							goto USAGE;
					}
					break;
				case 'K':	/* Keyboard-only mode (no mouse support) */
					uifc.mode |= UIFC_NOMOUSE;
					break;
		        case 'M':   /* Monochrome mode */
        			uifc.mode|=UIFC_MONO;
                    break;
                case 'C':
        			uifc.mode|=UIFC_COLOR;
                    break;
                case 'V':
                    textmode(atoi(argv[i]+2));
                    break;
                default:
					USAGE:
                    printf("usage: echocfg [path/to/sbbsecho.ini] [options]"
                        "\n\noptions:\n\n"
						"-k  =  keyboard mode only (no mouse support)\n"
                        "-c  =  force color mode\n"
						"-m  =  force monochrome mode\n"
                        "-e# =  set escape delay to #msec\n"
						"-iX =  set interface mode to X (default=auto) where X is one of:\n"
#ifdef __unix__
						"       X = X11 mode\n"
						"       C = Curses mode\n"
						"       F = Curses mode with forced IBM charset\n"
#else
						"       W = Win32 native mode\n"
#endif
						"       A = ANSI mode\n"
						"       D = standard input/output/door mode\n"
                        "-v# =  set video mode to # (default=auto)\n"
                        "-l# =  set screen lines to # (default=auto-detect)\n"
                        );
        			exit(0);
		}
		else
			strcpy(str,argv[i]);
	}
	if(str[0]==0) {
		p=getenv("SBBSCTRL");
		if(!p) {
			p=getenv("SBBSNODE");
			if(!p) {
				goto USAGE;
				exit(1); 
			}
			strcpy(str,p);
			backslash(str);
			strcat(str,"../ctrl/sbbsecho.ini"); 
		}
		else {
			strcpy(str,p);
			backslash(str);
			strcat(str,"sbbsecho.ini"); 
		} 
	}
	SAFECOPY(cfg.cfgfile,str);

	if(!sbbsecho_read_ini(&cfg)) {
		fprintf(stderr, "ERROR %d (%s) reading %s\n", errno, strerror(errno), cfg.cfgfile);
		exit(1);
	}
	orig_cfg = cfg;

	// savnum=0;
	if((opt=(char **)malloc(sizeof(char *)*1000))==NULL) {
		puts("memory allocation error\n");
		exit(1); 
	}
	for(i=0;i<1000;i++)
		if((opt[i]=(char *)malloc(MAX_OPLN+1))==NULL) {
			puts("memory allocation error\n");
			exit(1); 
		}
	uifc.size=sizeof(uifc);
	if(!door_mode) {
		i=initciolib(ciolib_mode);
		if(i!=0) {
    		printf("ciolib library init returned error %d\n",i);
    		exit(1);
		}
    	i=uifcini32(&uifc);  /* curses/conio/X/ANSI */
	}
	else
    	i=uifcinix(&uifc);  /* stdio */

	if(i!=0) {
		printf("uifc library init returned error %d\n",i);
		exit(1);
	}

	uifc.timedisplay = NULL;
	sprintf(str,"SBBSecho Config v%u.%02u",SBBSECHO_VERSION_MAJOR, SBBSECHO_VERSION_MINOR);
	uifc.scrn(str);
	p=cfg.cfgfile;
	if(strlen(p) + strlen(str) + 4 > uifc.scrn_width)
		p=getfname(cfg.cfgfile);
	uifc.printf(uifc.scrn_width-(strlen(p)+1),1,uifc.bclr|(uifc.cclr<<4),p);

	/* Remember current menu item selections using these vars: */
	int netmail_opt = 0;
	int echomail_opt = 0;
	int path_opt = 0;
	int node_opt = 0;
	int archive_opt = 0;
	int echolist_opt = 0;
	dflt=0;
	while(1) {
		if(memcmp(&cfg, &orig_cfg, sizeof(cfg)) != 0)
			uifc.changes = TRUE;
		uifc.helpbuf=
	"~ SBBSecho Configuration ~\n\n"
	"This program allows you to easily configure the Synchronet BBS\n"
	"FidoNet-style EchoMail program known as `SBBSecho`.  Alternatively, you\n"
	"may edit the SBBSecho configuration file (e.g. `ctrl/sbbsecho.ini`) using\n"
	"an ASCII/plain-text editor.\n"
	"\n"
	"For detailed documentation, see `http://wiki.synchro.net/util:sbbsecho`\n"
	"\n"
	"The `Global Settings` sub-menu is where SBBSecho configuration settings\n"
	"are located which are neither NetMail nor EchoMail specific, but more\n"
	"general to the operation of SBBSecho.\n"
	"\n"
	"The `Linked Nodes` sub-menu is where you configure your FidoNet-style\n"
	"links: other FidoNet-style nodes/systems you regularly connect with\n"
	"to exchange mail/files.\n"
	"\n"
	"The `Archive Types` sub-menu is where you configure your archive\n"
	"programs (a.k.a. \"packers\") used for the packing and unpacking of\n"
	"EchoMail bundle files (usually in 'zip' format).\n"
	"\n"
	"The `NetMail Settings` sub-menu is where you configure settings specific\n"
	"to NetMail (private one-on-one networked mail).\n"
	"\n"
	"The `EchoMail Settings` sub-menu is where you configure settings specific\n"
	"to EchoMail (public group discussions in topical message areas, echoes).\n"
	"\n"
	"The `Paths and Filenames` sub-menu is where you configure your system's\n"
	"directory and file paths used by SBBSecho.\n"
	"\n"
	"The `EchoLists` sub-menu is for configuring additional (optional)\n"
	"lists of FidoNet-style message areas (echoes) in `BACKBONE.NA` file\n"
	"format.  These lists, if configured, are used in addition to your main\n"
	"`Area File` (e.g. areas.bbs) for advanced AreaFix/AreaMgr operations."
	;
		i=0;
		sprintf(opt[i++],"Global Settings...");
		sprintf(opt[i++],"Linked Nodes...");
		sprintf(opt[i++],"Archive Types...");
		sprintf(opt[i++],"NetMail Settings...");
		sprintf(opt[i++],"EchoMail Settings...");
		sprintf(opt[i++],"Paths and Filenames...");
		sprintf(opt[i++],"EchoLists...");
		if(uifc.changes)
			snprintf(opt[i++],MAX_OPLN-1,"Save Changes to %s", getfname(cfg.cfgfile));
		opt[i][0]=0;
		switch(uifc.list(WIN_ORG|WIN_MID|WIN_ACT|WIN_ESC,0,0,0,&dflt,0
			,"Configure SBBSecho",opt)) {

			case 0:
				global_settings();
				break;

			case 1:
				i=0;
				while(1) {
					uifc.helpbuf=
	"~ Linked Nodes ~\n\n"
	"From this menu you can configure the settings for your linked\n"
	"FidoNet-style nodes (uplinks and downlinks).\n"
	"\n"
	"A single node configuration can represent one node or a collection\n"
	"of nodes, by using the `ALL` wildcard word.\n"
	"\n"
	"The hexadecimal numbers in parentheses are provided as an aide when\n"
	"correlating FidoNet files and BSO directories with node numbers."
	;

					for(u=0;u<cfg.nodecfgs;u++) {
						char hexaddr[16] = "";
						if(cfg.nodecfg[u].addr.zone != 0xffff) {
							if(!faddr_contains_wildcard(&cfg.nodecfg[u].addr))
								sprintf(hexaddr, "(%04hx%04hx)", cfg.nodecfg[u].addr.net,cfg.nodecfg[u].addr.node);
							else
								sprintf(hexaddr, "(.%03X)", cfg.nodecfg[u].addr.zone);
						}
						snprintf(opt[u], MAX_OPLN-1, "%-16s %-10s  %s"
							,faddrtoa(&cfg.nodecfg[u].addr), hexaddr
							,cfg.nodecfg[u].name[0] ? cfg.nodecfg[u].name : cfg.nodecfg[u].comment);
					}
					opt[u][0]=0;
					int mode = WIN_SAV | WIN_INS | WIN_DEL | WIN_ACT 
						| WIN_INSACT | WIN_DELACT | WIN_XTR;
					if(cfg.nodecfgs)
						mode |= WIN_COPY | WIN_CUT;
					if (savnodecfg.addr.zone)
						mode |= WIN_PASTE | WIN_PASTEXTR;
					i=uifc.list(mode,0,0,0,&node_opt,0,"Linked Nodes",opt);
					if(i==-1)
						break;
					int msk = i&MSK_ON;
					i &= MSK_OFF;
					if (msk == MSK_INS) {
						str[0]=0;
	uifc.helpbuf=
	"~ Address ~\n\n"
	"This is the FidoNet style address of the node you wish to add (3D or 4D).\n"
	"The `ALL` wildcard may be usd for portions of the address.\n"
	;
						if(uifc.input(WIN_MID|WIN_SAV,0,0
							,"Node Address (ALL wildcard allowed)",str
							,25,K_EDIT)<1)
							continue;
						if(!new_node(i)) {
							printf("\nMemory Allocation Error\n");
							exit(1); 
						}
						cfg.nodecfg[i].addr=atofaddr(str);
						uifc.changes=TRUE;
						continue; 
					}

					if (msk == MSK_DEL || msk == MSK_CUT) {
						if(msk == MSK_CUT)
							memcpy(&savnodecfg, &cfg.nodecfg[i], sizeof(nodecfg_t));
						cfg.nodecfgs--;
						if(cfg.nodecfgs<=0) {
							cfg.nodecfgs=0;
							continue; 
						}
						for(u=i;u<cfg.nodecfgs;u++)
							memcpy(&cfg.nodecfg[u],&cfg.nodecfg[u+1]
								,sizeof(nodecfg_t));
						uifc.changes=TRUE;
						continue; 
					}
					if (msk == MSK_COPY) {
						memcpy(&savnodecfg,&cfg.nodecfg[i],sizeof(nodecfg_t));
						continue; 
					}
					if (msk == MSK_PASTE) {
						if(!new_node(i))
							continue;
						memcpy(&cfg.nodecfg[i],&savnodecfg,sizeof(nodecfg_t));
						uifc.changes=TRUE;
						continue; 
					}
					while(1) {
	uifc.helpbuf=
	"~ Linked Node Settings ~\n"
	"\n"
	"`Address` is the FidoNet-style address in the Zone:Net/Node (3D) or\n"
	"    Zone:Net/Node.Point (4D) format. The wildcard word '`ALL`' may be used\n"
	"    in place of one of the fields to create a node configuration which\n"
	"    will apply to *all* nodes matching that address pattern.\n"
	"    e.g. '`1:ALL`' matches all nodes within FidoNet Zone 1.\n"
	"\n"
	"`Name` is name of the system operator of the configured node. This is\n"
	"    as the destination name for AreaFix Notification NetMail messages.\n"
	"\n"
	"`Comment` is a note to yourself about this node. Setting this to the\n"
	"    BBS name or official FidoNet title corresponding with the configured\n"
	"    node can be a helpful reminder to yourself later.\n"
	"\n"
	"`Archive Type` is the name of an archive type corresponding with one of\n"
	"    your configured archive types or '`None`'.  This archive type will\n"
	"    be used when creating EchoMail bundles or if `None`, raw/uncompressed\n"
	"    EchoMail packets will be sent to this node.\n"
	"    This setting may be managed by the node using AreaFix requests.\n"
	"\n"
	"`Packet Type` is the type of outbound packet generated for this node.\n"
	"    Incoming packet types are automatically detected from among the list\n"
	"    of supported packet types (`2`, `2.2`, `2e`, and `2+`).\n"
	"    The default outbound packet type is `2+`.\n"
	"\n"
	"`Packet Password` is an optional password that may be added to outbound\n"
	"    packets for this node.  Incoming packet passwords are compared with\n"
	"    this password value.  If this password is blank/empty and `Strict\n"
	"    Packet Passwords` are enabled, then incoming packets from this node\n"
	"    must also have no password.  Packet passwords are case insensitive.\n"
	"    This setting may be managed by the node using AreaFix requests.\n"
	"\n"
	"`TIC File Password` is an optional password that may be configured here\n"
	"    (and in your `sbbsecho.ini` file) for use by `tickit.js` when creating\n"
	"    or authenticating `.TIC` files.\n"
	"    This setting may be managed by the node using AreaFix requests.\n"
	"\n"
	"`AreaFix Support` is a toggle that determines whether or not this node\n"
	"    may send AreaFix NetMail requests to your system to perform remote\n"
	"    area management.\n"
	"\n"
	"`AreaFix Password` is an optional password used to authenticate inbound\n"
	"    AreaFix NetMail requests (Remote Area Management) from this node.\n"
	"    AreaFix Passwords are case insensitive.\n"
	"    This setting may be managed by the node using AreaFix requests.\n"
	"\n"
	"`EchoList Keys` is a list of keys which enable AreaFix access to one or\n"
	"    more additional EchoLists.\n"
	"\n"
	"`Status` is the default mode for sending mail to this node: `Normal`, `Hold`\n"
	"    (wait for pickup) or `Crash` (immediate).\n"
	"\n"
	"`Direct` determines whether to connect to this node directly (whenever\n"
	"    possible) when sending mail to this node.\n"
	"\n"
	"`Passive` is used to temporarily disable (pause) the packing and sending\n"
	"    of EchoMail for this node.  The opposite of Passive is `Active`.\n"
	"    This setting may be managed by the node using AreaFix requests.\n"
	"\n"
	"`Send Notify List` is used to flag nodes that you want notified via\n"
	"    NetMail of their current AreaFix settings whenever SBBSecho is run\n"
	"    with the '`G`' option.\n"
	"\n"
	"`Uplink for Message Groups` is an optional list of Message Groups (short\n"
	"    names) for which this node is a hub/uplink for your system.  This\n"
	"    setting is used in combination with the `Auto Add Sub-boards` feature\n"
	"    to auto-link hubs with the newly added areas in your Area File.\n"
	"\n"
	"`Route To` is only used in Binkley-Style Outbound (BSO/FLO) operating\n"
	"    mode and is used to set the FTN address to route mail for this node.\n"
	"\n"
	"`Inbox Directory` is only used in BSO operating mode and is an optional\n"
	"    alternate directory to search for incoming files from this node\n"
	"    (e.g. used in combination with BinkD's ibox setting).\n"
	"\n"
	"`Outbox Directory` is only used in BSO operating mode and is an optional\n"
	"    alternate directory to place outbound files for this node (e.g. used\n"
	"    in combination with BinkD's obox setting).\n"
	;
						j=0;
						snprintf(opt[j++],MAX_OPLN-1,"%-30.30s %s","Address"
							,faddrtoa(&cfg.nodecfg[i].addr));
						snprintf(opt[j++],MAX_OPLN-1,"%-30.30s %s","Name"
							,cfg.nodecfg[i].name);
						snprintf(opt[j++],MAX_OPLN-1,"%-30.30s %s","Comment"
							,cfg.nodecfg[i].comment);
						snprintf(opt[j++],MAX_OPLN-1,"%-30.30s %s","Archive Type"
							,cfg.nodecfg[i].archive ==  SBBSECHO_ARCHIVE_NONE ?
							"None":cfg.nodecfg[i].archive->name);
						snprintf(opt[j++],MAX_OPLN-1,"%-30.30s %s","Packet Type"
							,pktTypeStringList[cfg.nodecfg[i].pkt_type]);
						snprintf(opt[j++],MAX_OPLN-1,"%-30.30s %s","Packet Password"
							,cfg.nodecfg[i].pktpwd);
						snprintf(opt[j++],MAX_OPLN-1,"%-30.30s %s","TIC File Password"
							,cfg.nodecfg[i].ticpwd);
						snprintf(opt[j++],MAX_OPLN-1,"%-30.30s %s","AreaFix Support"
							,cfg.nodecfg[i].areafix ? "Yes" : "No");
						snprintf(opt[j++],MAX_OPLN-1,"%-30.30s %s","AreaFix Password"
							,cfg.nodecfg[i].password);
						snprintf(opt[j++],MAX_OPLN-1,"%-30.30s %s","EchoList Keys"
							,strListCombine(cfg.nodecfg[i].keys,str,sizeof(str),","));
						snprintf(opt[j++],MAX_OPLN-1,"%-30.30s %s","Status"
							,mailStatusStringList[cfg.nodecfg[i].status]);
						snprintf(opt[j++],MAX_OPLN-1,"%-30.30s %s","Direct"
							,cfg.nodecfg[i].direct ? "Yes":"No");
						snprintf(opt[j++],MAX_OPLN-1,"%-30.30s %s","Passive"
							,cfg.nodecfg[i].passive ? "Yes":"No");
						snprintf(opt[j++],MAX_OPLN-1,"%-30.30s %s","Send Notify List"
							,cfg.nodecfg[i].send_notify ? "Yes" : "No");
						snprintf(opt[j++],MAX_OPLN-1,"%-30.30s %s","Uplink for Message Groups"
							,strListCombine(cfg.nodecfg[i].grphub,str,sizeof(str),","));
						if(cfg.flo_mailer) {
							snprintf(opt[j++],MAX_OPLN-1,"%-30.30s %s","Route To"
								,cfg.nodecfg[i].route.zone
								? faddrtoa(&cfg.nodecfg[i].route) : "Disabled");
							if(!faddr_contains_wildcard(&cfg.nodecfg[i].addr)) {
								snprintf(opt[j++],MAX_OPLN-1,"%-30.30s %s", "Inbox Directory", cfg.nodecfg[i].inbox);
								snprintf(opt[j++],MAX_OPLN-1,"%-30.30s %s", "Outbox Directory", cfg.nodecfg[i].outbox);
							}
						}
						opt[j][0]=0;
						SAFEPRINTF(str, "Linked Node - %s"
							,cfg.nodecfg[i].name[0] ? cfg.nodecfg[i].name : faddrtoa(&cfg.nodecfg[i].addr));
						k=uifc.list(WIN_MID|WIN_ACT|WIN_SAV,0,0,0,&nodeop,0,str,opt);
						if(k==-1)
							break;
						switch(k) {
							case __COUNTER__:
	uifc.helpbuf=
	"~ Address ~\n\n"
	"This is the FidoNet style address of this linked node.\n";
								strcpy(str,faddrtoa(&cfg.nodecfg[i].addr));
								if(uifc.input(WIN_MID|WIN_SAV,0,0
									,"Node Address (ALL wildcard allowed)",str
									,25,K_EDIT|K_UPPER)>0)
									cfg.nodecfg[i].addr=atofaddr(str);
								break;
							case __COUNTER__:
	uifc.helpbuf=
	"~ Name ~\n\n"
	"This is an optional NetMail destination name for the node (e.g. the sysop's name).\n"
	"This is used for AreaFix Notification NetMail messages.\n";
								uifc.input(WIN_MID|WIN_SAV,0,0
									,"Name"
									,cfg.nodecfg[i].name,sizeof(cfg.nodecfg[i].name)-1
									,K_EDIT);
								break;
							case __COUNTER__:
	uifc.helpbuf=
	"~ Comment ~\n\n"
	"This is an optional comment for the node (e.g. the sysop's name).\n"
	"This is used for informational purposes only.\n";
								uifc.input(WIN_MID|WIN_SAV,0,0
									,"Comment"
									,cfg.nodecfg[i].comment,sizeof(cfg.nodecfg[i].comment)-1
									,K_EDIT);
								break;
							case __COUNTER__:
	uifc.helpbuf=
	"~ Archive Type ~\n\n"
	"This is the archive type that will be used for compressing packets\n"
	"into archive bundles for this node.\n";
								int cur=cfg.arcdefs;
								for(u=0;u<cfg.arcdefs;u++) {
									if(cfg.nodecfg[i].archive == &cfg.arcdef[u])
										cur=u;
									strcpy(opt[u],cfg.arcdef[u].name);
								}
								strcpy(opt[u++],"None");
								opt[u][0]=0;
								k=uifc.list(WIN_RHT|WIN_SAV,0,0,0,&cur,0
									,"Archive Type",opt);
								if(k==-1)
									break;
								if((unsigned)k>=cfg.arcdefs)
									cfg.nodecfg[i].archive = SBBSECHO_ARCHIVE_NONE;
								else
									cfg.nodecfg[i].archive = &cfg.arcdef[k];
								uifc.changes=TRUE;
								break;
							case __COUNTER__:
	uifc.helpbuf=
	"~ Packet Type ~\n\n"
	"This is the packet header type that will be used in mail packets\n"
	"created for this node.  SBBSecho defaults to creating `Type-2+` packets.\n"
	"\n"
	"`Type-2  ` packets are defined in FTS-0001.16 (Stone Age)\n"
	"`Type-2e ` packets are defined in FSC-0039.04 (Sometimes called 2+)\n"
	"`Type-2+ ` packets are defined in FSC-0048.02 (4D address support)\n"
	"`Type-2.2` packets are defined in FSC-0045.01 (5D address support)\n"
	;
								j=cfg.nodecfg[i].pkt_type;
								k=uifc.list(WIN_RHT|WIN_SAV,0,0,0,&j,0,"Packet Type"
									,pktTypeStringList);
								if(k==-1)
									break;
								cfg.nodecfg[i].pkt_type=k;
								uifc.changes=TRUE;
								break;
							case __COUNTER__:
	uifc.helpbuf=
	"~ Packet Password ~\n\n"
	"This is an optional password that SBBSecho will place into packets\n"
	"destined for this node.\n";
								uifc.input(WIN_MID|WIN_SAV,0,0
									,"Packet Password (optional)"
									,cfg.nodecfg[i].pktpwd,sizeof(cfg.nodecfg[i].pktpwd)-1
									,K_EDIT|K_UPPER);
								break;
							case __COUNTER__:
	uifc.helpbuf=
	"~ TIC File Password ~\n\n"
	"This is an optional password that tickit.js will use for creating\n"
	"and authenticating `.TIC` files to/from this node.\n";
								uifc.input(WIN_MID|WIN_SAV,0,0
									,"TIC File Password (optional)"
									,cfg.nodecfg[i].ticpwd,sizeof(cfg.nodecfg[i].ticpwd)-1
									,K_EDIT|K_UPPER);
								break;
							case __COUNTER__:
	uifc.helpbuf=
	"~ AreaFix Support ~\n\n"
	"If you wish for this node to be able to remotely configure their configuration\n"
	"via `AreaFix` NetMail messages, set to option to `Yes`.\n";
								k = cfg.nodecfg[i].areafix;
								switch(uifc.list(WIN_MID|WIN_SAV,0,0,0,&k,0
									,"AreaFix Support",uifcYesNoOpts)) {
									case 0:	cfg.nodecfg[i].areafix = true;	uifc.changes=TRUE; break;
									case 1:	cfg.nodecfg[i].areafix = false;	uifc.changes=TRUE; break;
								}
								break;
							case __COUNTER__:
	uifc.helpbuf=
	"~ AreaFix Password ~\n\n"
	"This is the password that will be used by this node when doing remote\n"
	"AreaManager / AreaFix functions.\n";
								uifc.input(WIN_MID|WIN_SAV,0,0
									,"AreaFix Password"
									,cfg.nodecfg[i].password,sizeof(cfg.nodecfg[i].password)-1
									,K_EDIT|K_UPPER);
								break;
							case __COUNTER__:
	uifc.helpbuf=
	"~ EchoList Keys ~\n\n"
	"These are a named-keys to be given to this node allowing access to one or\n"
	"more of the configured `EchoLists`\n";
								while(1) {
									for(j=0; cfg.nodecfg[i].keys!=NULL && cfg.nodecfg[i].keys[j]!=NULL ;j++)
										strcpy(opt[j],cfg.nodecfg[i].keys[j]);
									opt[j][0]=0;
									k=uifc.list(WIN_SAV|WIN_INS|WIN_DEL|WIN_ACT|
										WIN_XTR|WIN_INSACT|WIN_DELACT|WIN_RHT
										,0,0,0,&k,0,"EchoList Keys",opt);
									if(k==-1)
										break;
									if((k&MSK_ON)==MSK_INS) {
										k&=MSK_OFF;
										if(uifc.input(WIN_MID|WIN_SAV,0,0
											,"EchoList Key",str,SBBSECHO_MAX_KEY_LEN
											,K_UPPER)<1)
											continue;
										strListInsert(&cfg.nodecfg[i].keys, str, k);
										uifc.changes=TRUE;
										continue; 
									}

									if((k&MSK_ON)==MSK_DEL) {
										k&=MSK_OFF;
										strListRemove(&cfg.nodecfg[i].keys, k);
										uifc.changes=TRUE;
										continue; 
									}
									SAFECOPY(str,cfg.nodecfg[i].keys[k]);
									uifc.input(WIN_MID|WIN_SAV,0,0,"EchoList Key"
										,str,SBBSECHO_MAX_KEY_LEN,K_EDIT|K_UPPER);
									strListReplace(cfg.nodecfg[i].keys, k, str);
									uifc.changes=TRUE;
									continue; 
								}
								break;
							case __COUNTER__:
	uifc.helpbuf=
	"~ Mail Status ~\n\n"
	"Set the mail status for this node: `Normal`, `Hold`, or `Crash`.\n";
								j=cfg.nodecfg[i].status;
								k=uifc.list(WIN_RHT|WIN_SAV,0,0,0,&j,0,"Mail Status"
									,mailStatusStringList);
								if(k==-1)
									break;
								if(cfg.nodecfg[i].status!=k) {
									cfg.nodecfg[i].status=k;
									uifc.changes=TRUE;
								}
								break;
							case __COUNTER__:
								k = !cfg.nodecfg[i].direct;
								switch(uifc.list(WIN_MID|WIN_SAV,0,0,0,&k,0
									,"Direct Delivery",uifcYesNoOpts)) {
									case 0:	cfg.nodecfg[i].direct = true;	uifc.changes=TRUE; break;
									case 1:	cfg.nodecfg[i].direct = false;	uifc.changes=TRUE; break;
								}
								break;
							case __COUNTER__:
								k = !cfg.nodecfg[i].passive;
								switch(uifc.list(WIN_MID|WIN_SAV,0,0,0,&k,0
									,"Passive Node",uifcYesNoOpts)) {
									case 0:	cfg.nodecfg[i].passive = true;	uifc.changes=TRUE; break;
									case 1:	cfg.nodecfg[i].passive = false;	uifc.changes=TRUE; break;
								}
								break;
							case __COUNTER__:
								k = !cfg.nodecfg[i].send_notify;
								switch(uifc.list(WIN_MID|WIN_SAV,0,0,0,&k,0
									,"Send AreaFix Notifications",uifcYesNoOpts)) {
									case 0:	cfg.nodecfg[i].send_notify = true;	uifc.changes=TRUE; break;
									case 1:	cfg.nodecfg[i].send_notify = false;	uifc.changes=TRUE; break;
								}
								break;
							case __COUNTER__:
	uifc.helpbuf=
	"~ Uplink for Message Groups ~\n\n"
	"These are Message Group short names (as configured in SCFG) for which\n"
	"this linked-node is your system's uplink (hub).\n"
	"\n"
	"Use of this setting allows your hub to be automatically linked with new\n"
	"areas when new Sub-boards (within a listed group) are auto-added to the\n"
	"Area File."
	;
								while(1) {
									for(j=0; cfg.nodecfg[i].grphub!=NULL && cfg.nodecfg[i].grphub[j]!=NULL ;j++)
										strcpy(opt[j],cfg.nodecfg[i].grphub[j]);
									opt[j][0]=0;
									k=uifc.list(WIN_SAV|WIN_INS|WIN_DEL|WIN_ACT|
										WIN_XTR|WIN_INSACT|WIN_DELACT|WIN_RHT
										,0,0,0,&k,0,"Uplink for Message Groups",opt);
									if(k==-1)
										break;
									if((k&MSK_ON)==MSK_INS) {
										k&=MSK_OFF;
										if(uifc.input(WIN_MID|WIN_SAV,0,0
											,"Message Group (short name)",str,LEN_GSNAME
											,/* kmode: */0) < 1)
											continue;
										strListInsert(&cfg.nodecfg[i].grphub, str, k);
										uifc.changes=TRUE;
										continue; 
									}

									if((k&MSK_ON)==MSK_DEL) {
										k&=MSK_OFF;
										strListRemove(&cfg.nodecfg[i].grphub, k);
										uifc.changes=TRUE;
										continue; 
									}
									SAFECOPY(str,cfg.nodecfg[i].grphub[k]);
									uifc.input(WIN_MID|WIN_SAV,0,0,"Message Group (short name)"
										,str,LEN_GSNAME,K_EDIT|K_UPPER);
									strListReplace(cfg.nodecfg[i].grphub, k, str);
									uifc.changes=TRUE;
									continue; 
								}
								break;
							case __COUNTER__:
	uifc.helpbuf=
	"~ Route To ~\n\n"
	"When using a BSO/FLO type mailer, this is the Fido address to route mail\n"
	"for this node(s) to.\n"
	"\n"
	"This option is normally only used with wildcard type node entries\n"
	"(e.g. `ALL`, or `1:ALL`, `2:ALL`, etc.) and is used to route non-direct\n"
	"NetMail packets to your uplink node (hub).\n";
								strcpy(str,faddrtoa(&cfg.nodecfg[i].route));
								if(uifc.input(WIN_MID|WIN_SAV,0,0
									,"Node Address to Route To",str
									,25,K_EDIT) >= 0) {
									if(str[0])
										cfg.nodecfg[i].route=atofaddr(str);
									else
										cfg.nodecfg[i].route.zone=0;
									uifc.changes=TRUE;
								}
								break;
							case __COUNTER__:
								uifc.input(WIN_MID|WIN_SAV,0,0,"Inbound FileBox Directory"
									,cfg.nodecfg[i].inbox, sizeof(cfg.nodecfg[i].inbox)-1
									,K_EDIT);
								break;
							case __COUNTER__:
								uifc.input(WIN_MID|WIN_SAV,0,0,"Outbound FileBox Directory"
									,cfg.nodecfg[i].outbox, sizeof(cfg.nodecfg[i].outbox)-1
									,K_EDIT);
								break;
						} 
					} 
				}
				break;

			case 5:	/* Paths and Filenames... */
				j=0;
				while(1) {
					i=0;
					snprintf(opt[i++],MAX_OPLN-1,"%-30.30s %s","Non-secure Inbound Directory"
						,cfg.inbound[0] ? cfg.inbound : DEFAULT_INBOUND);
					snprintf(opt[i++],MAX_OPLN-1,"%-30.30s %s","Secure Inbound Directory"
						,cfg.secure_inbound[0] ? cfg.secure_inbound : DEFAULT_SECURE_INBOUND);
					snprintf(opt[i++],MAX_OPLN-1,"%-30.30s %s","Outbound Directory"
						,cfg.outbound[0] ? cfg.outbound : DEFAULT_OUTBOUND);
					snprintf(opt[i++],MAX_OPLN-1,"%-30.30s %s","Area File"
						,cfg.areafile[0] ? cfg.areafile : DEFAULT_AREA_FILE);
					snprintf(opt[i++],MAX_OPLN-1,"%-30.30s %s","Bad Area File"
						,cfg.badareafile[0] ? cfg.badareafile : DEFAULT_BAD_AREA_FILE);
					snprintf(opt[i++],MAX_OPLN-1,"%-30.30s %s","Log File"
						,cfg.logfile[0] ? cfg.logfile : DEFAULT_LOG_FILE);
					snprintf(opt[i++],MAX_OPLN-1,"%-30.30s %s","Echo Statistics File"
						,cfg.echostats[0] ? cfg.echostats : DEFAULT_ECHOSTATS_FILE);
					snprintf(opt[i++],MAX_OPLN-1,"%-30.30s %s","Temporary File Directory"
						,cfg.temp_dir[0] ? cfg.temp_dir	: DEFAULT_TEMP_DIR);
					snprintf(opt[i++],MAX_OPLN-1,"%-30.30s %s","Outgoing Semaphore File"
						,cfg.outgoing_sem);
					opt[i][0]=0;
					uifc.helpbuf=
						"~ Paths and Filenames ~\n\n"
						"From this menu you can configure the paths and filenames that SBBSecho\n"
						"will use during its operation (e.g. importing and exporting messages).\n";
					j=uifc.list(WIN_BOT|WIN_L2R|WIN_ACT|WIN_SAV,0,0,0,&path_opt,0
						,"Paths and Filenames",opt);
					if(j==-1)
						break;
					switch(j) {
						case 0:
	uifc.helpbuf=
	"~ Non-secure Inbound Directory ~\n\n"
	"This is the path where your FTN mailer stores, and where SBBSecho will\n"
	"look for, incoming files (potentially including message bundles and\n"
	"packets) from unauthenticated (non-secure) mailer sessions.\n"
	"\n"
	"Default value is `" DEFAULT_INBOUND "`."
	;
							uifc.input(WIN_L2R|WIN_SAV,0,0,"Non-secure Inbound Directory"
								,cfg.inbound,sizeof(cfg.inbound)-1
								,K_EDIT);
							break;

						case 1:
	uifc.helpbuf=
	"~ Secure Inbound Directory ~\n\n"
	"This is the path where your FTN mailer stores, and where SBBSecho will\n"
	"look for, incoming message bundles and packets for `Secure` (password\n"
	"protected) sessions.\n"
	"\n"
	"Default value is `" DEFAULT_SECURE_INBOUND "`."
	;
							uifc.input(WIN_L2R|WIN_SAV,0,0,"Secure Inbound Directory"
								,cfg.secure_inbound,sizeof(cfg.secure_inbound)-1
								,K_EDIT);
							break;

						case 2:
	uifc.helpbuf=
	"~ Outbound Directory ~\n\n"
	"This is the path where your FTN mailer will look for, and where SBBSecho\n"
	"will place, outgoing message bundles and packets.\n"
	"\n"
	"In Binkley-Style Outbound mode, this serves as the base directory\n"
	"name for special foreign zone and point destination nodes as well."
	"\n"
	"Default value is `" DEFAULT_OUTBOUND "`."
	;
							uifc.input(WIN_L2R|WIN_SAV,0,0,"Outbound Directory"
								,cfg.outbound,sizeof(cfg.outbound)-1
								,K_EDIT);
							break;

						case 3:
	uifc.helpbuf=
	"~ Area File ~\n\n"
	"This is the path of the file SBBSecho will use as your primary\n"
	"list of FidoNet-style message areas (default is `data/areas.bbs`).\n"
	"\n"
	"Each line in the file defines an FTN message area (echo) of the format:\n"
	"\n"
	"   <`code`> <`tag`> [[`link`] [`link`] [...]]\n"
	"\n"
	"Each field is separated by one or more white-space characters:\n"
	"\n"
	"   `<code>` is the Synchronet `internal code` for the local sub-board\n"
	"   `<tag>`  is the network's agreed-upon `echo tag` for the message area\n"
	"   `[link]` is an `FTN address` to send and receive messages for this area\n"
	"          (there may be many linked nodes for each area)\n"
	"          (often your FTN uplink/hub may be the only linked node)\n"
	"\n"
	"Example Area Line:\n"
	"\n"
	"   `FIDO_BBS_CARN    BBS_CARNIVAL                        1:218/700`\n"
	"\n"
	"Notes:\n"
	"\n"
	" `*` Only the `<code>` and `<tag>` fields are required\n"
	" `*` The `<code>` and `<tag>` fields are case in-sensitive\n"
	" `*` The `[link]` fields must be 2D, 3D, or 4D FidoNet-style node addresses\n"
	" `*` The '`<`' and '`>`', '`[`' and '`]`' characters are not part of the syntax\n"
	" `*` Lines beginning with a semicolon (`;`) are ignored (i.e. comments)\n"
	" `*` Leading white-space characters are ignored\n"
	" `*` Blank lines are ignored\n"
	" `*` A `<tag>` value of `*` indicates a `bad echo` (unknown) area\n"
	" `*` A `<code>` value of `P` indicates a pass-through message area\n"
	" `*` This file may be import/exported to/from your `Message Areas` in `SCFG`\n"
	" `*` This file may be remotely modified by authorized nodes using `AreaFix`\n"
	"\n"
	"Default value is `" DEFAULT_AREA_FILE "`."
	;
							uifc.input(WIN_L2R|WIN_SAV,0,0,"Area File"
								,cfg.areafile,sizeof(cfg.areafile)-1
								,K_EDIT);
							break;

						case 4:
	uifc.helpbuf=
	"~ Bad Area File ~\n\n"
	"This is the path of the file SBBSecho will use to record the names\n"
	"(echo tags) and descriptions of FTN message areas (echoes) that your\n"
	"system has received EchoMail for, but does not carry locally. The\n"
	"default path/filename is `" DEFAULT_BAD_AREA_FILE "`.\n"
	"\n"
	"Notes:\n"
	"\n"
	" `*` The descriptions of the areas will only be included if the\n"
	"   corresponding echo tags can be located in one of your configured\n"
	"   `EchoLists`.\n"
	"\n"
	" `*` The format of the file is the same as `BACKBONE.NA` and suitable for\n"
	"   importing into a Synchronet Message Group using `SCFG`.\n"
	"\n"
	" `*` SBBSecho will automatically sort and maintain this list, removing\n"
	"   areas if they are added to your configuration (`SCFG->Message Areas`)\n"
	"   and your `Area File`.\n"
	;
							uifc.input(WIN_L2R|WIN_SAV,0,0,"Bad Area File"
								,cfg.badareafile,sizeof(cfg.badareafile)-1
								,K_EDIT);
							break;

						case 5:
	uifc.helpbuf=
	"~ Log File ~\n\n"
	"This is the path of the file SBBSecho will use to log information each\n"
	"time it is run (default is `" DEFAULT_LOG_FILE "`)."
	;
							uifc.input(WIN_L2R|WIN_SAV,0,0,"Log File"
								,cfg.logfile,sizeof(cfg.logfile)-1
								,K_EDIT);
							break; 

						case 6:
	uifc.helpbuf=
	"~ EchoStats File ~\n\n"
	"This is the path of the file SBBSecho will use to track statistics for\n"
	"EchoMail message areas (default is `" DEFAULT_ECHOSTATS_FILE "`)."
	;
							uifc.input(WIN_L2R|WIN_SAV,0,0,"EchoStats File"
								,cfg.echostats,sizeof(cfg.echostats)-1
								,K_EDIT);
							break; 

						case 7:
	uifc.helpbuf=
	"~ Temporary File Directory ~\n\n"
	"This is the directory where SBBSecho will store temporary files that\n"
	"it creates and uses during its run-time (default is `" DEFAULT_TEMP_DIR "`)."
	;
							uifc.input(WIN_L2R|WIN_SAV,0,0,"Temp Dir"
								,cfg.temp_dir,sizeof(cfg.temp_dir)-1
								,K_EDIT);
							break; 

						case 8:
	uifc.helpbuf=
	"~ Outgoing Semaphore File ~\n\n"
	"This is an optional file to create/touch whenever there are new outbound\n"
	"files created or updated by SBBSecho."
	;
							uifc.input(WIN_L2R|WIN_SAV,0,0,"Outgoing Sem File"
								,cfg.outgoing_sem,sizeof(cfg.outgoing_sem)-1
								,K_EDIT);
							break; 
					} 
				}
				break;

			case 3:	/* NetMail Settings */
				j=0;
				while(1) {
					uifc.helpbuf=
	"~ NetMail Settings ~\n"
	"\n"
	"`Sysop Aliases` is a comma-separated list of names by which the sysop\n"
	"    (user #1) may receive NetMail messages, in addition to the alias\n"
	"    and real name associated with their BBS user account.\n"
	"    This setting defaults to just '`SYSOP`'.\n"
	"\n"
    "`Default Recipient` is the name of the user account you wish to receive\n"
	"    inbound NetMail messages that have been addressed to an unrecognized\n"
	"    user name or alias.\n"
	"\n"
	"`Fuzzy Zone Operation` when set to `Yes`, if SBBSecho receives an inbound\n"
	"    netmail with no international zone information, it will compare the\n"
	"    net/node of the destination to the net/node information in your AKAs\n"
	"    and assume the zone of a matching AKA.\n"
	"    This setting defaults to `No`.\n"
	"\n"
	"`Kill/Ignore Empty NetMail Messages` will instruct SBBSecho to simply\n"
	"    discard (not import or export) NetMail messages without any body.\n"
	"    This setting defaults to `Yes`.\n"
	"\n"
	"`Delete Processed NetMail Messages` will instruct SBBSecho to delete\n"
	"    NetMail messages/files after they have been sent or imported.\n"
	"    When set to `No`, SBBSecho will mark them as Sent or Received instead.\n"
	"    This setting defaults to `Yes`.\n"
	"\n"
	"`Ignore NetMail Destination Address` will instruct SBBSecho to treat\n"
	"    all NetMail as though it is destined for one of your systems's FTN\n"
	"    addresses (AKAs) and potentially import it.\n"
	"    This setting defaults to `No`.\n"
	"\n"
	"`Ignore Netmail 'Sent' Attribute` will instruct SBBSecho to export\n"
	"    NetMail messages even when their 'Sent' attribute flag is set.\n"
	"    This setting `should not` be set to `Yes` when `Delete NetMail` is\n"
	"    disabled.\n"
	"    This setting defaults to `No`.\n"
	"\n"
	"`Ignore Netmail 'KillSent' Attribute` will instruct SBBSecho to ignore\n"
	"    this attribute flag and will `not` delete Sent NetMail.\n"
	"    This setting defaults to `No`.\n"
	"\n"
	"`Ignore Netmail 'Received' Attribute` will instruct SBBSecho to import\n"
	"    NetMail messages even when their 'Received' attribute flag is set.\n"
	"    This setting defaults to `No`.\n"
	"\n"
	"`Ignore NetMail 'Local' Attribute` will instruct SBBSecho to import\n"
	"    NetMail messages even when their 'Local' attribute flag is set.\n"
	"    This setting defaults to `No`.\n"
	"\n"
	"`Maximum Age of Imported NetMail` allows you to optionally set an age\n"
	"    limit of NetMail messages that may be imported.\n"
	"    This setting defaults to `None` (no maximum age).\n"
;
					i=0;
					strListCombine(cfg.sysop_alias_list, sysop_aliases, sizeof(sysop_aliases)-1, ",");
					snprintf(opt[i++],MAX_OPLN-1,"%-40.40s%s", "Sysop Aliases",sysop_aliases);
					snprintf(opt[i++],MAX_OPLN-1,"%-40.40s%s", "Default Recipient"
						,cfg.default_recipient);
					snprintf(opt[i++],MAX_OPLN-1,"%-40.40s%-3.3s","Fuzzy Zone Operation"
						,cfg.fuzzy_zone ? "Yes":"No");
					snprintf(opt[i++],MAX_OPLN-1,"%-40.40s%-3.3s","Kill/Ignore Empty NetMail "
						"Messages",cfg.kill_empty_netmail ? "Yes":"No");
					snprintf(opt[i++],MAX_OPLN-1,"%-40.40s%-3.3s","Delete Processed NetMail"
						,cfg.delete_netmail ? "Yes":"No");
					snprintf(opt[i++],MAX_OPLN-1,"%-40.40s%-3.3s","Ignore NetMail Destination Address"
						,cfg.ignore_netmail_dest_addr ? "Yes" : "No");
					snprintf(opt[i++],MAX_OPLN-1,"%-40.40s%-3.3s","Ignore NetMail 'Sent' Attribute"
						,cfg.ignore_netmail_sent_attr ? "Yes" : "No");
					snprintf(opt[i++],MAX_OPLN-1,"%-40.40s%-3.3s","Ignore NetMail 'KillSent' Attribute"
						,cfg.ignore_netmail_kill_attr ? "Yes" : "No");
					snprintf(opt[i++],MAX_OPLN-1,"%-40.40s%-3.3s","Ignore NetMail 'Received' Attribute"
						,cfg.ignore_netmail_recv_attr ? "Yes" : "No");
					snprintf(opt[i++],MAX_OPLN-1,"%-40.40s%-3.3s","Ignore NetMail 'Local' Attribute"
						,cfg.ignore_netmail_local_attr ? "Yes" : "No");
					if(cfg.max_netmail_age)
						duration_to_vstr(cfg.max_netmail_age, str, sizeof(str));
					else
						strcpy(str, "None");
					snprintf(opt[i++],MAX_OPLN-1,"%-40.40s%s","Maximum Age of Imported NetMail"	, str);
					opt[i][0]=0;
					j=uifc.list(WIN_ACT|WIN_SAV,0,0,0,&netmail_opt,0,"NetMail Settings",opt);
					if(j==-1)
						break;
					switch(j) {
						case 0:
							uifc.helpbuf=
							"~ Sysop Aliases ~\n\n"
							"This is a comma-separated list of additional `To` names that the sysop\n"
							"(user #1) can receive netmail by. When specifying multiple aliases,\n"
							"they must be separated by a single comma and no extra white-space\n"
							"(e.g. \"SYSOP,COORDINATOR\"). The default value is just `SYSOP`.\n";
							if(uifc.input(WIN_MID|WIN_BOT|WIN_SAV,0,0,"Sysop Aliases (comma separated)"
								,sysop_aliases
								,sizeof(sysop_aliases)-1,K_EDIT|K_UPPER) >= 0) {
								strListFree(&cfg.sysop_alias_list);
								cfg.sysop_alias_list = strListSplit(NULL, sysop_aliases, ",");
							}
							break;
						case 1:
							uifc.input(WIN_MID|WIN_SAV,0,0,"Default Recipient"
								,cfg.default_recipient, sizeof(cfg.default_recipient)-1
								,K_EDIT|K_UPPER);
							break;
						case 2:
							k = !cfg.fuzzy_zone;
							switch(uifc.list(WIN_MID|WIN_SAV,0,0,0,&k,0
								,"Fuzzy Zone Operation",uifcYesNoOpts)) {
								case 0:	cfg.fuzzy_zone = true;	break;
								case 1:	cfg.fuzzy_zone = false;	break;
							}
							break;
						case 3:
							k = !cfg.kill_empty_netmail;
							switch(uifc.list(WIN_MID|WIN_SAV,0,0,0,&k,0
								,"Delete Empty NetMail",uifcYesNoOpts)) {
								case 0:	cfg.kill_empty_netmail = true;	break;
								case 1:	cfg.kill_empty_netmail = false;	break;
							}
							break;
						case 4:
							k = !cfg.delete_netmail;
							switch(uifc.list(WIN_MID|WIN_SAV,0,0,0,&k,0
								,"Delete Processed NetMail",uifcYesNoOpts)) {
								case 0:	cfg.delete_netmail = true;	break;
								case 1:	cfg.delete_netmail = false;	break;
							}
							break;
						case 5:
							k = !cfg.ignore_netmail_dest_addr;
							switch(uifc.list(WIN_MID|WIN_SAV,0,0,0,&k,0
								,"Ignore NetMail Destination Address",uifcYesNoOpts)) {
								case 0:	cfg.ignore_netmail_dest_addr = true;	break;
								case 1:	cfg.ignore_netmail_dest_addr = false;	break;
							}
							break;
						case 6:
							k = !cfg.ignore_netmail_sent_attr;
							switch(uifc.list(WIN_MID|WIN_SAV,0,0,0,&k,0
								,"Ignore NetMail 'Sent' Attribute",uifcYesNoOpts)) {
								case 0:	cfg.ignore_netmail_sent_attr = true;	break;
								case 1:	cfg.ignore_netmail_sent_attr = false;	break;
							}
							break;
						case 7:
							k = !cfg.ignore_netmail_kill_attr;
							switch(uifc.list(WIN_MID|WIN_SAV,0,0,0,&k,0
								,"Ignore NetMail 'KillSent' Attribute",uifcYesNoOpts)) {
								case 0:	cfg.ignore_netmail_kill_attr = true;	break;
								case 1:	cfg.ignore_netmail_kill_attr = false;	break;
							}
							break;
						case 8:
							k = !cfg.ignore_netmail_recv_attr;
							switch(uifc.list(WIN_MID|WIN_SAV,0,0,0,&k,0
								,"Ignore NetMail 'Received' Attribute",uifcYesNoOpts)) {
								case 0:	cfg.ignore_netmail_recv_attr = true;	break;
								case 1:	cfg.ignore_netmail_recv_attr = false;	break;
							}
							break;
						case 9:
							k = !cfg.ignore_netmail_local_attr;
							switch(uifc.list(WIN_MID|WIN_SAV,0,0,0,&k,0
								,"Ignore NetMail 'Local' Attribute",uifcYesNoOpts)) {
								case 0:	cfg.ignore_netmail_local_attr = true;	break;
								case 1:	cfg.ignore_netmail_local_attr = false;	break;
							}
							break;
						case 10:
							uifc.helpbuf=
							"~ Maximum Age of Imported NetMail ~\n\n"
							"Maximum age of NetMail that may be imported. The age is based\n"
							"on the date supplied in the message header and may be incorrect in some\n"
							"conditions (e.g. erroneous software or incorrect system date).\n"
							"Set this value to `0` to disable this feature (no maximum age imposed)."
							;
							if(cfg.max_netmail_age)
								duration_to_vstr(cfg.max_netmail_age, str, sizeof(str));
							else
								strcpy(str, "None");
							if(uifc.input(WIN_MID|WIN_BOT|WIN_SAV,0,0,"Maximum NetMail Age"
								,str, 10, K_EDIT) >= 0)
								cfg.max_netmail_age = (ulong)parse_duration(str);
							break;

					} 
				}
				break;

			case 4:	/* EchoMail Settings */
				j=0;
				while(1) {
					uifc.helpbuf=
	"~ EchoMail Settings ~\n"
	"\n"
	"`Area Manager` is the BBS user name or alias to notify (via email) of\n"
	"    AreaFix activities and errors.  This setting defaults to `SYSOP`.\n"
	"\n"
	"`Maximum Packet Size` is the largest packet file size that SBBSecho will\n"
	"    normally create (in bytes).\n"
	"    This settings defaults to `250K` (250 Kilobytes, or 256,000 bytes).\n"
	"\n"
	"`Maximum Bundle Size` is the largest bundle file size that SBBSecho will\n"
	"    normally create (in bytes).\n"
	"    This settings defaults to `250K` (250 Kilobytes, or 256,000 bytes).\n"
	"\n"
	"`Secure Operation` tells SBBSecho to check the Area File (e.g. areas.bbs)\n"
	"    to insure that the packet origin (FTN address) of EchoMail messages\n"
	"    is already linked to the EchoMail area where the message was posted.\n"
	"    This setting defaults to `No`.\n"
	"\n"
	"`Notify Users of Received EchoMail` tells SBBSecho to send telegrams\n"
	"    (short messages) to BBS users when EchoMail addressed to their name\n"
	"    or alias has been imported into a message base that the user has\n"
	"    access to read.\n"
	"\n"
	"`Convert Existing Tear Lines` tells SBBSecho to convert any tear lines\n"
	"    (`---`) existing in the message text to `===`.\n"
	"    This setting defaults to `No`.\n"
	"\n"
	"`Strip Line Feeds From Outgoing Messages` when set to `Yes` instructs\n"
	"    SBBSecho to remove any line-feed (ASCII 10) characters from the body\n"
	"    text of messages being exported to FidoNet EchoMail.\n"
	"    This setting defaults to `No`.\n"
	"\n"
	"`Automatically Add New Subs to Area File`, when set to `Yes`, enables\n"
	"    SBBSecho to detect newly added Sub-boards in any Message Groups that\n"
	"    are listed with a `Linked Node` as their hub/uplink and add those\n"
	"    Sub-boards as new areas in your Area File.\n"
	"\n"
	"`Allow Nodes to Add Areas from Area File` when set to `Yes` allows linked\n"
	"    nodes to add areas listed in your Area File (e.g. `areas.bbs`).\n"
	"    When set to `No`, only areas found in one or more `EchoLists` may be\n"
	"    added via AreaFix, provided the linked node has access.\n"
	"    This setting defaults to `Yes`.\n"
	"\n"
	"`Maximum Backups to Maintain of Area File` defines the number of backup\n"
	"    versions SBBSecho will maintain of your Area File (e.g. `areas.bbs`).\n"
	"\n"
	"`Circular Path Detection` when `Enabled` will cause SBBSecho, during\n"
	"    EchoMail import, to check the PATH kludge lines for any of the\n"
	"    system's AKAs and if found (indicating a message loop), not import\n"
	"    the message.\n"
	"\n"
	"`Relay Filtered Messages` controls whether or not incoming messages that\n"
	"    have been filtered (e.g. due to maximum message age restrictions)\n"
	"    are to be forwarded to downlinks.\n"
	"\n"
	"`Outbound Bundle Attachments` may be either `Deleted` (killed) or `Truncated`\n"
	"    (changed to 0-bytes in length) after being sent by your mailer.\n"
	"    This only controls the bundle prefix that is written to the FLO\n"
	"    files (`#` for truncate, `^` for delete).\n"
	"\n"
	"`Zone Blind SEEN-BY and PATH Lines` when `Enabled` will cause SBBSecho\n"
	"    to assume that node numbers are not duplicated across zones and\n"
	"    that a net/node combination in either of these Kludge lines should\n"
	"    be used to identify a specific node regardless of which zone that\n"
	"    node is located (thus breaking the rules of FidoNet 3D addressing).\n"
	"\n"
	"`Maximum Age of Imported EchoMail` allows you to optionally set an age\n"
	"    limit of EchoMail messages that may be imported.\n"
	"    This setting defaults to `60 days`.\n"
	;

					i=0;
					snprintf(opt[i++],MAX_OPLN-1,"%-45.45s%s", "Area Manager",cfg.areamgr);
					snprintf(opt[i++],MAX_OPLN-1,"%-45.45s%luK","Maximum Packet Size"
						,cfg.maxpktsize/1024UL);
					snprintf(opt[i++],MAX_OPLN-1,"%-45.45s%luK","Maximum Bundle Size"
						,cfg.maxbdlsize/1024UL);
					snprintf(opt[i++],MAX_OPLN-1,"%-45.45s%-3.3s","Secure Operation"
						,cfg.secure_echomail ? "Yes":"No");
					snprintf(opt[i++],MAX_OPLN-1,"%-45.45s%-3.3s","Notify Users of Received EchoMail"
						,cfg.echomail_notify ? "Yes":"No");
					snprintf(opt[i++],MAX_OPLN-1,"%-45.45s%-3.3s","Convert Existing Tear Lines"
						,cfg.convert_tear ? "Yes":"No");
					snprintf(opt[i++],MAX_OPLN-1,"%-45.45s%-3.3s","Strip Line Feeds "
						"from Outgoing Messages",cfg.strip_lf ? "Yes":"No");
					snprintf(opt[i++],MAX_OPLN-1,"%-45.45s%-3.3s","Automatically Add New Subs "
						"to Area File",cfg.auto_add_subs ? "Yes":"No");
					snprintf(opt[i++],MAX_OPLN-1,"%-45.45s%-3.3s","Allow Nodes to Add Areas "
						"from Area File",cfg.add_from_echolists_only ? "No":"Yes");
					snprintf(opt[i++],MAX_OPLN-1,"%-45.45s%u","Maximum Backups to Maintain of Area File"
						,cfg.areafile_backups);
					snprintf(opt[i++],MAX_OPLN-1,"%-45.45s%s","Circular Path Detection"
						,cfg.check_path ? "Enabled" : "Disabled");
					snprintf(opt[i++],MAX_OPLN-1,"%-45.45s%s","Relay Filtered Messages"
						,cfg.relay_filtered_msgs ? "Yes" : "No");
					snprintf(opt[i++],MAX_OPLN-1,"%-45.45s%s","Outbound Bundle Attachments"
						,cfg.trunc_bundles ? "Truncate" : "Delete");
					if(cfg.zone_blind)
						sprintf(str,"Zones 1-%u", cfg.zone_blind_threshold);
					else
						strcpy(str,"Disabled");
					snprintf(opt[i++],MAX_OPLN-1,"%-45.45s%s","Zone Blind SEEN-BY and PATH Lines", str);
					if(cfg.max_echomail_age)
						duration_to_vstr(cfg.max_echomail_age, str, sizeof(str));
					else
						strcpy(str, "None");
					snprintf(opt[i++],MAX_OPLN-1,"%-45.45s%s","Maximum Age of Imported EchoMail", str);
					opt[i][0]=0;
					j=uifc.list(WIN_ACT|WIN_MID|WIN_SAV,0,0,0,&echomail_opt,0,"EchoMail Settings",opt);
					if(j==-1)
						break;
					switch(j) {
						case 0:
				uifc.helpbuf=
				"~ Area Manager ~\n\n"
				"Local user to notify of AreaFix activity and errors.\n";
							uifc.input(WIN_MID|WIN_BOT|WIN_SAV,0,0,"Area Manager (user name or alias)"
								,cfg.areamgr
								,LEN_ALIAS,K_EDIT);
							break;

						case 1:
				uifc.helpbuf=
				"~ Maximum Packet Size ~\n\n"
				"This is the maximum file size that SBBSecho will create when placing\n"
				"outgoing messages into packets.  The default max size is 250 Kilobytes.\n";
							sprintf(str,"%lu",cfg.maxpktsize);
							uifc.input(WIN_MID|WIN_BOT|WIN_SAV,0,0,"Maximum Packet Size (in Bytes)",str
								,9,K_EDIT|K_NUMBER);
							cfg.maxpktsize=atol(str);
							break;

						case 2:
				uifc.helpbuf=
				"~ Maximum Bundle Size ~\n\n"
				"This is the maximum file size that SBBSecho will create when placing\n"
				"outgoing packets into bundles.  The default max size is 250 Kilobytes.\n";
							sprintf(str,"%lu",cfg.maxbdlsize);
							uifc.input(WIN_MID|WIN_BOT|WIN_SAV,0,0,"Maximum Bundle Size (in Bytes)",str
								,9,K_EDIT|K_NUMBER);
							cfg.maxbdlsize=atol(str);
							break;
						case 3:
							k = !cfg.secure_echomail;
							switch(uifc.list(WIN_MID|WIN_SAV,0,0,0,&k,0
								,"Secure Operation",uifcYesNoOpts)) {
								case 0:	cfg.secure_echomail = true;		break;
								case 1:	cfg.secure_echomail = false;	break;
							}
							break;
						case 4:
							k = !cfg.echomail_notify;
							switch(uifc.list(WIN_MID|WIN_SAV,0,0,0,&k,0
								,"Notify Users",uifcYesNoOpts)) {
								case 0:	cfg.echomail_notify = true;		break;
								case 1:	cfg.echomail_notify = false;	break;
							}
							break;
						case 5:
							k = !cfg.convert_tear;
							switch(uifc.list(WIN_MID|WIN_SAV,0,0,0,&k,0
								,"Convert Tear Lines",uifcYesNoOpts)) {
								case 0:	cfg.convert_tear = true;	break;
								case 1:	cfg.convert_tear = false;	break;
							}
							break;
						case 6:
							k = !cfg.strip_lf;
							switch(uifc.list(WIN_MID|WIN_SAV,0,0,0,&k,0
								,"Strip Line Feeds",uifcYesNoOpts)) {
								case 0:	cfg.strip_lf = true;	break;
								case 1:	cfg.strip_lf = false;	break;
							}
							break;
						case 7:
							k = !cfg.auto_add_subs;
							switch(uifc.list(WIN_MID|WIN_SAV,0,0,0,&k,0
								,"Automatically Add New Sub-boards to Area File",uifcYesNoOpts)) {
								case 0:	cfg.auto_add_subs = true;	break;
								case 1:	cfg.auto_add_subs = false;	break;
							}
							break;
						case 8:
							k = cfg.add_from_echolists_only;
							switch(uifc.list(WIN_MID|WIN_SAV,0,0,0,&k,0
								,"Allow AreaFix-Add from Area File",uifcYesNoOpts)) {
								case 0:	cfg.add_from_echolists_only = false;	break;
								case 1:	cfg.add_from_echolists_only = true;		break;
							}
							break;
						case 9:
							sprintf(str, "%u", cfg.areafile_backups);
							if(uifc.input(WIN_MID|WIN_SAV, 0, 0, "Total Area File Backups to Maintain"
								,str, 5, K_EDIT|K_NUMBER) >= 0)
								cfg.areafile_backups = atoi(str);
							break;
						case 10:
							k = !cfg.check_path;
							switch(uifc.list(WIN_MID|WIN_SAV,0,0,0,&k,0
								,"Circular Path Detection",uifcYesNoOpts)) {
								case 0:	cfg.check_path = true;	break;
								case 1:	cfg.check_path = false;	break;
							}
							break;
						case 11:
							k = !cfg.check_path;
							switch(uifc.list(WIN_MID|WIN_SAV,0,0,0,&k,0
								,"Relay Filtered EchoMail Messages",uifcYesNoOpts)) {
								case 0:	cfg.relay_filtered_msgs = true;		break;
								case 1:	cfg.relay_filtered_msgs = false;	break;
							}
							break;
						case 12:
						{
							k = cfg.trunc_bundles;
							char* opt[] = {"Delete after Sent", "Truncate after Sent", NULL };
							switch(uifc.list(WIN_MID|WIN_SAV,0,0,0,&k,0
								,"Outbound Bundles",opt)) {
								case 0:	cfg.trunc_bundles = false;	break;
								case 1:	cfg.trunc_bundles = true;	break;
							}
							break;
						}
						case 13:
							k = !cfg.zone_blind;
							switch(uifc.list(WIN_MID|WIN_SAV,0,0,0,&k,0,"Zone Blind",uifcYesNoOpts)) {
								case 0:
									cfg.zone_blind = true;
									uifc.helpbuf=
									"Zone Blind Threshold";
									sprintf(str,"%u",cfg.zone_blind_threshold);
									if(uifc.input(WIN_MID|WIN_SAV,0,0
										,"Zone Blind Threshold (highest zone in the blind range)"
										, str, 5, K_EDIT|K_NUMBER) >= 0)
										cfg.zone_blind_threshold = (uint16_t)atol(str);
									break;
								case 1:
									cfg.zone_blind = false;
									break;
							}
							break;
						case 14:
							uifc.helpbuf=
							"~ Maximum Age of Imported EchoMail ~\n\n"
							"Maximum age of EchoMail that may be imported. The age is based\n"
							"on the date supplied in the message header and may be incorrect in some\n"
							"conditions (e.g. erroneous software or incorrect system date).\n"
							"Set this value to `0` to disable this feature (no maximum age imposed)."
							;
							if(cfg.max_echomail_age)
								duration_to_vstr(cfg.max_echomail_age, str, sizeof(str));
							else
								strcpy(str, "None");
							if(uifc.input(WIN_MID|WIN_BOT|WIN_SAV,0,0,"Maximum EchoMail Age"
								,str, 10, K_EDIT) >= 0)
								cfg.max_echomail_age = (ulong)parse_duration(str);
							break;

					} 
				}
				break;

			case 2:	/* Archive Types */
				i=0;
				while(1) {
					uifc.helpbuf=
	"~ Archive Types ~\n\n"
	"These are the archive file types that have been configured along with\n"
	"their corresponding archive programs and command-lines for the packing\n"
	"and unpacking of EchoMail bundle files.\n"
	"\n"
	"The corresponding archive programs are sometimes referred to as `packers`."
	;
					for(u=0;u<cfg.arcdefs;u++)
						snprintf(opt[u],MAX_OPLN-1,"%-30.30s",cfg.arcdef[u].name);
					opt[u][0]=0;
					int mode = WIN_SAV | WIN_INS | WIN_DEL | WIN_ACT
						| WIN_INSACT | WIN_DELACT | WIN_XTR;
					if(cfg.arcdefs)
						mode |= WIN_COPY | WIN_CUT;
					if(savarcdef.name[0])
						mode |= WIN_PASTE | WIN_PASTEXTR;
					i=uifc.list(mode,0,0,0,&archive_opt,0,"Archive Types",opt);
					if(i==-1)
						break;
					int msk = i & MSK_ON;
					i &= MSK_OFF;
					if (msk == MSK_INS) {
						str[0]=0;
	uifc.helpbuf=
	"~ Archive Type ~\n\n"
	"This is the identifying name of the archiving program (packer).\n";
						if(uifc.input(WIN_MID|WIN_SAV,0,0
							,"Archive Type",str,25,K_EDIT|K_UPPER)<1)
							continue;
						if(!new_arcdef(i)) {
							printf("\nMemory Allocation Error\n");
							exit(1); 
						}
						strcpy(cfg.arcdef[i].name,str);
						continue; 
					}

					if (msk == MSK_DEL || msk == MSK_CUT) {
						if (msk == MSK_CUT)
							memcpy(&savarcdef, &cfg.arcdef[i], sizeof(arcdef_t));
						cfg.arcdefs--;
						if(cfg.arcdefs<=0) {
							cfg.arcdefs=0;
							continue; 
						}
						for(u=i;u<cfg.arcdefs;u++)
							memcpy(&cfg.arcdef[u],&cfg.arcdef[u+1]
								,sizeof(arcdef_t));
						continue; 
					}
					if (msk == MSK_COPY) {
						memcpy(&savarcdef,&cfg.arcdef[i],sizeof(arcdef_t));
						continue; 
					}
					if (msk == MSK_PASTE) {
						if(!new_arcdef(i))
							continue;
						memcpy(&cfg.arcdef[i],&savarcdef,sizeof(arcdef_t));
						continue; 
					}
					if (msk != 0)
						continue;
					while(1) {
						uifc.helpbuf=
	"Archive Type and Program configuration";
						j=0;
						snprintf(opt[j++],MAX_OPLN-1,"%-30.30s %s","Archive Type"
							,cfg.arcdef[i].name);
						snprintf(opt[j++],MAX_OPLN-1,"%-30.30s %s","Signature"
							,cfg.arcdef[i].hexid);
						snprintf(opt[j++],MAX_OPLN-1,"%-30.30s %u","Signature Offset"
							,cfg.arcdef[i].byteloc);
						snprintf(opt[j++],MAX_OPLN-1,"%-30.30s %s","Pack Command Line"
							,cfg.arcdef[i].pack);
						snprintf(opt[j++],MAX_OPLN-1,"%-30.30s %s","Unpack Command Line"
							,cfg.arcdef[i].unpack);
						opt[j][0]=0;
						SAFEPRINTF(str,"Archive Type - %s", cfg.arcdef[i].name);
						k=uifc.list(WIN_ACT|WIN_SAV|WIN_RHT|WIN_BOT,0,0,0,&packop,0,str,opt);
						if(k==-1)
							break;
						switch(k) {
							case 0:
								uifc.helpbuf=
								"~ Archive Type ~\n\n"
								"This is the identifying name of the archive file type. Usually this name\n"
								"corresponds with the common file extension or suffix denoting this type\n"
								"of archive file (e.g. `zip`, `arc`, etc.)."
								;
								uifc.input(WIN_MID|WIN_SAV,0,0
									,"Archive Type"
									,cfg.arcdef[i].name,sizeof(cfg.arcdef[i].name)-1
									,K_EDIT|K_UPPER);
								break;
							case 1:
								uifc.helpbuf=
								"~ Archive Signature ~\n\n"
								"This is the identifying signature of the archive file format (in\n"
								"hexadecimal notation).  This signature is used in combination with the\n"
								"Archive `Signature Offset` for the automatic detection of proper archive\n"
								"program to extract (unarchive) inbound EchoMail bundle files."
								;
								uifc.input(WIN_MID|WIN_SAV,0,0
									,"Archive Signature"
									,cfg.arcdef[i].hexid,sizeof(cfg.arcdef[i].hexid)-1
									,K_EDIT|K_UPPER);
								break;
							case 2:
								uifc.helpbuf=
								"~ Archive Signature Offset ~\n\n"
								"This is the byte-offset of the identifying signature of the archive file\n"
								"format.  This offset is used in combination with the Archive `Signature`\n"
								"for the automatic detection of proper archive program to extract\n"
								"(unarchive) inbound EchoMail bundle files."
								;
								sprintf(str,"%u",cfg.arcdef[i].byteloc);
								if(uifc.input(WIN_MID|WIN_SAV,0,0
									,"Archive Signature Offset",str,5
									,K_NUMBER|K_EDIT) > 0)
									cfg.arcdef[i].byteloc=atoi(str);
								break;
							case 3:
								uifc.helpbuf=
								"~ Pack Command Line ~\n\n"
								"This is the command-line to execute to create an archive file of the\n"
								"selected type.  The following command-line specifiers may be used for\n"
								"dynamic variable replacement:\n"
								"\n"
								"  `%f`  The path/filename of the archive file to be created\n"
								"  `%s`  The path/filename of the file(s) to be added to the archive\n"
								"  `%!`  The Synchronet `exec` directory\n"
								"  `%@`  The Synchronet `exec` directory only for non-Unix systems\n"
								"  `%.`  Blank for Unix systems, '`.exe`' otherwise\n"
								"  `%?`  The current platform description (e.g. 'linux', 'win32')\n"
								"  `%j`  The Synchronet `data` directory\n"
								"  `%k`  The Synchronet `ctrl` directory\n"
								"  `%o`  The configured system operator name\n"
								"  `%q`  The configured system QWK-ID\n"
								"  `%g`  The configured temporary file directory\n"
								;
								uifc.input(WIN_MID|WIN_SAV,0,0
									,"Pack Command Line"
									,cfg.arcdef[i].pack,sizeof(cfg.arcdef[i].pack)-1
									,K_EDIT);
								break;
							case 4:
								uifc.helpbuf=
								"~ Unpack Command Line ~\n\n"
								"This is the command-line to execute to extract an archive file of the\n"
								"selected type.  The following command-line specifiers may be used for\n"
								"dynamic variable replacement:\n"
								"\n"
								"  `%f`  The path/filename of the archive file to be extracted\n"
								"  `%s`  The destination directory for files extracted from the archive\n"
								"  `%!`  The Synchronet `exec` directory\n"
								"  `%@`  The Synchronet `exec` directory only for non-Unix systems\n"
								"  `%.`  Blank for Unix systems, '`.exe`' otherwise\n"
								"  `%?`  The current platform description (e.g. 'linux', 'win32')\n"
								"  `%j`  The Synchronet `data` directory\n"
								"  `%k`  The Synchronet `ctrl` directory\n"
								"  `%o`  The configured system operator name\n"
								"  `%q`  The configured system QWK-ID\n"
								"  `%g`  The configured temporary file directory\n"
								;
								uifc.input(WIN_MID|WIN_SAV,0,0
									,"Unpack Command Line"
									,cfg.arcdef[i].unpack,sizeof(cfg.arcdef[i].unpack)-1
									,K_EDIT);
								break;
						} 
					} 
				}
				break;

			case 6:
	uifc.helpbuf=
	"~ EchoLists ~\n\n"
	"This feature allows you to specify lists of echoes, in `BACKBONE.NA`\n"
	"format, which are utilized in `addition` to your Area File (e.g. \n"
	"`areas.bbs`) for advanced AreaFix (Area Management) operations.\n";
				i=0;
				while(1) {
					for(u=0;u<cfg.listcfgs;u++)
						snprintf(opt[u],MAX_OPLN-1,"%s",cfg.listcfg[u].listpath);
					opt[u][0]=0;
					int mode = WIN_SAV | WIN_INS | WIN_DEL | WIN_ACT 
						| WIN_INSACT | WIN_DELACT | WIN_XTR;
					if(cfg.listcfgs)
						mode |= WIN_COPY | WIN_CUT;
					if(savlistcfg.listpath[0])
						mode |= WIN_PASTE | WIN_PASTEXTR;
					i=uifc.list(mode,0,0,0,&echolist_opt,0,"EchoLists",opt);
					if(i==-1)
						break;
					int msk = i&MSK_ON;
					i &= MSK_OFF;
					if (msk == MSK_INS) {
						str[0]=0;
	uifc.helpbuf=
	"~ EchoList ~\n\n"
	"This is the path and filename of the echolist file you wish\n"
	"to add.\n";
						if(uifc.input(WIN_MID|WIN_SAV,0,0
							,"EchoList Path/Name",str,50,K_EDIT)<1)
							continue;
						if(!new_list(i)) {
							printf("\nMemory Allocation Error\n");
							exit(1); 
						}
						strcpy(cfg.listcfg[i].listpath,str);
						continue; 
					}

					if (msk == MSK_DEL || msk == MSK_CUT) {
						if(msk == MSK_CUT)
							memcpy(&savlistcfg, &cfg.listcfg[i], sizeof(echolist_t));
						cfg.listcfgs--;
						if(cfg.listcfgs<=0) {
							cfg.listcfgs=0;
							continue; 
						}
						for(u=i;u<cfg.listcfgs;u++)
							memcpy(&cfg.listcfg[u],&cfg.listcfg[u+1]
								,sizeof(echolist_t));
						continue; 
					}
					if (msk == MSK_COPY) {
						memcpy(&savlistcfg,&cfg.listcfg[i],sizeof(echolist_t));
						continue; 
					}
					if (msk == MSK_PASTE) {
						if(!new_list(i))
							continue;
						memcpy(&cfg.listcfg[i],&savlistcfg,sizeof(echolist_t));
						continue; 
					}
					if (msk != 0)
						continue;
					while(1) {
						j=0;
						uifc.helpbuf=
						"Configuring an EchoList"
						;
						snprintf(opt[j++],MAX_OPLN-1,"%-30.30s %s","EchoList Path/Name"
							,cfg.listcfg[i].listpath);
						snprintf(opt[j++],MAX_OPLN-1,"%-30.30s %s","Required Key"
							,strListCombine(cfg.listcfg[i].keys, str, sizeof(str), ","));
						snprintf(opt[j++],MAX_OPLN-1,"%-30.30s %s","Hub Address"
							,(cfg.listcfg[i].hub.zone) ?
							 faddrtoa(&cfg.listcfg[i].hub) : "None");
						snprintf(opt[j++],MAX_OPLN-1,"%-30.30s %s","Forward AreaMgr Requests"
							,cfg.listcfg[i].forward ? "Yes" : "No");
						if(cfg.listcfg[i].forward) {
							snprintf(opt[j++],MAX_OPLN-1,"%-30.30s %s","AreaMgr Name"
								,cfg.listcfg[i].areamgr);
							snprintf(opt[j++],MAX_OPLN-1,"%-30.30s %s","AreaMgr Password"
								,cfg.listcfg[i].password);
						}
						opt[j][0]=0;
						SAFEPRINTF(str, "EchoList - %s", getfname(cfg.listcfg[i].listpath));
						k=uifc.list(WIN_ACT|WIN_SAV|WIN_RHT|WIN_BOT,0,0,0,&listop,0,str,opt);
						if(k==-1)
							break;
						switch(k) {
							case 0:
								uifc.input(WIN_MID|WIN_SAV,0,0
									,"EchoList Path/Name"
									,cfg.listcfg[i].listpath,sizeof(cfg.listcfg[i].listpath)-1
									,K_EDIT);
								break;
							case 1:
								uifc.helpbuf=
								"~ Required Key ~\n\n"
								"These keys determine which linked nodes have access to the selected\n"
								"echolist file via AreaFix requests (e.g. query, add, remove).\n"
								"\n"
								"A linked node need only have one of the required keys to have access\n"
								"the echolist."
								;
								while(1) {
									for(u=0; cfg.listcfg[i].keys!=NULL && cfg.listcfg[i].keys[u] != NULL;u++)
										strcpy(opt[u],cfg.listcfg[i].keys[u]);
									opt[u][0]=0;
									x=uifc.list(WIN_SAV|WIN_INS|WIN_DEL|WIN_ACT|
										WIN_XTR|WIN_INSACT|WIN_DELACT|WIN_RHT
										,0,0,0,&x,0,"Required Key (any one of)",opt);
									if(x==-1)
										break;
									if((x&MSK_ON)==MSK_INS) {
										x&=MSK_OFF;
										str[0]=0;
										if(uifc.input(WIN_MID|WIN_SAV,0,0
											,"EchoList Key",str,SBBSECHO_MAX_KEY_LEN
											,K_EDIT|K_UPPER)<1)
											continue;
										strListInsert(&cfg.listcfg[i].keys,str,x);
										continue; 
									}

									if((x&MSK_ON)==MSK_DEL) {
										x&=MSK_OFF;
										strListRemove(&cfg.listcfg[i].keys,x);
										continue; 
									}
									SAFECOPY(str,cfg.listcfg[i].keys[x]);
										uifc.input(WIN_MID|WIN_SAV,0,0,"EchoList Keys"
											,str,SBBSECHO_MAX_KEY_LEN,K_EDIT|K_UPPER);
										strListReplace(cfg.listcfg[i].keys,x,str);
										continue; 
								}
								break;
							case 2:
								if(cfg.listcfg[i].hub.zone)
									strcpy(str,faddrtoa(&cfg.listcfg[i].hub));
								else
									str[0]=0;
								uifc.input(WIN_MID|WIN_SAV,0,0
									,"Hub Address",str
									,25,K_EDIT);
								if(str[0])
									cfg.listcfg[i].hub=atofaddr(str);
								else
									memset(&cfg.listcfg[i].hub,0
										,sizeof(faddr_t));
								break;
							case 3:
								cfg.listcfg[i].forward = !cfg.listcfg[i].forward;
								break;
							case 4:
								uifc.input(WIN_MID|WIN_SAV,0,0
									,"Name to use when forwarding AreaMgr requests"
									,cfg.listcfg[i].areamgr,sizeof(cfg.listcfg[i].areamgr)-1
									,K_EDIT);
								break;
							case 5:
								uifc.input(WIN_MID|WIN_SAV,0,0
									,"Password to use when forwarding AreaMgr requests"
									,cfg.listcfg[i].password,sizeof(cfg.listcfg[i].password)-1
									,K_EDIT|K_UPPER);
								break;
						} 
					} 
				}
				break;

			case 7:
				if(!sbbsecho_write_ini(&cfg))
					uifc.msg("Error saving configuration file");
				else {
					orig_cfg = cfg;
					uifc.changes = FALSE;
				}
				break;
			case -1:
				if(uifc.changes) {
		uifc.helpbuf=
		"~ Save Configuration File ~\n\n"
		"Select `Yes` to save the config file, `No` to quit without saving,\n"
		"or hit ~ ESC ~ to go back to the menu.\n\n";
					i=0;
					strcpy(opt[0],"Yes");
					strcpy(opt[1],"No");
					opt[2][0]=0;
					i=uifc.list(WIN_MID,0,0,0,&i,0,"Save Config File",opt);
					if(i==-1) break;
					if(i) {uifc.bail(); exit(0);}
					if(!sbbsecho_write_ini(&cfg))
						uifc.msg("Error saving configuration file");
				}
				uifc.bail();
				exit(0);
		}
	}
}
