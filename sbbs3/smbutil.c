/* smbutil.c */

/* Synchronet message base (SMB) utility */

/* $Id$ */

/****************************************************************************
 * @format.tab-size 4		(Plain Text/Source Code File Header)			*
 * @format.use-tabs true	(see http://www.synchro.net/ptsc_hdr.html)		*
 *																			*
 * Copyright 2000 Rob Swindell - http://www.synchro.net/copyright.html		*
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

#define SMBUTIL_VER "2.30"

#define NOANALYSIS		(1L<<0)

#ifdef __WATCOMC__
	#define ffblk find_t
    #define findfirst(x,y,z) _dos_findfirst(x,z,y)
	#define findnext(x) _dos_findnext(x)
#endif

#if defined(_WIN32)
	#include <ctype.h>	/* isdigit() */
	#include <conio.h>	/* getch() */
#endif

/* ANSI */
#include <stdio.h>
#include <time.h>	/* time */
#include <errno.h>	/* errno */
#include <string.h>	/* strrchr */

#include "smbwrap.h"
#include "smblib.h"
#include "crc32.h"

#ifdef __WATCOMC__
	#include <dos.h>
#endif

/* gets is dangerous */
#define gets(str)  fgets((str), sizeof(str) - 1, stdin)

/********************/
/* Global variables */
/********************/

smb_t smb;
ulong mode=0L;
ushort tzone=PST;

/************************/
/* Program usage/syntax */
/************************/

char *usage=
"usage: smbutil [-opts] cmd <filespec.shd>\n"
"\n"
"cmd:\n"
"       l[n] = list msgs starting at number n\n"
"       r[n] = read msgs starting at number n\n"
"       v[n] = view msg headers starting at number n\n"
"       i[f] = import msg from text file f (or use stdin)\n"
"       e[f] = import e-mail from text file f (or use stdin)\n"
"       n[f] = import netmail from text file f (or use stdin)\n"
"       s    = display msg base status\n"
"       c    = change msg base status\n"
"       m    = maintain msg base - delete old msgs and msgs over max\n"
"       p[k] = pack msg base (k specifies minimum packable Kbytes)\n"
"opts:\n"
"       c    = create message base if it doesn't exist\n"
"       a    = always pack msg base (disable compression analysis)\n"
"       t<s> = set 'to' user name for imported message\n"
"       n<s> = set 'to' netmail address for imported message\n"
"       u<s> = set 'to' user number for imported message\n"
"       f<s> = set 'from' user name for imported message\n"
"       e<s> = set 'from' user number for imported message\n"
"       s<s> = set 'subject' for imported message\n"
"       z[n] = set time zone (n=min +/- from UT or 'EST','EDT','CST',etc)\n"
;

char *wday[]={"Sun","Mon","Tue","Wed","Thu","Fri","Sat"};
char *mon[]={"Jan","Feb","Mar","Apr","May","Jun"
            ,"Jul","Aug","Sep","Oct","Nov","Dec"};

/****************************************************************************/
/* Updates 16-bit "rcrc" with character 'ch'                                */
/****************************************************************************/
void ucrc16(uchar ch, ushort *rcrc) {
	ushort i, cy;
    uchar nch=ch;
 
	for (i=0; i<8; i++) {
		cy=*rcrc & 0x8000;
		*rcrc<<=1;
		if (nch & 0x80) *rcrc |= 1;
		nch<<=1;
		if (cy) *rcrc ^= 0x1021; }
}

/****************************************************************************/
/* Returns CRC-16 of string (not including terminating NULL)				*/
/****************************************************************************/
ushort crc16(char *str)
{
	int 	i=0;
	ushort	crc=0;

	ucrc16(0,&crc);
	while(str[i])
		ucrc16(str[i++],&crc);
	ucrc16(0,&crc);
	ucrc16(0,&crc);
	return(crc);
}

void remove_re(char *str)
{
	while(!strnicmp(str,"RE:",3)) {
		strcpy(str,str+3);
		while(str[0]==SP)
			strcpy(str,str+1); 
	}
}

/************************************************/
/* Truncates white-space chars off end of 'str' */
/************************************************/
static void truncsp(char *str)
{
	uint c;

	c=strlen(str);
	while(c && (uchar)str[c-1]<=' ') c--;
	str[c]=0;
}



/****************************************************************************/
/* Adds a new message to the message base									*/
/****************************************************************************/
void postmsg(char type, char* to, char* to_number, char* to_address, 
			 char* from, char* from_number, char* subject, FILE* fp)
{
	char	str[128];
	char	buf[128];
	char	pad=0;
	char*	msgtxt=NULL;
	long	msgtxtlen;
	ushort	xlat,net;
	int 	i;
	long	l,length;
	ulong	offset,crc;
	smbmsg_t	msg;

	/* Read message text from stream (file or stdin) */
	msgtxtlen=0;
	while(!feof(fp)) {
		i=fread(buf,1,sizeof(buf),fp);
		if(i<1)
			break;
		if((msgtxt=(char*)realloc(msgtxt,msgtxtlen+i))==NULL) {
			printf("\n\7malloc(%ld) failure\n",msgtxtlen+i);
			exit(1);
		}
		memcpy(msgtxt+msgtxtlen,buf,i);
		msgtxtlen+=i;
	}

	/* Allocate space in message base */
	length=msgtxtlen+sizeof(xlat);	/* for translation string */
	if(!(smb.status.attr&SMB_HYPERALLOC)) {
		i=smb_open_da(&smb);
		if(i) {
			printf("\n\7!smb_open_da returned %d: %s\n",i,smb.last_error);
			exit(1); 
		}
		offset=smb_allocdat(&smb,length,1);
		smb_close_da(&smb); 
	} else
		offset=smb_hallocdat(&smb);

	fseek(smb.sdt_fp,offset,SEEK_SET);
	xlat=XLAT_NONE;
	smb_fwrite(&xlat,sizeof(xlat),smb.sdt_fp);
	smb_fwrite(msgtxt,msgtxtlen,smb.sdt_fp);
	for(l=length;l%SDT_BLOCK_LEN;l++)
		smb_fwrite(&pad,1,smb.sdt_fp);
	fflush(smb.sdt_fp);

	memset(&msg,0,sizeof(smbmsg_t));
	memcpy(msg.hdr.id,"SHD\x1a",4);
	msg.hdr.version=smb_ver();
	msg.hdr.when_written.time=time(NULL);
	msg.hdr.when_written.zone=tzone;
	msg.hdr.when_imported=msg.hdr.when_written;

	if(smb.status.max_crcs) {
		crc=0xffffffffUL;
		for(l=0;l<msgtxtlen;l++) 
			crc=ucrc32(msgtxt[l],crc);
		crc=~crc;
		i=smb_addcrc(&smb,crc);
		if(i) {
			printf("\n\7!smb_addcrc returned %d: %s\n",i,smb.last_error);
			smb_freemsgdat(&smb,offset,length,1);
			exit(1); 
		} 
	}

	msg.hdr.offset=offset;

	if(to==NULL) {
		printf("To User Name: ");
		fgets(str,sizeof(str)-1,stdin); 
	} else
		sprintf(str,"%.*s",sizeof(str)-1,to);

	truncsp(str);
	i=smb_hfield(&msg,RECIPIENT,(ushort)strlen(str),str);
	if(i) {
		printf("\n\7!smb_hfield returned %d: %s\n",i,smb.last_error);
		smb_freemsgdat(&smb,offset,length,1);
		exit(1); }
	if(type=='E' || type=='N')
		smb.status.attr|=SMB_EMAIL;
	if(smb.status.attr&SMB_EMAIL) {
		if(to_number==NULL) {
			printf("To User Number (0=QWKnet or Internet): ");
			gets(str);
		} else
			sprintf(str,"%.*s",sizeof(str)-1,to_number);
		truncsp(str);
		i=smb_hfield(&msg,RECIPIENTEXT,(ushort)strlen(str),str);
		if(i) {
			printf("\n\7!smb_hfield returned %d: %s\n",i,smb.last_error);
			smb_freemsgdat(&smb,offset,length,1);
			exit(1); 
		}
		msg.idx.to=atoi(str); }
	else {
		strlwr(str);
		msg.idx.to=crc16(str); 
	}

	if(smb.status.attr&SMB_EMAIL && (type=='N' || !msg.idx.to)) {
		if(to_address==NULL) {
			printf("To Address: ");
			gets(str);
		} else
			sprintf(str,"%.*s",sizeof(str)-1,to_address);
		truncsp(str);
		if(*str) {
			if(strchr(str,'.'))
				net=NET_INTERNET;
			else
				net=NET_QWK;
			i=smb_hfield(&msg,RECIPIENTNETTYPE,sizeof(net),&net);
			if(i) {
				printf("\n\7!smb_hfield returned %d: %s\n",i,smb.last_error);
				smb_freemsgdat(&smb,offset,length,1);
				exit(1); }
			i=smb_hfield(&msg,RECIPIENTNETADDR,(ushort)strlen(str),str);
			if(i) {
				printf("\n\7!smb_hfield returned %d: %s\n",i,smb.last_error);
				smb_freemsgdat(&smb,offset,length,1);
				exit(1); 
			} 
		} 
	}

	if(from==NULL) {
		printf("From User Name: ");
		gets(str);
	} else
		sprintf(str,"%.*s",sizeof(str)-1,from);
	truncsp(str);
	i=smb_hfield(&msg,SENDER,(ushort)strlen(str),str);
	if(i) {
		printf("\n\7!smb_hfield returned %d: %s\n",i,smb.last_error);
		smb_freemsgdat(&smb,offset,length,1);
		exit(1); }
	if(smb.status.attr&SMB_EMAIL) {
		if(from_number==NULL) {
			printf("From User Number: ");
			gets(str);
		} else
			sprintf(str,"%.*s",sizeof(str)-1,from_number);
		truncsp(str);
		i=smb_hfield(&msg,SENDEREXT,(ushort)strlen(str),str);
		if(i) {
			printf("\n\7!smb_hfield returned %d: %s\n",i,smb.last_error);
			smb_freemsgdat(&smb,offset,length,1);
			exit(1); 
		}
		msg.idx.from=atoi(str); 
	} else {
		strlwr(str);
		msg.idx.from=crc16(str); 
	}

	if(subject==NULL) {
		printf("Subject: ");
		gets(str);
	} else
		sprintf(str,"%.*s",sizeof(str)-1,subject);
	truncsp(str);
	i=smb_hfield(&msg,SUBJECT,(ushort)strlen(str),str);
	if(i) {
		printf("\n\7!smb_hfield returned %d: %s\n",i,smb.last_error);
		smb_freemsgdat(&smb,offset,length,1);
		exit(1); 
	}
	remove_re(str);
	strlwr(str);
	msg.idx.subj=crc16(str);

	i=smb_dfield(&msg,TEXT_BODY,length);
	if(i) {
		printf("\n\7!smb_dfield returned %d: %s\n",i,smb.last_error);
		smb_freemsgdat(&smb,offset,length,1);
		exit(1); }

	i=smb_addmsghdr(&smb,&msg,smb.status.attr&SMB_HYPERALLOC);

	if(i) {
		printf("\n\7!smb_addmsghdr returned %d: %s\n",i,smb.last_error);
		smb_freemsgdat(&smb,offset,length,1);
		exit(1); }
	smb_freemsgmem(&msg);

	free(msgtxt);
}

/****************************************************************************/
/* Shows the message base header											*/
/****************************************************************************/
void showstatus(void)
{
	int i;

	i=smb_locksmbhdr(&smb);
	if(i) {
		printf("\n\7!smb_locksmbhdr returned %d: %s\n",i,smb.last_error);
		return; }
	i=smb_getstatus(&smb);
	smb_unlocksmbhdr(&smb);
	if(i) {
		printf("\n\7!smb_getstatus returned %d: %s\n",i,smb.last_error);
		return; }
	printf("last_msg        =%lu\n"
		   "total_msgs      =%lu\n"
		   "header_offset   =%lu\n"
		   "max_crcs        =%lu\n"
		   "max_msgs        =%lu\n"
		   "max_age         =%u\n"
		   "attr            =%04Xh\n"
		   ,smb.status.last_msg
		   ,smb.status.total_msgs
		   ,smb.status.header_offset
		   ,smb.status.max_crcs
		   ,smb.status.max_msgs
		   ,smb.status.max_age
		   ,smb.status.attr
		   );
}

/****************************************************************************/
/* Configure message base header											*/
/****************************************************************************/
void config(void)
{
	char max_msgs[128],max_crcs[128],max_age[128],header_offset[128],attr[128];
	int i;

	i=smb_locksmbhdr(&smb);
	if(i) {
		printf("\n\7!smb_locksmbhdr returned %d: %s\n",i,smb.last_error);
		return; }
	i=smb_getstatus(&smb);
	smb_unlocksmbhdr(&smb);
	if(i) {
		printf("\n\7!smb_getstatus returned %d: %s\n",i,smb.last_error);
		return; }
	printf("Header offset =%-5lu  New value (CR=No Change): "
		,smb.status.header_offset);
	gets(header_offset);
	printf("Max msgs      =%-5lu  New value (CR=No Change): "
		,smb.status.max_msgs);
	gets(max_msgs);
	printf("Max crcs      =%-5lu  New value (CR=No Change): "
		,smb.status.max_crcs);
	gets(max_crcs);
	printf("Max age       =%-5u  New value (CR=No Change): "
		,smb.status.max_age);
	gets(max_age);
	printf("Attributes    =%-5u  New value (CR=No Change): "
		,smb.status.attr);
	gets(attr);
	i=smb_locksmbhdr(&smb);
	if(i) {
		printf("\n\7!smb_locksmbhdr returned %d: %s\n",i,smb.last_error);
		return; }
	i=smb_getstatus(&smb);
	if(i) {
		printf("\n\7!smb_getstatus returned %d: %s\n",i,smb.last_error);
		smb_unlocksmbhdr(&smb);
		return; }
	if(isdigit(max_msgs[0]))
		smb.status.max_msgs=atol(max_msgs);
	if(isdigit(max_crcs[0]))
		smb.status.max_crcs=atol(max_crcs);
	if(isdigit(max_age[0]))
		smb.status.max_age=atoi(max_age);
	if(isdigit(header_offset[0]))
		smb.status.header_offset=atol(header_offset);
	if(isdigit(attr[0]))
		smb.status.attr=atoi(attr);
	i=smb_putstatus(&smb);
	smb_unlocksmbhdr(&smb);
	if(i)
		printf("\n\7!smb_putstatus returned %d: %s\n",i,smb.last_error);
}

/****************************************************************************/
/* Lists messages' to, from, and subject                                    */
/****************************************************************************/
void listmsgs(ulong start, ulong count)
{
	int i;
	ulong l=0;
	smbmsg_t msg;

	if(!start)
		start=1;
	fseek(smb.sid_fp,(start-1L)*sizeof(idxrec_t),SEEK_SET);
	while(l<count) {
		if(!fread(&msg.idx,1,sizeof(idxrec_t),smb.sid_fp))
			break;
		i=smb_lockmsghdr(&smb,&msg);
		if(i) {
			printf("\n\7!smb_lockmsghdr returned %d: %s\n",i,smb.last_error);
			break; }
		i=smb_getmsghdr(&smb,&msg);
		smb_unlockmsghdr(&smb,&msg);
		if(i) {
			printf("\n\7!smb_getmsghdr returned %d: %s\n",i,smb.last_error);
			break; }
		printf("%4lu %-25.25s %-25.25s %.20s\n"
			,msg.hdr.number,msg.from,msg.to,msg.subj);
		smb_freemsgmem(&msg);
		l++; }
}

/****************************************************************************/
/* Returns an ASCII string for FidoNet address 'addr'                       */
/****************************************************************************/
char *faddrtoa(fidoaddr_t addr)
{
	static char str[25];
	char point[25];

	sprintf(str,"%u:%u/%u",addr.zone,addr.net,addr.node);
	if(addr.point) {
		sprintf(point,".%u",addr.point);
		strcat(str,point); }
	return(str);
}

char *binstr(uchar *buf, ushort length)
{
	static char str[512];
	char tmp[128];
	int i;

	str[0]=0;
	for(i=0;i<length;i++)
		if(buf[i] && (buf[i]<SP || buf[i]>=0x7f) && buf[i]!=CR && buf[i]!=LF)
			break;
	if(i==length)		/* not binary */
		return(buf);
	for(i=0;i<length;i++) {
		sprintf(tmp,"%02X ",buf[i]);
		strcat(str,tmp); 
		if(i>=100) {
			strcat(str,"...");
			break;
		}
	}
	return(str);
}

/****************************************************************************/
/* Generates a 24 character ASCII string that represents the time_t pointer */
/* Used as a replacement for ctime()                                        */
/****************************************************************************/
char *timestr(time_t *intime)
{
    static char str[256];
    char mer[3],hour;
    struct tm *gm;

	gm=localtime(intime);
	if(gm==NULL) {
		strcpy(str,"Invalid Time");
		return(str); }
	if(gm->tm_hour>=12) {
		if(gm->tm_hour==12)
			hour=12;
		else
			hour=gm->tm_hour-12;
		strcpy(mer,"pm"); }
	else {
		if(gm->tm_hour==0)
			hour=12;
		else
			hour=gm->tm_hour;
		strcpy(mer,"am"); }
	sprintf(str,"%s %s %02d %4d %02d:%02d %s"
		,wday[gm->tm_wday],mon[gm->tm_mon],gm->tm_mday,1900+gm->tm_year
		,hour,gm->tm_min,mer);
	return(str);
}


/****************************************************************************/
/* Converts when_t.zone into ASCII format									*/
/****************************************************************************/
char *zonestr(short zone)
{
	static char str[32];

	switch((ushort)zone) {
		case 0: 	return("UT");
		case AST:	return("AST");
		case EST:	return("EST");
		case CST:	return("CST");
		case MST:	return("MST");
		case PST:	return("PST");
		case YST:	return("YST");
		case HST:	return("HST");
		case BST:	return("BST");
		case ADT:	return("ADT");
		case EDT:	return("EDT");
		case CDT:	return("CDT");
		case MDT:	return("MDT");
		case PDT:	return("PDT");
		case YDT:	return("YDT");
		case HDT:	return("HDT");
		case BDT:	return("BDT");
		case MID:	return("MID");
		case VAN:	return("VAN");
		case EDM:	return("EDM");
		case WIN:	return("WIN");
		case BOG:	return("BOG");
		case CAR:	return("CAR");
		case RIO:	return("RIO");
		case FER:	return("FER");
		case AZO:	return("AZO");
		case LON:	return("LON");
		case BER:	return("BER");
		case ATH:	return("ATH");
		case MOS:	return("MOS");
		case DUB:	return("DUB");
		case KAB:	return("KAB");
		case KAR:	return("KAR");
		case BOM:	return("BOM");
		case KAT:	return("KAT");
		case DHA:	return("DHA");
		case BAN:	return("BAN");
		case HON:	return("HON");
		case TOK:	return("TOK");
		case SYD:	return("SYD");
		case NOU:	return("NOU");
		case WEL:	return("WEL");
		}

	sprintf(str,"%02d:%02u",zone/60,zone<0 ? (-zone)%60 : zone%60);
	return(str);
}
			 

/****************************************************************************/
/* Displays message header information										*/
/****************************************************************************/
void viewmsgs(ulong start, ulong count)
{
	char when_written[128]
		,when_imported[128];
	int i;
	ulong l=0;
	smbmsg_t msg;

	if(!start)
		start=1;
	fseek(smb.sid_fp,(start-1L)*sizeof(idxrec_t),SEEK_SET);
	while(l<count) {
		if(!fread(&msg.idx,1,sizeof(idxrec_t),smb.sid_fp))
			break;
		i=smb_lockmsghdr(&smb,&msg);
		if(i) {
			printf("\n\7!smb_lockmsghdr returned %d: %s\n",i,smb.last_error);
			break; }
		i=smb_getmsghdr(&smb,&msg);
		smb_unlockmsghdr(&smb,&msg);
		if(i) {
			printf("\n\7!smb_getmsghdr returned %d: %s\n",i,smb.last_error);
			break; }

		sprintf(when_written,"%.24s %s"
			,timestr((time_t*)&msg.hdr.when_written.time)
			,zonestr(msg.hdr.when_written.zone));
		sprintf(when_imported,"%.24s %s"
			,timestr((time_t*)&msg.hdr.when_imported.time)
			,zonestr(msg.hdr.when_imported.zone));

		printf( "%-20.20s %s\n"
				"%-20.20s %s\n"
				"%-20.20s %s\n"
				"%-20.20s %04Xh\n"
				"%-20.20s %04Xh\n"
				"%-20.20s %u\n"
				"%-20.20s %04Xh\n"
				"%-20.20s %08lXh\n"
				"%-20.20s %08lXh\n"
				"%-20.20s %s\n"
				"%-20.20s %s\n"
				"%-20.20s %ld (%ld)\n"
				"%-20.20s %ld\n"
				"%-20.20s %ld\n"
				"%-20.20s %ld\n"
				"%-20.20s %s\n"
				"%-20.20s %06lXh\n"
				"%-20.20s %u\n",

			"subj",
			msg.subj,

			"from",
			msg.from,

			"to",
			msg.to,

			"type",
			msg.hdr.type,

			"version",
			msg.hdr.version,

			"length",
			msg.hdr.length,

			"attr",
			msg.hdr.attr,

			"auxattr",
			msg.hdr.auxattr,

			"netattr",
			msg.hdr.netattr,

			"when_written",
			when_written,

			"when_imported",
			when_imported,

			"number",
			msg.hdr.number,
			ftell(smb.sid_fp)/sizeof(idxrec_t),

			"thread_orig",
			msg.hdr.thread_orig,

			"thread_next",
			msg.hdr.thread_next,

			"thread_first",
			msg.hdr.thread_first,

			"reserved[16]",
			binstr(msg.hdr.reserved,16),

			"offset",
			msg.hdr.offset,

			"total_dfields",
			msg.hdr.total_dfields
			);
		for(i=0;i<msg.hdr.total_dfields;i++)
			printf("dfield[%02u].type      %02Xh\n"
				   "dfield[%02u].offset    %lu\n"
				   "dfield[%02u].length    %lu\n"
				   ,i,msg.dfield[i].type
				   ,i,msg.dfield[i].offset
				   ,i,msg.dfield[i].length);

		for(i=0;i<msg.total_hfields;i++)
			printf("hfield[%02u].type      %02Xh\n"
				   "hfield[%02u].length    %d\n"
				   "hfield[%02u]_dat       %s\n"
				   ,i,msg.hfield[i].type
				   ,i,msg.hfield[i].length
				   ,i,binstr(msg.hfield_dat[i],msg.hfield[i].length));

		if(msg.from_net.type)
			printf("from_net.type        %02Xh\n"
				   "from_net.addr        %s\n"
				,msg.from_net.type
				,msg.from_net.type==NET_FIDO
				? faddrtoa(*(fidoaddr_t *)msg.from_net.addr) : msg.from_net.addr);

		if(msg.to_net.type)
			printf("to_net.type          %02Xh\n"
				   "to_net.addr          %s\n"
				,msg.to_net.type
				,msg.to_net.type==NET_FIDO
				? faddrtoa(*(fidoaddr_t *)msg.to_net.addr) : msg.to_net.addr);

		if(msg.replyto_net.type)
			printf("replyto_net.type     %02Xh\n"
				   "replyto_net.addr     %s\n"
				,msg.replyto_net.type
				,msg.replyto_net.type==NET_FIDO
				? faddrtoa(*(fidoaddr_t *)msg.replyto_net.addr)
					: msg.replyto_net.addr);

		printf("from_agent           %02Xh\n"
			   "to_agent             %02Xh\n"
			   "replyto_agent        %02Xh\n"
			   ,msg.from_agent
			   ,msg.to_agent
			   ,msg.replyto_agent);

		printf("\n");
		smb_freemsgmem(&msg);
		l++; }
}

/****************************************************************************/
/* Maintain message base - deletes messages older than max age (in days)	*/
/* or messages that exceed maximum											*/
/****************************************************************************/
void maint(void)
{
	int i;
	ulong l,m,n,f,flagged=0;
	time_t now;
	smbmsg_t msg;
	idxrec_t HUGE16 *idx;

	printf("Maintaining %s\r\n",smb.file);
	now=time(NULL);
	i=smb_locksmbhdr(&smb);
	if(i) {
		printf("\n\7!smb_locksmbhdr returned %d: %s\n",i,smb.last_error);
		return; }
	i=smb_getstatus(&smb);
	if(i) {
		smb_unlocksmbhdr(&smb);
		printf("\n\7!smb_getstatus returned %d: %s\n",i,smb.last_error);
		return; }
	if(!smb.status.total_msgs) {
		smb_unlocksmbhdr(&smb);
		printf("Empty\n");
		return; }
	printf("Loading index...\n");
	if((idx=(idxrec_t *)LMALLOC(sizeof(idxrec_t)*smb.status.total_msgs))
		==NULL) {
		smb_unlocksmbhdr(&smb);
		printf("\n\7!Error allocating %lu bytes of memory\n"
			,sizeof(idxrec_t)*smb.status.total_msgs);
		return; }
	fseek(smb.sid_fp,0L,SEEK_SET);
	for(l=0;l<smb.status.total_msgs;l++) {
		printf("%lu of %lu\r"
			,l+1,smb.status.total_msgs);
		if(!fread(&idx[l],1,sizeof(idxrec_t),smb.sid_fp))
			break; }
	printf("\nDone.\n\n");

	printf("Scanning for pre-flagged messages...\n");
	for(m=0;m<l;m++) {
		printf("\r%2u%%",m ? (long)(100.0/((float)l/m)) : 0);
		if(idx[m].attr&MSG_DELETE)
			flagged++; }
	printf("\r100%% (%lu pre-flagged for deletion)\n",flagged);

	if(smb.status.max_age) {
		printf("Scanning for messages more than %u days old...\n"
			,smb.status.max_age);
		for(m=f=0;m<l;m++) {
			printf("\r%2u%%",m ? (long)(100.0/((float)l/m)) : 0);
			if(idx[m].attr&(MSG_PERMANENT|MSG_DELETE))
				continue;
			if((ulong)now>idx[m].time && (now-idx[m].time)/(24L*60L*60L)
				>smb.status.max_age) {
				f++;
				flagged++;
				idx[m].attr|=MSG_DELETE; } }  /* mark for deletion */
		printf("\r100%% (%lu flagged for deletion)\n",f); }

	printf("Scanning for read messages to be killed...\n");
	for(m=f=0;m<l;m++) {
		printf("\r%2u%%",m ? (long)(100.0/((float)l/m)) : 0);
		if(idx[m].attr&(MSG_PERMANENT|MSG_DELETE))
			continue;
		if((idx[m].attr&(MSG_READ|MSG_KILLREAD))==(MSG_READ|MSG_KILLREAD)) {
			f++;
			flagged++;
			idx[m].attr|=MSG_DELETE; } }
	printf("\r100%% (%lu flagged for deletion)\n",f);

	if(l-flagged>smb.status.max_msgs) {
		printf("Flagging excess messages for deletion...\n");
		for(m=n=0,f=flagged;l-flagged>smb.status.max_msgs && m<l;m++) {
			if(idx[m].attr&(MSG_PERMANENT|MSG_DELETE))
				continue;
			printf("%lu of %lu\r",++n,(l-f)-smb.status.max_msgs);
			flagged++;
			idx[m].attr|=MSG_DELETE; }			/* mark for deletion */
		printf("\nDone.\n\n"); }

	if(!flagged) {				/* No messages to delete */
		LFREE(idx);
		smb_unlocksmbhdr(&smb);
		return; }

	if(!(mode&NOANALYSIS)) {

		printf("Freeing allocated header and data blocks for deleted messages...\n");
		if(!(smb.status.attr&SMB_HYPERALLOC)) {
			i=smb_open_da(&smb);
			if(i) {
				smb_unlocksmbhdr(&smb);
				printf("\n\7!smb_open_da returned %d: %s\n",i,smb.last_error);
				exit(1); }
			i=smb_open_ha(&smb);
			if(i) {
				smb_unlocksmbhdr(&smb);
				printf("\n\7!smb_open_ha returned %d: %s\n",i,smb.last_error);
				exit(1); } }

		for(m=n=0;m<l;m++) {
			if(idx[m].attr&MSG_DELETE) {
				printf("%lu of %lu\r",++n,flagged);
				msg.idx=idx[m];
				msg.hdr.number=msg.idx.number;
				if((i=smb_getmsgidx(&smb,&msg))!=0) {
					printf("\n\7!smb_getmsgidx returned %d: %s\n",i,smb.last_error);
					continue; }
				i=smb_lockmsghdr(&smb,&msg);
				if(i) {
					printf("\n\7!smb_lockmsghdr returned %d: %s\n",i,smb.last_error);
					break; }
				if((i=smb_getmsghdr(&smb,&msg))!=0) {
					smb_unlockmsghdr(&smb,&msg);
					printf("\n\7!smb_getmsghdr returned %d: %s\n",i,smb.last_error);
					break; }
				msg.hdr.attr|=MSG_DELETE;			/* mark header as deleted */
				if((i=smb_putmsg(&smb,&msg))!=0) {
					smb_freemsgmem(&msg);
					smb_unlockmsghdr(&smb,&msg);
					printf("\n\7!smb_putmsg returned %d: %s\n",i,smb.last_error);
					break; }
				smb_unlockmsghdr(&smb,&msg);
				if((i=smb_freemsg(&smb,&msg))!=0) {
					smb_freemsgmem(&msg);
					printf("\n\7!smb_freemsg returned %d: %s\n",i,smb.last_error);
					break; }
				smb_freemsgmem(&msg); } }
		if(!(smb.status.attr&SMB_HYPERALLOC)) {
			smb_close_ha(&smb);
			smb_close_da(&smb); }
		printf("\nDone.\n\n"); }

	printf("Re-writing index...\n");
	rewind(smb.sid_fp);
	if(chsize(fileno(smb.sid_fp),0L))
		printf("chsize failed!\n");
	for(m=n=0;m<l;m++) {
		if(idx[m].attr&MSG_DELETE)
			continue;
		printf("%lu of %lu\r",++n,l-flagged);
		fwrite(&idx[m],sizeof(idxrec_t),1,smb.sid_fp); }
	printf("\nDone.\n\n");
	fflush(smb.sid_fp);

	LFREE(idx);
	smb.status.total_msgs-=flagged;
	smb_putstatus(&smb);
	smb_unlocksmbhdr(&smb);
}


typedef struct {
	ulong old,new;
	} datoffset_t;

/****************************************************************************/
/* Removes all unused blocks from SDT and SHD files 						*/
/****************************************************************************/
void packmsgs(ulong packable)
{
	uchar str[128],buf[SDT_BLOCK_LEN],ch,fname[128],tmpfname[128];
	int i,size;
	ulong l,m,n,datoffsets=0,length,total,now;
	FILE *tmp_sdt,*tmp_shd,*tmp_sid;
	smbhdr_t	hdr;
	smbmsg_t	msg;
	datoffset_t *datoffset=NULL;

	now=time(NULL);
	printf("Packing %s\n",smb.file);
	i=smb_locksmbhdr(&smb);
	if(i) {
		printf("\n\7!smb_locksmbhdr returned %d: %s\n",i,smb.last_error);
		return; }
	i=smb_getstatus(&smb);
	if(i) {
		smb_unlocksmbhdr(&smb);
		printf("\n\7!smb_getstatus returned %d: %s\n",i,smb.last_error);
		return; }

	if(!(smb.status.attr&SMB_HYPERALLOC)) {
		i=smb_open_ha(&smb);
		if(i) {
			smb_unlocksmbhdr(&smb);
			printf("\n\7!smb_open_ha returned %d: %s\n",i,smb.last_error);
			return; }
		i=smb_open_da(&smb);
		if(i) {
			smb_unlocksmbhdr(&smb);
			smb_close_ha(&smb);
			printf("\n\7!smb_open_da returned %d: %s\n",i,smb.last_error);
			return; } }

	if(!smb.status.total_msgs) {
		printf("Empty\n");
		rewind(smb.shd_fp);
		chsize(fileno(smb.shd_fp),smb.status.header_offset);
		rewind(smb.sdt_fp);
		chsize(fileno(smb.sdt_fp),0L);
		rewind(smb.sid_fp);
		chsize(fileno(smb.sid_fp),0L);
		if(!(smb.status.attr&SMB_HYPERALLOC)) {
			rewind(smb.sha_fp);
			chsize(fileno(smb.sha_fp),0L);
			rewind(smb.sda_fp);
			chsize(fileno(smb.sda_fp),0L);
			smb_close_ha(&smb);
			smb_close_da(&smb); }
		smb_unlocksmbhdr(&smb);
		return; }


	if(!(smb.status.attr&SMB_HYPERALLOC) && !(mode&NOANALYSIS)) {
		printf("Analyzing data blocks...\n");

		length=filelength(fileno(smb.sda_fp));

		fseek(smb.sda_fp,0L,SEEK_SET);
		for(l=m=0;l<length;l+=2) {
			printf("\r%2u%%  ",l ? (long)(100.0/((float)length/l)) : 0);
			i=0;
			if(!fread(&i,2,1,smb.sda_fp))
				break;
			if(!i)
				m++; }

		printf("\rAnalyzing header blocks...\n");

		length=filelength(fileno(smb.sha_fp));

		fseek(smb.sha_fp,0L,SEEK_SET);
		for(l=n=0;l<length;l++) {
			printf("\r%2u%%  ",l ? (long)(100.0/((float)length/l)) : 0);
			ch=0;
			if(!fread(&ch,1,1,smb.sha_fp))
				break;
			if(!ch)
				n++; }

		if(!m && !n) {
			printf("\rAlready compressed.\n\n");
			smb_close_ha(&smb);
			smb_close_da(&smb);
			smb_unlocksmbhdr(&smb);
			return; }

		if(packable && (m*SDT_BLOCK_LEN)+(n*SHD_BLOCK_LEN)<packable*1024L) {
			printf("\rLess than %luk compressible bytes.\n\n",packable);
			smb_close_ha(&smb);
			smb_close_da(&smb);
			smb_unlocksmbhdr(&smb);
			return; }

		printf("\rCompressing %lu data blocks (%lu bytes)\n"
				 "        and %lu header blocks (%lu bytes)\n"
				  ,m,m*SDT_BLOCK_LEN,n,n*SHD_BLOCK_LEN); }

	if(!(smb.status.attr&SMB_HYPERALLOC)) {
		rewind(smb.sha_fp);
		chsize(fileno(smb.sha_fp),0L);		/* Reset both allocation tables */
		rewind(smb.sda_fp);
		chsize(fileno(smb.sda_fp),0L); }

	if(smb.status.attr&SMB_HYPERALLOC && !(mode&NOANALYSIS)) {
		printf("Analyzing %s\n",smb.file);

		length=filelength(fileno(smb.shd_fp));
		m=n=0;
		for(l=smb.status.header_offset;l<length;l+=size) {
			printf("\r%2u%%  ",(long)(100.0/((float)length/l)));
			msg.idx.offset=l;
			if((i=smb_lockmsghdr(&smb,&msg))!=0) {
				printf("\n(%06lX) smb_lockmsghdr returned %d\n",l,i);
				size=SHD_BLOCK_LEN;
				continue; }
			if((i=smb_getmsghdr(&smb,&msg))!=0) {
				smb_unlockmsghdr(&smb,&msg);
				m++;
				size=SHD_BLOCK_LEN;
				continue; }
			smb_unlockmsghdr(&smb,&msg);
			if(msg.hdr.attr&MSG_DELETE) {
				m+=smb_hdrblocks(msg.hdr.length);
				total=0;
				for(i=0;i<msg.hdr.total_dfields;i++)
					total+=msg.dfield[i].length;
				n+=smb_datblocks(total); }
			size=smb_getmsghdrlen(&msg);
			if(size<1) size=SHD_BLOCK_LEN;
			while(size%SHD_BLOCK_LEN)
				size++;
			smb_freemsgmem(&msg); }


		if(!m && !n) {
			printf("\rAlready compressed.\n\n");
			smb_unlocksmbhdr(&smb);
			return; }

		if(packable && (n*SDT_BLOCK_LEN)+(m*SHD_BLOCK_LEN)<packable*1024L) {
			printf("\rLess than %luk compressible bytes.\n\n",packable);
			smb_unlocksmbhdr(&smb);
			return; }

		printf("\rCompressing %lu data blocks (%lu bytes)\n"
				 "        and %lu header blocks (%lu bytes)\n"
				  ,n,n*SDT_BLOCK_LEN,m,m*SHD_BLOCK_LEN); }

	sprintf(fname,"%s.sd$",smb.file);
	tmp_sdt=fopen(fname,"wb");
	sprintf(fname,"%s.sh$",smb.file);
	tmp_shd=fopen(fname,"wb");
	sprintf(fname,"%s.si$",smb.file);
	tmp_sid=fopen(fname,"wb");
	if(!tmp_sdt || !tmp_shd || !tmp_sid) {
		smb_unlocksmbhdr(&smb);
		if(!(smb.status.attr&SMB_HYPERALLOC)) {
			smb_close_ha(&smb);
			smb_close_da(&smb); }
		printf("\n\7!Error opening temp files\n");
		return; }
	setvbuf(tmp_sdt,NULL,_IOFBF,2*1024);
	setvbuf(tmp_shd,NULL,_IOFBF,2*1024);
	setvbuf(tmp_sid,NULL,_IOFBF,2*1024);
	if(!(smb.status.attr&SMB_HYPERALLOC)
		&& (datoffset=(datoffset_t *)LMALLOC(sizeof(datoffset_t)*smb.status.total_msgs))
		==NULL) {
		smb_unlocksmbhdr(&smb);
		smb_close_ha(&smb);
		smb_close_da(&smb);
		fclose(tmp_sdt);
		fclose(tmp_shd);
		fclose(tmp_sid);
		printf("\n\7!Error allocating memory\n");
		return; }
	fseek(smb.shd_fp,0L,SEEK_SET);
	fread(&hdr,1,sizeof(smbhdr_t),smb.shd_fp);
	fwrite(&hdr,1,sizeof(smbhdr_t),tmp_shd);
	fwrite(&(smb.status),1,sizeof(smbstatus_t),tmp_shd);
	for(l=sizeof(smbhdr_t)+sizeof(smbstatus_t);l<smb.status.header_offset;l++) {
		fread(&ch,1,1,smb.shd_fp);			/* copy additional base header records */
		fwrite(&ch,1,1,tmp_shd); }
	fseek(smb.sid_fp,0L,SEEK_SET);
	total=0;
	for(l=0;l<smb.status.total_msgs;l++) {
		printf("%lu of %lu\r",l+1,smb.status.total_msgs);
		if(!fread(&msg.idx,1,sizeof(idxrec_t),smb.sid_fp))
			break;
		if(msg.idx.attr&MSG_DELETE) {
			printf("\nDeleted index.\n");
			continue; }
		i=smb_lockmsghdr(&smb,&msg);
		if(i) {
			printf("\n\7!smb_lockmsghdr returned %d: %s\n",i,smb.last_error);
			continue; }
		i=smb_getmsghdr(&smb,&msg);
		smb_unlockmsghdr(&smb,&msg);
		if(i) {
			printf("\n\7!smb_getmsghdr returned %d: %s\n",i,smb.last_error);
			continue; }
		if(msg.hdr.attr&MSG_DELETE) {
			printf("\nDeleted header.\n");
			smb_freemsgmem(&msg);
			continue; }
		if(msg.expiration.time && msg.expiration.time<=now) {
			printf("\nExpired message.\n");
			smb_freemsgmem(&msg);
			continue; }
		for(m=0;m<datoffsets;m++)
			if(msg.hdr.offset==datoffset[m].old)
				break;
		if(m<datoffsets) {				/* another index pointed to this data */
			printf("duplicate index\n");
			msg.hdr.offset=datoffset[m].new;
			smb_incdat(&smb,datoffset[m].new,smb_getmsgdatlen(&msg),1); }
		else {

			if(!(smb.status.attr&SMB_HYPERALLOC))
				datoffset[datoffsets].old=msg.hdr.offset;

			fseek(smb.sdt_fp,msg.hdr.offset,SEEK_SET);

			m=smb_getmsgdatlen(&msg);
			if(m>16L*1024L*1024L) {
				printf("\n\7!Invalid data length (%lu)\n",m);
				continue; }

			if(!(smb.status.attr&SMB_HYPERALLOC)) {
				datoffset[datoffsets].new=msg.hdr.offset
					=smb_fallocdat(&smb,m,1);
				datoffsets++;
				fseek(tmp_sdt,msg.hdr.offset,SEEK_SET); }
			else {
				fseek(tmp_sdt,0L,SEEK_END);
				msg.hdr.offset=ftell(tmp_sdt); }

			/* Actually copy the data */

			n=smb_datblocks(m);
			for(m=0;m<n;m++) {
				fread(buf,1,SDT_BLOCK_LEN,smb.sdt_fp);
				if(!m && *(ushort *)buf!=XLAT_NONE && *(ushort *)buf!=XLAT_LZH) {
					printf("\nUnsupported translation type (%04X)\n"
						,*(ushort *)buf);
					break; }
				fwrite(buf,1,SDT_BLOCK_LEN,tmp_sdt); }
			if(m<n)
				continue; }

		/* Write the new index entry */
		length=smb_getmsghdrlen(&msg);
		if(smb.status.attr&SMB_HYPERALLOC)
			msg.idx.offset=ftell(tmp_shd);
		else
			msg.idx.offset=smb_fallochdr(&smb,length)+smb.status.header_offset;
		msg.idx.number=msg.hdr.number;
		msg.idx.attr=msg.hdr.attr;
		msg.idx.time=msg.hdr.when_imported.time;
		sprintf(str,"%.128s",msg.subj);
		strlwr(str);
		remove_re(str);
		msg.idx.subj=crc16(str);
		if(smb.status.attr&SMB_EMAIL) {
			if(msg.to_ext)
				msg.idx.to=atoi(msg.to_ext);
			else
				msg.idx.to=0;
			if(msg.from_ext)
				msg.idx.from=atoi(msg.from_ext);
			else
				msg.idx.from=0; }
		else {
			sprintf(str,"%.128s",msg.to);
			strlwr(str);
			msg.idx.to=crc16(str);
			sprintf(str,"%.128s",msg.from);
			strlwr(str);
			msg.idx.from=crc16(str); }
		fwrite(&msg.idx,1,sizeof(idxrec_t),tmp_sid);

		/* Write the new header entry */
		fseek(tmp_shd,msg.idx.offset,SEEK_SET);
		fwrite(&msg.hdr,1,sizeof(msghdr_t),tmp_shd);
		for(n=0;n<msg.hdr.total_dfields;n++)
			fwrite(&msg.dfield[n],1,sizeof(dfield_t),tmp_shd);
		for(n=0;n<msg.total_hfields;n++) {
			fwrite(&msg.hfield[n],1,sizeof(hfield_t),tmp_shd);
			fwrite(msg.hfield_dat[n],1,msg.hfield[n].length,tmp_shd); }
		while(length%SHD_BLOCK_LEN) {	/* pad with NULLs */
			fputc(0,tmp_shd);
			length++; }
		total++;
		smb_freemsgmem(&msg); }

	if(datoffset)
		LFREE(datoffset);
	if(!(smb.status.attr&SMB_HYPERALLOC)) {
		smb_close_ha(&smb);
		smb_close_da(&smb); }

	/* Change *.sh$ into *.shd */
	fclose(smb.shd_fp);
	fclose(tmp_shd);
	sprintf(fname,"%s.shd",smb.file);
	if(remove(fname)!=0)
		printf("\n\7!Error %d removing %s\n",errno,fname);
	sprintf(tmpfname,"%s.sh$",smb.file);
	if(rename(tmpfname,fname)!=0)
		printf("\n\7!Error %d renaming %s to %s\n",errno,tmpfname,fname);


	/* Change *.sd$ into *.sdt */
	fclose(smb.sdt_fp);
	fclose(tmp_sdt);
	sprintf(fname,"%s.sdt",smb.file);
	if(remove(fname)!=0)
		printf("\n\7!Error %d removing %s\n",errno,fname);

	sprintf(tmpfname,"%s.sd$",smb.file);
	if(rename(tmpfname,fname)!=0)
		printf("\n\7!Error %d renaming %s to %s\n",errno,tmpfname,fname);

	/* Change *.si$ into *.sid */
	fclose(smb.sid_fp);
	fclose(tmp_sid);
	sprintf(fname,"%s.sid",smb.file);
	if(remove(fname)!=0)
		printf("\n\7!Error %d removing %s\n",errno,fname);

	sprintf(tmpfname,"%s.si$",smb.file);
	if(rename(tmpfname,fname)!=0)
		printf("\n\7!Error %d renaming %s to %s\n",errno,tmpfname,fname);

	if((i=smb_open(&smb))!=0) {
		printf("\n\7!Error %d reopening %s\n",i,smb.file);
		return; }

	smb.status.total_msgs=total;
	if((i=smb_putstatus(&smb))!=0)
		printf("\n\7!smb_putstatus returned %d: %s\n",i,smb.last_error);
	printf("\nDone.\n\n");
}


/****************************************************************************/
/* Read messages in message base											*/
/****************************************************************************/
void readmsgs(ulong start)
{
	char	HUGE16 *inbuf;
	int 	i,done=0,domsg=1;
	smbmsg_t msg;

	if(start)
		msg.offset=start-1;
	else
		msg.offset=0;
	while(!done) {
		if(domsg) {
			fseek(smb.sid_fp,msg.offset*sizeof(idxrec_t),SEEK_SET);
			if(!fread(&msg.idx,1,sizeof(idxrec_t),smb.sid_fp))
				break;
			i=smb_lockmsghdr(&smb,&msg);
			if(i) {
				printf("\n\7!smb_lockmsghdr returned %d: %s\n",i,smb.last_error);
				break; }
			i=smb_getmsghdr(&smb,&msg);
			if(i) {
				printf("\n\7!smb_getmsghdr returned %d: %s\n",i,smb.last_error);
				break; }

			printf("\n%lu (%lu)\n",msg.hdr.number,msg.offset+1);
			printf("Subj : %s\n",msg.subj);
			printf("To   : %s",msg.to);
			if(msg.to_net.type)
				printf(" (%s)",msg.to_net.type==NET_FIDO
					? faddrtoa(*(fidoaddr_t *)msg.to_net.addr) : msg.to_net.addr);
			printf("\nFrom : %s",msg.from);
			if(msg.from_net.type)
				printf(" (%s)",msg.from_net.type==NET_FIDO
					? faddrtoa(*(fidoaddr_t *)msg.from_net.addr)
						: msg.from_net.addr);
			printf("\nDate : %.24s %s"
				,timestr((time_t*)&msg.hdr.when_written.time)
				,zonestr(msg.hdr.when_written.zone));

			printf("\n\n");

			if((inbuf=smb_getmsgtxt(&smb,&msg,GETMSGTXT_TAILS))!=NULL) {
				printf("%s",inbuf);
				FREE(inbuf); }

			i=smb_unlockmsghdr(&smb,&msg);
			if(i) {
				printf("\n\7!smb_unlockmsghdr returned %d: %s\n",i,smb.last_error);
				break; }
			smb_freemsgmem(&msg); }
		domsg=1;
		printf("\nReading %s (?=Menu): ",smb.file);
		switch(toupper(getch())) {
			case '?':
				printf("\n"
					   "\n"
					   "(R)e-read current message\n"
					   "(L)ist messages\n"
					   "(T)en more titles\n"
					   "(V)iew message headers\n"
					   "(Q)uit\n"
					   "(+/-) Forward/Backward\n"
					   "\n");
				domsg=0;
				break;
			case 'Q':
				printf("Quit\n");
				done=1;
				break;
			case 'R':
				printf("Re-read\n");
				break;
			case '-':
				printf("Backwards\n");
				if(msg.offset)
					msg.offset--;
				break;
			case 'T':
				printf("Ten titles\n");
				listmsgs(msg.offset+2,10);
				msg.offset+=10;
				domsg=0;
				break;
			case 'L':
				printf("List messages\n");
				listmsgs(1,-1);
				domsg=0;
				break;
			case 'V':
				printf("View message headers\n");
				viewmsgs(1,-1);
				domsg=0;
				break;
			case CR:
			case '+':
				printf("Next\n");
				msg.offset++;
				break; } }
}

time_t checktime(void)
{
	struct tm tm;

    memset(&tm,0,sizeof(tm));
    tm.tm_year=94;
    tm.tm_mday=1;
    return(mktime(&tm)-0x2D24BD00L);
}

/***************/
/* Entry point */
/***************/
int main(int argc, char **argv)
{
	char	cmd[128]="",*p,*s;
	char*	to=NULL;
	char*	to_number=NULL;
	char*	to_address=NULL;
	char*	from=NULL;
	char*	from_number=NULL;
	char*	subj=NULL;
	FILE*	fp;
	int		i,j,x,y;
	time_t	t;
	BOOL	create=FALSE;

	setvbuf(stdout,0,_IONBF,0);

	smb.file[0]=0;
	printf("\nSMBUTIL Version %s (%s) SMBLIB %s - Synchronet Message Base "\
		"Utility\n\n"
		,SMBUTIL_VER
#if defined(__linux__)
		,"Linux"
#elif defined(__unix__)
		,"Unix"
#elif defined(__OS2__)
		,"OS/2"
#elif defined(_WIN32)
		,"Win32"
#elif defined(__DOS4G__)
		,"DOS4G"
#elif defined(__FLAT__)
		,"DOS32"
#else
		,"DOS16"
#endif
		,smb_lib_ver()
		);

	putenv("TZ=UCT0");
	tzset();

	if((t=checktime())!=0) {
		fprintf(stderr,"!TIME PROBLEM (%ld)\n",t);
		return(-1);
	}

	for(x=1;x<argc && x>0;x++) {
		if(
#ifndef __unix__
			argv[x][0]=='/' ||		/* for backwards compatibilty */
#endif
			argv[x][0]=='-') {
			for(j=1;argv[x][j];j++)
				switch(toupper(argv[x][j])) {
					case 'A':
						mode|=NOANALYSIS;
						break;
					case 'Z':
						if(isdigit(argv[x][j+1]))
							tzone=atoi(argv[x]+j+1);
						else if(!stricmp(argv[x]+j+1,"EST"))
							tzone=EST;
						else if(!stricmp(argv[x]+j+1,"EDT"))
							tzone=EDT;
						else if(!stricmp(argv[x]+j+1,"CST"))
							tzone=CST;
						else if(!stricmp(argv[x]+j+1,"CDT"))
							tzone=CDT;
						else if(!stricmp(argv[x]+j+1,"MST"))
							tzone=MST;
						else if(!stricmp(argv[x]+j+1,"MDT"))
							tzone=MDT;
						else if(!stricmp(argv[x]+j+1,"PST"))
							tzone=PST;
						else if(!stricmp(argv[x]+j+1,"PDT"))
							tzone=PDT;
						j=strlen(argv[x])-1;
						break;
					case 'C':
						create=TRUE;
						break;
					case 'T':
						to=argv[x]+j+1;
						j=strlen(argv[x])-1;
						break;
					case 'U':
						to_number=argv[x]+j+1;
						j=strlen(argv[x])-1;
						break;
					case 'N':
						to_address=argv[x]+j+1;
						j=strlen(argv[x])-1;
						break;
					case 'F':
						from=argv[x]+j+1;
						j=strlen(argv[x])-1;
						break;
					case 'E':
						from_number=argv[x]+j+1;
						j=strlen(argv[x])-1;
						break;
					case 'S':
						subj=argv[x]+j+1;
						j=strlen(argv[x])-1;
						break;
					default:
						printf("\nUnknown opt '%c'\n",argv[x][j]);
					case '?':
						printf("%s",usage);
						exit(1);
						break; } }
		else {
			if(!cmd[0])
				strcpy(cmd,argv[x]);
			else {
				sprintf(smb.file,"%.64s",argv[x]);
				p=strrchr(smb.file,'.');
				s=strrchr(smb.file,'/');
				if(s==NULL)
					s=strrchr(smb.file,'\\');
				if(p>s) *p=0;
				smb.retry_time=30;
				printf("Opening %s\r\n",smb.file);
				if((i=smb_open(&smb))!=0) {
					printf("\n\7!Error %d (%s) opening %s message base\n"
						,i,smb.last_error,smb.file);
					exit(1); }
				if(!filelength(fileno(smb.shd_fp))) {
					if(!create) {
						printf("Empty\n");
						smb_close(&smb);
						continue; }
					smb.status.max_crcs=0;
					smb.status.max_age=0;
					smb.status.max_msgs=1000;
					smb.status.attr=0;
					if((i=smb_create(&smb))!=0) {
						smb_close(&smb);
						printf("!Error %d (%s) creating %s\n",i,smb.last_error,smb.file);
						continue; 
					} 
				}
				for(y=0;cmd[y];y++)
					switch(toupper(cmd[y])) {
						case 'I':
						case 'E':
						case 'N':
							if(cmd[1]!=0 && cmd[1]!='-')
								fp=fopen(cmd+1,"r");
							else
								fp=stdin;
							i=smb_locksmbhdr(&smb);
							if(i) {
								printf("\n\7!smb_locksmbhdr returned %d: %s\n",i,smb.last_error);
								return(1); }
							postmsg((char)toupper(cmd[y]),to,to_number,to_address,from,from_number,subj,fp);
							fclose(fp);
							y=strlen(cmd)-1;
							break;
						case 'S':
							showstatus();
							break;
						case 'C':
							config();
							break;
						case 'L':
							listmsgs(atol(cmd+1),-1L);
							y=strlen(cmd)-1;
							break;
						case 'P':
							packmsgs(atol(cmd+y+1));
							y=strlen(cmd)-1;
							break;
						case 'R':
							readmsgs(atol(cmd+1));
							y=strlen(cmd)-1;
							break;
						case 'V':
							viewmsgs(atol(cmd+1),-1L);
							y=strlen(cmd)-1;
							break;
						case 'M':
							maint();
							break;
						default:
							printf("%s",usage);
							break; }
				smb_close(&smb); } } }
	if(!cmd[0])
		printf("%s",usage);
	return(0);
}
