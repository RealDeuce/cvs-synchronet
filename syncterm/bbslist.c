#include <stdio.h>
#include <stdlib.h>

#include <dirwrap.h>
#include <ini_file.h>
#include <uifc.h>

#include "bbslist.h"
#include "uifcinit.h"

enum {
	 USER_BBSLIST
	,SYSTEM_BBSLIST
};

void sort_list(struct bbslist **list)  {
	struct bbslist *tmp;
	unsigned int	i,swapped=1;

	while(swapped) {
		swapped=0;
		for(i=1;list[i-1]->name[0] && list[i]->name[0];i++) {
			int	cmp=stricmp(list[i-1]->name,list[i]->name);
			if(cmp>0) {
				tmp=list[i];
				list[i]=list[i-1];
				list[i-1]=tmp;
				swapped=1;
			}
		}
	}
}

/*
 * Reads in a BBS list from listpath using *i as the counter into bbslist
 * first BBS read goes into list[i]
 */
void read_list(char *listpath, struct bbslist **list, int *i, int type)
{
	FILE	*listfile;
	char	*bbsname;
	str_list_t	bbses;

	if((listfile=fopen(listpath,"r"))!=NULL) {
		bbses=iniGetSectionList(listfile,NULL);
		while((bbsname=strListPop(&bbses))!=NULL) {
			if((list[*i]=(struct bbslist *)malloc(sizeof(struct bbslist)))==NULL)
				break;
			strcpy(list[*i]->name,bbsname);
			iniGetString(listfile,bbsname,"Address","",list[*i]->addr);
			list[*i]->port=iniGetShortInt(listfile,bbsname,"Port",513);
			list[*i]->added=iniGetInteger(listfile,bbsname,"Added",0);
			list[*i]->connected=iniGetInteger(listfile,bbsname,"LastConnected",0);
			list[*i]->calls=iniGetInteger(listfile,bbsname,"TotalCalls",0);
			iniGetString(listfile,bbsname,"UserName","",list[*i]->user);
			iniGetString(listfile,bbsname,"Password","",list[*i]->password);
			list[*i]->dumb=iniGetBool(listfile,bbsname,"BeDumb",0);
			list[*i]->reversed=iniGetBool(listfile,bbsname,"Reversed",0);
			list[*i]->type=type;
			list[*i]->id=*i;
			(*i)++;
		}
		fclose(listfile);
		strListFreeStrings(bbses);
	}

	/* Add terminator */
	list[*i]=(struct bbslist *)"";
}

int edit_list(struct bbslist *item,char *listpath)
{
	char	opt[8][80];
	char	*opts[8];
	int		changed=0;
	int		copt=0,i,j;
	char	str[6];
	FILE *listfile;
	str_list_t	inifile;
	char	tmp[LIST_NAME_MAX+1];

	for(i=0;i<8;i++)
		opts[i]=opt[i];
	if(item->type==SYSTEM_BBSLIST) {
		uifc.helpbuf=	"`Cannot edit system BBS list`\n\n"
						"SyncTERM supports system-wide and per-user lists.  You may only edit entries"
						"in your own personal list.\n"
						"\n"
						"The Be Dumb option can be used to connect to BBSs which support 'dumb' telnet";
		uifc.msg("Cannot edit system BBS list");
		return(0);
	}
	opt[7][0]=0;
	if((listfile=fopen(listpath,"r"))!=NULL) {
		inifile=iniReadFile(listfile);
		fclose(listfile);
	}
	else
		return(0);
	for(;;) {
		sprintf(opt[0],"BBS Name:       %s",item->name);
		sprintf(opt[1],"RLogin Address: %s",item->addr);
		sprintf(opt[2],"RLogin Port:    %hu",item->port);
		sprintf(opt[3],"Username:       %s",item->user);
		sprintf(opt[4],"Password");
		sprintf(opt[5],"Be Dumb:        %s",item->dumb?"Yes":"No");
		sprintf(opt[6],"Reversed:       %s",item->reversed?"Yes":"No");
		uifc.changes=0;

		uifc.helpbuf=	"`Edit BBS`\n\n"
						"Select item to edit.";
		switch(uifc.list(WIN_MID|WIN_SAV,0,0,0,&copt,NULL,"Edit Entry",opts)) {
			case -1:
				if((listfile=fopen(listpath,"w"))!=NULL) {
					iniWriteFile(listfile,inifile);
					fclose(listfile);
				}
				strListFreeStrings(inifile);
				return(changed);
			case 0:
				uifc.helpbuf=	"`BBS Name`\n\n"
								"Enter the BBS name as it is to appear in the list.";
				strcpy(tmp,item->name);
				uifc.input(WIN_MID|WIN_SAV,0,0,"BBS Name",item->name,LIST_NAME_MAX,K_EDIT);
				iniRenameSection(&inifile,tmp,item->name);
				break;
			case 1:
				uifc.helpbuf=	"`RLogin address`\n\n"
								"Enter the domain name of the system to connect to ie:\n"
								"nix.synchro.net";
				uifc.input(WIN_MID|WIN_SAV,0,0,"RLogin Address",item->addr,LIST_ADDR_MAX,K_EDIT);
				iniSetString(&inifile,item->name,"Address",item->addr,NULL);
				break;
			case 2:
				i=item->port;
				sprintf(str,"%hu",item->port?item->port:513);
				uifc.helpbuf=	"`RLogin port`\n\n"
								"Enter the port which RLogin is listening to on the remote system\n\n"
								"~ NOTE:~\n"
								"Connecting to telnet ports currently appears to work... however, if an\n"
								"ASCII 255 char is sent by either end, it will be handled incorreclty by\n"
								"the remote system.  Further, if the remote system follows the RFC, some\n"
								"Terminal weirdness should be expected.  This program DOES NOT do telnet.";
				uifc.input(WIN_MID|WIN_SAV,0,0,"RLogin Port",str,5,K_EDIT|K_NUMBER);
				j=atoi(str);
				if(j<1 || j>65535)
					j=513;
				item->port=j;
				iniSetShortInt(&inifile,item->name,"Port",item->port,NULL);
				if(i!=j)
					uifc.changes=1;
				else
					uifc.changes=0;
				break;
			case 3:
				uifc.helpbuf=	"`Username`\n\n"
								"Enter the username to attempt auto-login to the remote with.";
				uifc.input(WIN_MID|WIN_SAV,0,0,"Username",item->user,MAX_USER_LEN,K_EDIT);
				iniSetString(&inifile,item->name,"UserName",item->user,NULL);
				break;
			case 4:
				uifc.helpbuf=	"`Password`\n\n"
								"Enter your password for auto-login.";
				uifc.input(WIN_MID|WIN_SAV,0,0,"Password",item->password,MAX_PASSWD_LEN,K_EDIT);
				iniSetString(&inifile,item->name,"Password",item->password,NULL);
				break;
			case 5:
				item->dumb=!item->dumb;
				changed=1;
				iniSetBool(&inifile,item->name,"BeDumb",item->dumb,NULL);
				break;
			case 6:
				item->reversed=!item->reversed;
				changed=1;
				iniSetBool(&inifile,item->name,"Reversed",item->reversed,NULL);
				break;
		}
		if(uifc.changes)
			changed=1;
	}
}

void add_bbs(char *listpath, struct bbslist *bbs)
{
	FILE *listfile;
	str_list_t	inifile;

	if((listfile=fopen(listpath,"r"))!=NULL) {
		inifile=iniReadFile(listfile);
		fclose(listfile);
	}
	else {
		inifile=strListInit();
	}
	/* 
	 * Redundant:
	 * iniAddSection(&inifile,bbs->name,NULL);
	 */
	iniSetString(&inifile,bbs->name,"Address",bbs->addr,NULL);
	iniSetShortInt(&inifile,bbs->name,"Port",bbs->port,NULL);
	iniSetInteger(&inifile,bbs->name,"Added",time(NULL),NULL);
	iniSetInteger(&inifile,bbs->name,"LastConnected",bbs->connected,NULL);
	iniSetInteger(&inifile,bbs->name,"TotalCalls",bbs->calls,NULL);
	iniSetString(&inifile,bbs->name,"UserName",bbs->user,NULL);
	iniSetString(&inifile,bbs->name,"Password",bbs->password,NULL);
	iniSetBool(&inifile,bbs->name,"BeDumb",bbs->dumb,NULL);
	iniSetBool(&inifile,bbs->name,"Reversed",bbs->reversed,NULL);
	if((listfile=fopen(listpath,"w"))!=NULL) {
		iniWriteFile(listfile,inifile);
		fclose(listfile);
	}
	strListFreeStrings(inifile);
}

void del_bbs(char *listpath, struct bbslist *bbs)
{
	FILE *listfile;
	str_list_t	inifile;

	if((listfile=fopen(listpath,"r"))!=NULL) {
		inifile=iniReadFile(listfile);
		fclose(listfile);
		iniRemoveSection(&inifile,bbs->name);
		if((listfile=fopen(listpath,"w"))!=NULL) {
			iniWriteFile(listfile,inifile);
			fclose(listfile);
		}
		strListFreeStrings(inifile);
	}
}

/*
 * Displays the BBS list and allows edits to user BBS list
 * Mode is one of BBSLIST_SELECT or BBSLIST_EDIT
 */
struct bbslist *show_bbslist(int mode, char *path)
{
	char	*home;
	char	listpath[MAX_PATH+1];
	struct	bbslist	*list[MAX_OPTS+1];
	int		i,j;
	int		opt=0,bar=0;
	static struct bbslist retlist;
	int		val;
	int		listcount=0;
	char	str[6];
	char	*YesNo[3]={"Yes","No",""};

	if(init_uifc())
		return(NULL);

	/* User BBS list */
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
		return(NULL);
	}
	read_list(listpath, &list[0], &listcount, USER_BBSLIST);

	/* System BBS List */
#ifdef PREFIX
	strcpy(listpath,PREFIX"/etc/syncterm.lst");

	read_list(listpath, list, &listcount, SYSTEM_BBSLIST);
#endif
	sort_list(list);

	for(;;) {
		uifc.helpbuf=	"`SyncTERM Dialing List`\n\n"
						"Commands:\n"
						"~ CTRL-E ~ Switch listing to Edit mode\n"
						"~ CTRL-D ~ Switch listing to Dial mode\n"
						"Select a bbs to edit/dial an entry.";
		val=uifc.list((listcount<MAX_OPTS?WIN_XTR:0)|WIN_SAV|WIN_MID|WIN_INS|WIN_DEL|WIN_EXTKEYS|WIN_INSACT,0,0,0,&opt,&bar,mode==BBSLIST_SELECT?"Select BBS":"Edit BBS",(char **)list);
		if(val==listcount)
			val=listcount|MSK_INS;
		if(val<0) {
			switch(val) {
				case -7:		/* CTRL-E */
					mode=BBSLIST_EDIT;
					break;
				case -6:		/* CTRL-D */
					mode=BBSLIST_SELECT;
					break;
				case -1:		/* ESC */
					return(NULL);
			}
		}
		else if(val&MSK_ON) {
			switch(val&MSK_ON) {
				case MSK_INS:
					if(listcount>=MAX_OPTS) {
						uifc.helpbuf=	"`Max List size reached`\n\n"
										"The total combined size of loaded BBS lists is currently the highest\n"
										"Supported size.  You must delete entries before adding more.";
						uifc.msg("Max List size reached!");
						break;
					}
					listcount++;
					list[listcount]=list[listcount-1];
					list[listcount-1]=(struct bbslist *)malloc(sizeof(struct bbslist));
					memset(list[listcount-1],0,sizeof(struct bbslist));
					list[listcount-1]->id=listcount-1;
					uifc.changes=0;
					uifc.helpbuf=	"`BBS Name`\n\n"
									"Enter the BBS name as it is to appear in the list.";
					uifc.input(WIN_MID|WIN_SAV,0,0,"BBS Name",list[listcount-1]->name,LIST_NAME_MAX,K_EDIT);
					if(uifc.changes) {
						uifc.changes=0;
						uifc.helpbuf=	"`RLogin address`\n\n"
										"Enter the domain name of the system to connect to ie:\n"
										"nix.synchro.net";
						uifc.input(WIN_MID|WIN_SAV,0,0,"RLogin Address",list[listcount-1]->addr,LIST_ADDR_MAX,K_EDIT);
					}
					if(!uifc.changes) {
						free(list[listcount-1]);
						list[listcount-1]=list[listcount];
						listcount--;
					}
					else {
						while(!list[listcount-1]->port) {
							list[listcount-1]->port=513;
							sprintf(str,"%hu",list[listcount-1]->port);
							uifc.helpbuf=	"`RLogin port`\n\n"
											"Enter the port which RLogin is listening to on the remote system\n\n"
											"~ NOTE:~\n"
											"Connecting to telnet ports currently appears to work... however, if an\n"
											"ASCII 255 char is sent by either end, it will be handled incorreclty by\n"
											"the remote system.  Further, if the remote system follows the RFC, some\n"
											"Terminal weirdness should be expected.  This program DOES NOT do telnet.";
							uifc.input(WIN_MID|WIN_SAV,0,0,"RLogin Port",str,5,K_EDIT|K_NUMBER);
							j=atoi(str);
							if(j<1 || j>65535)
								j=0;
							list[listcount-1]->port=j;
						}
						if(list[listcount-1]->port != 513) {
							uifc.helpbuf=	"`Be Dumb`\n\n"
											"Select this option if attempting to connect to a dumb telnet BBS";
							list[listcount-1]->dumb=0;
							uifc.list(WIN_MID|WIN_SAV,0,0,0,&list[listcount-1]->dumb,NULL,"Be Dumb",YesNo);
							list[listcount-1]->dumb=!list[listcount-1]->dumb;
						}
						if(!list[listcount-1]->dumb) {
							uifc.helpbuf=	"`Username`\n\n"
											"Enter the username to attempt auto-login to the remote with.";
							uifc.input(WIN_MID|WIN_SAV,0,0,"User Name",list[listcount-1]->user,MAX_USER_LEN,K_EDIT);
							uifc.helpbuf=	"`Password`\n\n"
											"Enter your password for auto-login.";
							uifc.input(WIN_MID|WIN_SAV,0,0,"Password",list[listcount-1]->password,MAX_PASSWD_LEN,K_EDIT);
							uifc.helpbuf=	"`Reversed`\n\n"
											"Select this option if you wish to send the username and password in the wrong\n"
											"order (usefull for connecting to v3.11 and lower systems with the default"
											"config)";
							list[listcount-1]->reversed=0;
							uifc.list(WIN_MID|WIN_SAV,0,0,0,&list[listcount-1]->reversed,NULL,"Reversed",YesNo);
							list[listcount-1]->reversed=!list[listcount-1]->reversed;
						}
						else {
							list[listcount-1]->user[0]=0;
							list[listcount-1]->password[0]=0;
							list[listcount-1]->reversed=0;
						}
						add_bbs(listpath,list[listcount-1]);
						sort_list(list);
						for(j=0;list[j]->name[0];j++) {
							if(list[j]->id==listcount-1)
								opt=j;
						}
					}
					break;
				case MSK_DEL:
					if(!list[opt]->name[0]) {
						uifc.helpbuf=	"`Calming down`\n\n";
										"~ Some handy tips on calming down ~\n"
										"Close your eyes, imagine yourself alone on a brilliant white beach...\n"
										"Picture the palm trees up towards the small town...\n"
										"Glory in the deep blue of the perfectly clean ocean...\n"
										"Feel the plush comfort of your beach towel...\n"
										"Enjoy the shade of your satellite internet feed which envelops\n"
										"your head, keeping you cool...\n"
										"Set your TEMPEST rated laptop aside on the beach, knowing it's\n"
										"completely impervious to anything on the beach...\n"
										"Reach over to your fridge, grab a cold one...\n"
										"Watch the seagulls in their dance...\n";
						uifc.msg("It's gone, calm down man!");
						break;
					}
					del_bbs(listpath,list[opt]);
					free(list[opt]);
					for(i=opt;list[i]->name[0];i++) {
						list[i]=list[i+1];
					}
					for(i=0;list[i]->name[0];i++) {
						list[i]->id=i;
					}
					listcount--;
					break;
			}
		}
		else {
			if(mode==BBSLIST_EDIT) {
				i=list[opt]->id;
				if(edit_list(list[opt],listpath)) {
					sort_list(list);
					for(j=0;list[j]->name[0];j++) {
						if(list[j]->id==i)
							opt=j;
					}
				}
			}
			else {
				memcpy(&retlist,list[val],sizeof(struct bbslist));
				return(&retlist);
			}
		}
	}
}
