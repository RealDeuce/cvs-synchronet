/* ini_file.c */

/* Functions to parse ini files */

/* $Id$ */

/****************************************************************************
 * @format.tab-size 4		(Plain Text/Source Code File Header)			*
 * @format.use-tabs true	(see http://www.synchro.net/ptsc_hdr.html)		*
 *																			*
 * Copyright 2004 Rob Swindell - http://www.synchro.net/copyright.html		*
 *																			*
 * This library is free software; you can redistribute it and/or			*
 * modify it under the terms of the GNU Lesser General Public License		*
 * as published by the Free Software Foundation; either version 2			*
 * of the License, or (at your option) any later version.					*
 * See the GNU Lesser General Public License for more details: lgpl.txt or	*
 * http://www.fsf.org/copyleft/lesser.html									*
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

#include <stdlib.h>		/* strtol */
#include <string.h>		/* strlen */
#include <ctype.h>		/* isdigit */
#if !defined(NO_SOCKET_SUPPORT)
	#include "sockwrap.h"	/* inet_addr */
#endif
#include "dirwrap.h"	/* fexist */
#include "filewrap.h"	/* chsize */
#include "ini_file.h"

/* Maximum length of entire line, includes '\0' */
#define INI_MAX_LINE_LEN		(INI_MAX_VALUE_LEN*2)
#define INI_COMMENT_CHAR		';'
#define INI_OPEN_SECTION_CHAR	'['
#define INI_CLOSE_SECTION_CHAR	']'
#define INI_NEW_SECTION			((char*)~0)

static ini_style_t default_style;

static char* section_name(char* p)
{
	char*	tp;

	SKIP_WHITESPACE(p);
	if(*p!=INI_OPEN_SECTION_CHAR)
		return(NULL);
	p++;
	SKIP_WHITESPACE(p);
	tp=strrchr(p,INI_CLOSE_SECTION_CHAR);
	if(tp==NULL)
		return(NULL);
	*tp=0;
	truncsp(p);

	return(p);
}

static BOOL seek_section(FILE* fp, const char* section)
{
	char*	p;
	char	str[INI_MAX_LINE_LEN];

	rewind(fp);

	if(section==ROOT_SECTION)
		return(TRUE);

	while(!feof(fp)) {
		if(fgets(str,sizeof(str),fp)==NULL)
			break;
		if((p=section_name(str))==NULL)
			continue;
		if(stricmp(p,section)==0)
			return(TRUE);
	}
	return(FALSE);
}

static size_t find_section_index(str_list_t list, const char* section)
{
	char*	p;
	char	str[INI_MAX_VALUE_LEN];
	size_t	i;

	for(i=0; list[i]!=NULL; i++) {
		SAFECOPY(str,list[i]);
		if((p=section_name(str))!=NULL && stricmp(p,section)==0)
			return(i);
	}

	return(i);
}

static size_t find_section(str_list_t list, const char* section)
{
	size_t	i;

	if(section==ROOT_SECTION)
		return(0);

	i=find_section_index(list,section);
	if(list[i]!=NULL)
		i++;
	return(i);
}

static char* key_name(char* p, char** vp)
{
	char* equal;
	char* colon;

    *vp=NULL;
    
	if(p==NULL)
		return(NULL);

	/* Parse value name */
	SKIP_WHITESPACE(p);
	if(*p==INI_COMMENT_CHAR)
		return(NULL);
	if(*p==INI_OPEN_SECTION_CHAR)
		return(INI_NEW_SECTION);
	equal=strchr(p,'=');
	colon=strchr(p,':');
	if(colon==NULL || (equal!=NULL && equal<colon)) {
		*vp=equal;
		colon=NULL;
	} else
		*vp=colon;

	if(*vp==NULL)
		return(NULL);

	*(*vp)=0;
	truncsp(p);

	/* Parse value */
	(*vp)++;
	SKIP_WHITESPACE(*vp);
	if(colon!=NULL)			
		truncnl(*vp);		/* "key : value" - truncate new-line chars only */
	else	
		truncsp(*vp);		/* "key = value" - truncate all white-space chars */

	return(p);
}

static char* read_value(FILE* fp, const char* section, const char* key, char* value)
{
	char*	p;
	char*	vp=NULL;
	char	str[INI_MAX_LINE_LEN];

	if(fp==NULL)
		return(NULL);

	if(!seek_section(fp,section))
		return(NULL);

	while(!feof(fp)) {
		if(fgets(str,sizeof(str),fp)==NULL)
			break;
		if((p=key_name(str,&vp))==NULL)
			continue;
		if(p==INI_NEW_SECTION)
			break;
		if(stricmp(p,key)!=0)
			continue;
		if(vp==NULL)
			break;
		/* key found */
		sprintf(value,"%.*s",INI_MAX_VALUE_LEN-1,vp);
		return(value);
	}

	return(NULL);
}

static size_t get_value(str_list_t list, const char* section, const char* key, char* value)
{
	char	str[INI_MAX_LINE_LEN];
	char*	p;
	char*	vp;
	size_t	i;

	value[0]=0;
	for(i=find_section(list, section); list[i]!=NULL; i++) {
		SAFECOPY(str, list[i]);
		if((p=key_name(str,&vp))==NULL)
			continue;
		if(p==INI_NEW_SECTION)
			break;
		if(stricmp(p,key)!=0)
			continue;
		sprintf(value,"%.*s",INI_MAX_VALUE_LEN-1,vp);
		return(i);
	}

	return(i);
}

BOOL iniSectionExists(str_list_t* list, const char* section)
{
	size_t	i;

	if(section==ROOT_SECTION)
		return(TRUE);

	i=find_section_index(*list,section);
	return((*list)[i]!=NULL);
}

BOOL iniKeyExists(str_list_t* list, const char* section, const char* key)
{
	char	val[INI_MAX_VALUE_LEN];
	size_t	i;

	i=get_value(*list, section, key, val);

	if((*list)[i]==NULL || *(*list)[i]==INI_OPEN_SECTION_CHAR)
		return(FALSE);

	return(TRUE);
}

BOOL iniValueExists(str_list_t* list, const char* section, const char* key)
{
	char	val[INI_MAX_VALUE_LEN];

	get_value(*list, section, key, val);

	return(val[0]!=0);
}

BOOL iniRemoveKey(str_list_t* list, const char* section, const char* key)
{
	char	val[INI_MAX_VALUE_LEN];
	size_t	i;

	i=get_value(*list, section, key, val);

	if((*list)[i]==NULL || *(*list)[i]==INI_OPEN_SECTION_CHAR)
		return(FALSE);

	return(strListDelete(list,i));
}

BOOL iniRemoveValue(str_list_t* list, const char* section, const char* key)
{
	char	val[INI_MAX_VALUE_LEN];
	size_t	i;
	char*	vp=NULL;

	i=get_value(*list, section, key, val);

    key_name((*list)[i], &vp);
	if(vp==NULL)
		return(FALSE);

	*vp=0;	/* Terminate string at beginning of value */
	return(TRUE);
}

BOOL iniRemoveSection(str_list_t* list, const char* section)
{
	size_t	i;

	i=find_section_index(*list,section);
	if((*list)[i]==NULL)	/* not found */
		return(FALSE);
	do {
		strListDelete(list,i);
	} while((*list)[i]!=NULL && *(*list)[i]!=INI_OPEN_SECTION_CHAR);

	return(TRUE);
}

BOOL iniRenameSection(str_list_t* list, const char* section, const char* newname)
{
	char	str[INI_MAX_LINE_LEN];
	size_t	i;

	if(section==ROOT_SECTION)
		return(FALSE);

	i=find_section_index(*list,newname);
	if((*list)[i]!=NULL)	/* duplicate */
		return(FALSE);

	i=find_section_index(*list,section);
	if((*list)[i]==NULL)	/* not found */
		return(FALSE);

	SAFEPRINTF(str,"[%s]",newname);
	return(strListReplace(*list, i, str)!=NULL);
}

size_t iniAddSection(str_list_t* list, const char* section
					,ini_style_t* style)
{
	char	str[INI_MAX_LINE_LEN];
	size_t	i;

	if(section==ROOT_SECTION)
		return(0);

	i=find_section_index(*list, section);
	if((*list)[i]==NULL) {
		if(style==NULL)
			style=&default_style;
		if(style->section_separator!=NULL)
			strListAppend(list, style->section_separator, i++);
		SAFEPRINTF(str,"[%s]",section);
		strListAppend(list, str, i);
	}

	return(i);
}

char* iniSetString(str_list_t* list, const char* section, const char* key, const char* value
				 ,ini_style_t* style)
{
	char	str[INI_MAX_LINE_LEN];
	char	curval[INI_MAX_VALUE_LEN];
	size_t	i;

	if(style==NULL)
		style=&default_style;

	iniAddSection(list, section, style);

	if(key==NULL)
		return(NULL);
	if(style->key_prefix==NULL)
		style->key_prefix="";
	if(style->value_separator==NULL)
		style->value_separator="=";
	if(value==NULL)
		value="";
	safe_snprintf(str, sizeof(str), "%s%-*s%s%s"
		, style->key_prefix, style->key_len, key, style->value_separator, value);
	i=get_value(*list, section, key, curval);
	if((*list)[i]==NULL || *(*list)[i]==INI_OPEN_SECTION_CHAR) {
        while(i && *(*list)[i-1]==0) i--;   /* Insert before blank lines, not after */
		return strListInsert(list, str, i);
    }

	if(strcmp(curval,value)==0)
		return((*list)[i]);	/* no change */

	return strListReplace(*list, i, str);
}

char* iniSetInteger(str_list_t* list, const char* section, const char* key, long value
					,ini_style_t* style)
{
	char	str[INI_MAX_VALUE_LEN];

	SAFEPRINTF(str,"%ld",value);
	return iniSetString(list, section, key, str, style);
}

char* iniSetShortInt(str_list_t* list, const char* section, const char* key, ushort value
					,ini_style_t* style)
{
	char	str[INI_MAX_VALUE_LEN];

	SAFEPRINTF(str,"%hu",value);
	return iniSetString(list, section, key, str, style);
}

char* iniSetHexInt(str_list_t* list, const char* section, const char* key, ulong value
					,ini_style_t* style)
{
	char	str[INI_MAX_VALUE_LEN];

	SAFEPRINTF(str,"0x%lx",value);
	return iniSetString(list, section, key, str, style);
}

char* iniSetFloat(str_list_t* list, const char* section, const char* key, double value
					,ini_style_t* style)
{
	char	str[INI_MAX_VALUE_LEN];

	SAFEPRINTF(str,"%g",value);
	return iniSetString(list, section, key, str, style);
}

#if !defined(NO_SOCKET_SUPPORT)
char* iniSetIpAddress(str_list_t* list, const char* section, const char* key, ulong value
					,ini_style_t* style)
{
	struct in_addr in_addr;
	in_addr.s_addr=htonl(value);
	return iniSetString(list, section, key, inet_ntoa(in_addr), style);
}
#endif

char* iniSetBool(str_list_t* list, const char* section, const char* key, BOOL value
					,ini_style_t* style)
{
	return iniSetString(list, section, key, value ? "true":"false", style);
}

char* iniSetBitField(str_list_t* list, const char* section, const char* key
					 ,ini_bitdesc_t* bitdesc, ulong value, ini_style_t* style)
{
	char	str[INI_MAX_VALUE_LEN];
	int		i;

	if(style==NULL)
		style=&default_style;
	if(style->bit_separator==NULL)
		style->bit_separator="|";
	str[0]=0;
	for(i=0;bitdesc[i].name;i++) {
		if((value&bitdesc[i].bit)==0)
			continue;
		if(str[0])
			strcat(str,style->bit_separator);
		strcat(str,bitdesc[i].name);
		value&=~bitdesc[i].bit;
	}
	if(value) {	/* left over bits? */
		if(str[0])
			strcat(str,style->bit_separator);
		sprintf(str+strlen(str), "0x%lX", value);
	}
	return iniSetString(list, section, key, str, style);
}

char* iniSetStringList(str_list_t* list, const char* section, const char* key
					,const char* sep, str_list_t val_list, ini_style_t* style)
{
	char	value[INI_MAX_VALUE_LEN];
	size_t	i;

	value[0]=0;

	if(sep==NULL)
		sep=",";

	if(val_list!=NULL)
		for(i=0; val_list[i]!=NULL; i++) {
			if(value[0])
				strcat(value,sep);
			strcat(value,val_list[i]);
		}

	return iniSetString(list, section, key, value, style);
}

char* iniReadString(FILE* fp, const char* section, const char* key, const char* deflt, char* value)
{
	if(read_value(fp,section,key,value)==NULL || *value==0 /* blank */) {
		if(deflt==NULL)
			return(NULL);
		sprintf(value,"%.*s",INI_MAX_VALUE_LEN-1,deflt);
	}

	return(value);
}

char* iniGetString(str_list_t* list, const char* section, const char* key, const char* deflt, char* value)
{
	get_value(*list, section, key, value);

	if(*value==0 /* blank */) {
		if(deflt==NULL)
			return(NULL);
		sprintf(value,"%.*s",INI_MAX_VALUE_LEN-1,deflt);
	}

	return(value);
}

static str_list_t splitList(char* list, const char* sep)
{
	char*		token;
	ulong		items=0;
	str_list_t	lp;

	if((lp=strListInit())==NULL)
		return(NULL);

	if(sep==NULL)
		sep=",";

	token=strtok(list,sep);
	while(token!=NULL) {
		truncsp(token);
		if(strListAppend(&lp,token,items++)==NULL)
			break;
		token=strtok(NULL,sep);
	}

	return(lp);
}

str_list_t iniReadStringList(FILE* fp, const char* section, const char* key
						 ,const char* sep, const char* deflt)
{
	char*	value;
	char	buf[INI_MAX_VALUE_LEN];
	char	list[INI_MAX_VALUE_LEN];

	if((value=read_value(fp,section,key,buf))==NULL || *value==0 /* blank */)
		value=(char*)deflt;

	SAFECOPY(list,value);

	return(splitList(list,sep));
}

str_list_t iniGetStringList(str_list_t* list, const char* section, const char* key
						 ,const char* sep, const char* deflt)
{
	char	buf[INI_MAX_VALUE_LEN];
	char*	value=buf;

	get_value(*list, section, key, value);

	if(*value==0 /* blank */)
		value=(char*)deflt;

	SAFECOPY(buf,value);

	return(splitList(buf,sep));
}

void* iniFreeStringList(str_list_t list)
{
	strListFree(&list);
	return(list);
}

void* iniFreeNamedStringList(named_string_t** list)
{
	ulong	i;

	if(list==NULL)
		return(NULL);

	for(i=0;list[i]!=NULL;i++) {
		if(list[i]->name!=NULL)
			free(list[i]->name);
		if(list[i]->value!=NULL)
			free(list[i]->value);
		free(list[i]);
	}

	free(list);
	return(NULL);
}

str_list_t iniReadSectionList(FILE* fp, const char* prefix)
{
	char*	p;
	char	str[INI_MAX_LINE_LEN];
	ulong	items=0;
	str_list_t	lp;

	if((lp=strListInit())==NULL)
		return(NULL);

	if(fp==NULL)
		return(lp);

	rewind(fp);

	while(!feof(fp)) {
		if(fgets(str,sizeof(str),fp)==NULL)
			break;
		if((p=section_name(str))==NULL)
			continue;
		if(prefix!=NULL)
			if(strnicmp(p,prefix,strlen(prefix))!=0)
				continue;
		if(strListAppend(&lp,p,items++)==NULL)
			break;
	}

	return(lp);
}

str_list_t iniGetSectionList(str_list_t* list, const char* prefix)
{
	char*	p;
	char	str[INI_MAX_LINE_LEN];
	ulong	i,items=0;
	str_list_t	lp;

	if((lp=strListInit())==NULL)
		return(NULL);

	if(list==NULL)
		return(lp);

	for(i=0;list[i];i++) {
		SAFECOPY(str,list[i]);
		if((p=section_name(str))==NULL)
			continue;
		if(prefix!=NULL)
			if(strnicmp(p,prefix,strlen(prefix))!=0)
				continue;
		if(strListAppend(&lp,p,items++)==NULL)
			break;
	}

	return(lp);
}

str_list_t iniReadKeyList(FILE* fp, const char* section)
{
	char*	p;
	char*	vp;
	char	str[INI_MAX_LINE_LEN];
	ulong	items=0;
	str_list_t	lp;

	if((lp=strListInit())==NULL)
		return(NULL);

	if(fp==NULL)
		return(lp);

	rewind(fp);

	if(!seek_section(fp,section))
		return(lp);

	while(!feof(fp)) {
		if(fgets(str,sizeof(str),fp)==NULL)
			break;
		if((p=key_name(str,&vp))==NULL)
			continue;
		if(p==INI_NEW_SECTION)
			break;
		if(strListAppend(&lp,p,items++)==NULL)
			break;
	}

	return(lp);
}

str_list_t iniGetKeyList(str_list_t* list, const char* section)
{
	char*	p;
	char*	vp;
	char	str[INI_MAX_LINE_LEN];
	ulong	i,items=0;
	str_list_t	lp;

	if((lp=strListInit())==NULL)
		return(NULL);

	if(list==NULL)
		return(lp);

	for(i=find_section(*list,section);list[i]==NULL;i++) {
		SAFECOPY(str,list[i]);
		if((p=key_name(str,&vp))==NULL)
			continue;
		if(p==INI_NEW_SECTION)
			break;
		if(strListAppend(&lp,p,items++)==NULL)
			break;
	}

	return(lp);
}


named_string_t**
iniReadNamedStringList(FILE* fp, const char* section)
{
	char*	name;
	char*	value;
	char	str[INI_MAX_LINE_LEN];
	ulong	items=0;
	named_string_t** lp;
	named_string_t** np;

	if((lp=(named_string_t**)malloc(sizeof(named_string_t*)))==NULL)
		return(NULL);

	*lp=NULL;

	if(fp==NULL)
		return(lp);

	rewind(fp);

	if(!seek_section(fp,section))
		return(lp);

	while(!feof(fp)) {
		if(fgets(str,sizeof(str),fp)==NULL)
			break;
		if((name=key_name(str,&value))==NULL)
			continue;
		if(name==INI_NEW_SECTION)
			break;
		if((np=(named_string_t**)realloc(lp,sizeof(named_string_t*)*(items+2)))==NULL)
			break;
		lp=np;
		if((lp[items]=(named_string_t*)malloc(sizeof(named_string_t)))==NULL)
			break;
		if((lp[items]->name=strdup(name))==NULL)
			break;
		if((lp[items]->value=strdup(value))==NULL)
			break;
		items++;
	}

	lp[items]=NULL;	/* terminate list */

	return(lp);
}

named_string_t**
iniGetNamedStringList(str_list_t* list, const char* section)
{
	char*	name;
	char*	value;
	char	str[INI_MAX_LINE_LEN];
	ulong	i,items=0;
	named_string_t** lp;
	named_string_t** np;

	if((lp=(named_string_t**)malloc(sizeof(named_string_t*)))==NULL)
		return(NULL);

	*lp=NULL;

	if(list==NULL)
		return(lp);

	for(i=find_section(*list,section);list[i]!=NULL;i++) {
		SAFECOPY(str,list[i]);
		if((name=key_name(str,&value))==NULL)
			continue;
		if(name==INI_NEW_SECTION)
			break;
		if((np=(named_string_t**)realloc(lp,sizeof(named_string_t*)*(items+2)))==NULL)
			break;
		lp=np;
		if((lp[items]=(named_string_t*)malloc(sizeof(named_string_t)))==NULL)
			break;
		if((lp[items]->name=strdup(name))==NULL)
			break;
		if((lp[items]->value=strdup(value))==NULL)
			break;
		items++;
	}

	lp[items]=NULL;	/* terminate list */

	return(lp);
}


/* These functions read a single key of the specified type */

long iniReadInteger(FILE* fp, const char* section, const char* key, long deflt)
{
	char*	value;
	char	buf[INI_MAX_VALUE_LEN];

	if((value=read_value(fp,section,key,buf))==NULL)
		return(deflt);

	if(*value==0)		/* blank value */
		return(deflt);

	return(strtol(value,NULL,0));
}

long iniGetInteger(str_list_t* list, const char* section, const char* key, long deflt)
{
	char	value[INI_MAX_VALUE_LEN];

	get_value(*list, section, key, value);

	if(*value==0)		/* blank value */
		return(deflt);

	return(strtol(value,NULL,0));
}

ushort iniReadShortInt(FILE* fp, const char* section, const char* key, ushort deflt)
{
	return((ushort)iniReadInteger(fp, section, key, deflt));
}

ushort iniGetShortInt(str_list_t* list, const char* section, const char* key, ushort deflt)
{
	return((ushort)iniGetInteger(list, section, key, deflt));
}

#if !defined(NO_SOCKET_SUPPORT)

static ulong parseIpAddress(const char* value)
{
	if(strchr(value,'.')==NULL)
		return(strtol(value,NULL,0));

	return(ntohl(inet_addr(value)));
}

ulong iniReadIpAddress(FILE* fp, const char* section, const char* key, ulong deflt)
{
	char	buf[INI_MAX_VALUE_LEN];
	char*	value;

	if((value=read_value(fp,section,key,buf))==NULL)
		return(deflt);

	if(*value==0)		/* blank value */
		return(deflt);

	return(parseIpAddress(value));
}

ulong iniGetIpAddress(str_list_t* list, const char* section, const char* key, ulong deflt)
{
	char	value[INI_MAX_VALUE_LEN];

	get_value(*list, section, key, value);

	if(*value==0)		/* blank value */
		return(deflt);

	return(parseIpAddress(value));
}

#endif	/* !NO_SOCKET_SUPPORT */

double iniReadFloat(FILE* fp, const char* section, const char* key, double deflt)
{
	char	buf[INI_MAX_VALUE_LEN];
	char*	value;

	if((value=read_value(fp,section,key,buf))==NULL)
		return(deflt);

	if(*value==0)		/* blank value */
		return(deflt);

	return(atof(value));
}

double iniGetFloat(str_list_t* list, const char* section, const char* key, double deflt)
{
	char	value[INI_MAX_VALUE_LEN];

	get_value(*list, section, key, value);

	if(*value==0)		/* blank value */
		return(deflt);

	return(atof(value));
}

static BOOL parseBool(const char* value)
{
	if(!stricmp(value,"TRUE"))
		return(TRUE);
	if(!stricmp(value,"FALSE"))
		return(FALSE);

	return(strtol(value,NULL,0));
}

BOOL iniReadBool(FILE* fp, const char* section, const char* key, BOOL deflt)
{
	char	buf[INI_MAX_VALUE_LEN];
	char*	value;

	if((value=read_value(fp,section,key,buf))==NULL)
		return(deflt);

	if(*value==0)		/* blank value */
		return(deflt);

	return(parseBool(value));
}

BOOL iniGetBool(str_list_t* list, const char* section, const char* key, BOOL deflt)
{
	char	value[INI_MAX_VALUE_LEN];

	get_value(*list, section, key, value);

	if(*value==0)		/* blank value */
		return(deflt);

	return(parseBool(value));
}

static ulong parseBitField(char* value, ini_bitdesc_t* bitdesc)
{
	int		i;
	char*	p;
	char*	tp;
	ulong	v=0;

	for(p=value;*p;) {
		tp=strchr(p,'|');
		if(tp!=NULL)
			*tp=0;
		truncsp(p);

		for(i=0;bitdesc[i].name;i++)
			if(!stricmp(bitdesc[i].name,p))
				break;

		if(bitdesc[i].name)
			v|=bitdesc[i].bit;
		else
			v|=strtoul(p,NULL,0);

		if(tp==NULL)
			break;

		p=tp+1;
		SKIP_WHITESPACE(p);
	}

	return(v);
}

ulong iniReadBitField(FILE* fp, const char* section, const char* key, 
						ini_bitdesc_t* bitdesc, ulong deflt)
{
	char*	value;
	char	buf[INI_MAX_VALUE_LEN];

	if((value=read_value(fp,section,key,buf))==NULL)
		return(deflt);

	return(parseBitField(value,bitdesc));
}

ulong iniGetBitField(str_list_t* list, const char* section, const char* key, 
						ini_bitdesc_t* bitdesc, ulong deflt)
{
	char	value[INI_MAX_VALUE_LEN];

	get_value(*list, section, key, value);

	if(*value==0)		/* blank value */
		return(deflt);

	return(parseBitField(value,bitdesc));
}

FILE* iniOpenFile(const char* fname)
{
	char* mode="r+";

	if(!fexist(fname))
		mode="w+";

	return(fopen(fname,mode));
}

BOOL iniCloseFile(FILE* fp)
{
	return(fclose(fp)==0);
}

str_list_t iniReadFile(FILE* fp)
{
	size_t		i;
	str_list_t	list;
	
	rewind(fp);

	list = strListReadFile(fp, NULL, INI_MAX_LINE_LEN);
	if(list!=NULL) {
		/* truncate new-line chars off end of strings */
		for(i=0; list[i]!=NULL; i++)
			truncnl(list[i]);
	}

	return(list);
}

BOOL iniWriteFile(FILE* fp, const str_list_t list)
{
	size_t		count;

	rewind(fp);

	if(chsize(fileno(fp),0)!=0)	/* truncate */
		return(FALSE);

	count = strListWriteFile(fp,list,"\n");

	return(count == strListCount(list));
}
