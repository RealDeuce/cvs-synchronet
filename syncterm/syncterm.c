#include <sys/stat.h>

#include <gen_defs.h>
#include <stdlib.h>
#include <ciolib.h>

#include <ini_file.h>
#include <dirwrap.h>

#include "bbslist.h"
#include "conn.h"
#include "term.h"
#include "uifcinit.h"
#include "window.h"

#ifdef _WINSOCKAPI_

static WSADATA WSAData;
#define SOCKLIB_DESC WSAData.szDescription
static BOOL WSAInitialized=FALSE;

static BOOL winsock_startup(void)
{
	int		status;             /* Status Code */

    if((status = WSAStartup(MAKEWORD(1,1), &WSAData))==0) {
		fprintf(stderr,"%s %s",WSAData.szDescription, WSAData.szSystemStatus);
		WSAInitialized=TRUE;
		return (TRUE);
	}

    fprintf(stderr,"!WinSock startup ERROR %d", status);
	return (FALSE);
}

#else /* No WINSOCK */

#define winsock_startup()	(TRUE)
#define SOCKLIB_DESC NULL

#endif

int main(int argc, char **argv)
{
	struct bbslist *bbs=NULL;
	struct	text_info txtinfo;
	char	str[MAX_PATH];
	char	drive[MAX_PATH];
	char	path[MAX_PATH];
	char	fname[MAX_PATH];
	char	ext[MAX_PATH];
	/* Command-line parsing vars */
	char	url[MAX_PATH];
	char	*p1;
	char	*p2;
	char	*p3;
	int		i;
	int	ciolib_mode=CIOLIB_MODE_AUTO;
	struct	bbslist	*list[MAX_OPTS+1];
	int		listcount=0;
	str_list_t	inifile;
	FILE *listfile;
	char	listpath[MAX_PATH+1];
	char	*home;

	/* UIFC initialization */
    memset(&uifc,0,sizeof(uifc));
	uifc.mode=UIFC_NOCTRL;
	uifc.size=sizeof(uifc);
	uifc.esc_delay=25;
	url[0]=0;
	for(i=1;i<argc;i++) {
        if(argv[i][0]=='-'
#ifndef __unix__
            || argv[i][0]=='/'
#endif
            )
            switch(toupper(argv[i][1])) {
		        case 'M':   /* Monochrome mode */
        			uifc.mode|=UIFC_MONO;
                    break;
                case 'C':
        			uifc.mode|=UIFC_COLOR;
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
							printf("NOTICE: The -i option is depreciated, use -if instead\r\n");
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
						default:
							goto USAGE;
					}
					break;
                case 'V':
                    textmode(atoi(argv[i]+2));
                    break;
                default:
					goto USAGE;
           }
        else
			SAFECOPY(url,argv[i]);
    }

	initciolib(ciolib_mode);

    gettextinfo(&txtinfo);
	if((txtinfo.screenwidth<40) || txtinfo.screenheight<24) {
		fputs("Window too small, must be at least 80x24\n",stderr);
		return(1);
	}

	_splitpath(argv[0],drive,path,fname,ext);
	strcat(drive,path);
	FULLPATH(path,drive,sizeof(path));
	atexit(uifcbail);

#ifdef __unix__
	umask(077);
#endif

	/* User BBS list path */
	home=getenv("HOME");
	if(home==NULL)
		home=getenv("USERPROFILE");
	if(home==NULL)
		strcpy(listpath,path);
	else
		strcpy(listpath,home);
	strncat(listpath,"/syncterm.lst",sizeof(listpath));
	if(strlen(listpath)>MAX_PATH) {
		fprintf(stderr,"Path to syncterm.lst too long");
		return(0);
	}

	/* Auto-connect URL */
	if(url[0]) {
		if((bbs=(struct bbslist *)malloc(sizeof(struct bbslist)))==NULL) {
			uifcmsg("Unable to allocate memory","The system was unable to allocate memory.");
			return(1);
		}
		bbs->id=-1;
		bbs->added=time(NULL);
		bbs->calls=0;
		bbs->user[0]=0;
		bbs->password[0]=0;
		bbs->type=USER_BBSLIST;
		bbs->reversed=FALSE;
		bbs->screen_mode=SCREEN_MODE_CURRENT;
		if(!strnicmp("rlogin://",url,9)) {
			bbs->conn_type=CONN_TYPE_RLOGIN;
			bbs->port=513;
		}
		else if(!strnicmp("telnet://",url,9)) {
			bbs->conn_type=CONN_TYPE_TELNET;
			bbs->port=23;
		}
		else
			goto USAGE;
		p1=url+9;
		/* Remove trailing / (Win32 adds one 'cause it hates me) */
		p2=strchr(p1,'/');
		if(p2!=NULL)
			*p2=0;
		p3=strchr(p1,'@');
		if(p3!=NULL) {
			*p3=0;
			p2=strchr(p1,':');
			if(p2!=NULL) {
				*p2=0;
				p2++;
				SAFECOPY(bbs->password,p2);
			}
			SAFECOPY(bbs->user,p1);
			p1=p3+1;
		}
		SAFECOPY(bbs->name,p1);
		p2=strchr(p1,':');
		if(p2!=NULL) {
			*p2=0;
			p2++;
			bbs->port=atoi(p2);
			if(bbs->port==0)
				goto USAGE;
		}
		SAFECOPY(bbs->addr,p1);
		
		/* Find BBS listing in users phone book */
		read_list(listpath, &list[0], &listcount, USER_BBSLIST);
		for(i=0;i<listcount;i++) {
			if((stricmp(bbs->addr,list[i]->addr)==0)
					&& (bbs->port==list[i]->port)
					&& (bbs->conn_type==list[i]->conn_type)
					&& (bbs->user[0]==0 || (stricmp(bbs->name,list[i]->name)==0))
					&& (bbs->password[0]==0 || (stricmp(bbs->password,list[i]->password)==0))) {
				memcpy(bbs,list[i],sizeof(struct bbslist));
				break;
			}
		}
		free_list(&list[0],listcount);
	}

	if(!winsock_startup())
		return(1);

	while(bbs!=NULL || (bbs=show_bbslist(BBSLIST_SELECT,path))!=NULL) {
		if(!conn_connect(bbs->addr,bbs->port,bbs->reversed?bbs->password:bbs->user,bbs->reversed?bbs->user:bbs->password,bbs->conn_type)) {
			/* ToDo: Update the entry with new lastconnected */
			/* ToDo: Disallow duplicate entries */

			bbs->connected=time(NULL);
			bbs->calls++;
			if(bbs->id != -1) {
				if((listfile=fopen(listpath,"r"))!=NULL) {
					inifile=iniReadFile(listfile);
					fclose(listfile);
					iniSetInteger(&inifile,bbs->name,"LastConnected",bbs->connected,NULL);
					iniSetInteger(&inifile,bbs->name,"TotalCalls",bbs->calls,NULL);
					if((listfile=fopen(listpath,"w"))!=NULL) {
						iniWriteFile(listfile,inifile);
						fclose(listfile);
					}
					strListFreeStrings(inifile);
				}
			}
			uifcbail();
			switch(bbs->screen_mode) {
				case SCREEN_MODE_80X25:
					textmode(C80);
					break;
				case SCREEN_MODE_80X28:
					textmode(C80X28);
					break;
				case SCREEN_MODE_80X43:
					textmode(C80X43);
					break;
				case SCREEN_MODE_80X50:
					textmode(C80X50);
					break;
				case SCREEN_MODE_80X60:
					textmode(C80X60);
					break;
			}
			sprintf(str,"SyncTERM - %s",bbs->name);
			settitle(str);
			term.nostatus=bbs->nostatus;
			if(drawwin())
				return(1);
			doterm(bbs);
			textmode(txtinfo.currmode);
			settitle("SyncTERM");
		}
		if(url[0]) {
			if(bbs->id==-1) {
				char	*YesNo[3]={"Yes","No",""};
				/* Started from the command-line with a URL */
				init_uifc();
				switch(uifc.list(WIN_MID|WIN_SAV,0,0,0,&i,NULL,"Save this BBS in directory?",YesNo)) {
					case 0:	/* Yes */
						add_bbs(path,bbs);
						break;
					default: /* ESC/No */
						break;
				}
			}
			free(bbs);
			bbs=NULL;
			break;
		}
		else
			bbs=NULL;
	}
	uifcbail();
#ifdef _WINSOCKAPI_
	if(WSAInitialized && WSACleanup()!=0) 
		fprintf(stderr,"!WSACleanup ERROR %d",ERROR_VALUE);
#endif
	return(0);

	USAGE:
	uifcbail();
	clrscr();
    cprintf("\nusage: syncterm [options] [URL]"
        "\n\noptions:\n\n"
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
        "-v# =  set video mode to # (default=auto)\n"
        "-l# =  set screen lines to # (default=auto-detect)\n"
		"\n"
		"URL format is: (rlogin|telnet)://[user[:password]@]domainname[:port]\n"
		"examples: rlogin://deuce:password@nix.synchro.net:5885\n"
		"          telnet://deuce@nix.synchro.net\n"
		"          telnet://nix.synchro.net\n\nPress any key to exit..."
        );
	getch();
	return(0);
}
