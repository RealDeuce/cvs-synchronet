/* str_list.c */

/* Functions to deal with NULL-terminated string lists */

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

#include <stdlib.h>		/* malloc and qsort */
#include <string.h>		/* strtok */
#include "genwrap.h"	/* stricmp */
#include "str_list.h"

str_list_t strListInit()
{
	str_list_t list;

	if((list=(str_list_t)malloc(sizeof(char*)))==NULL)
		return(NULL);

	list[0]=NULL;	/* terminated by default */
	return(list);
}

size_t strListCount(const str_list_t list)
{
	size_t i;

	if(list==NULL)
		return(0);

	for(i=0; list[i]!=NULL; i++)
		;

	return(i);
}

static char* str_list_append(str_list_t* list, char* str, size_t index)
{
	str_list_t lp;

	if((lp=(str_list_t)realloc(*list,sizeof(char*)*(index+2)))==NULL)
		return(NULL);

	*list=lp;
	lp[index++]=str;
	lp[index]=NULL;	/* terminate list */

	return(str);
}

static char* str_list_insert(str_list_t* list, char* str, size_t index)
{
	size_t	i;
	size_t	count;
	str_list_t lp;

	count = strListCount(*list);
	if(index > count)	/* invalid index, do nothing */
		return(NULL);

	count++;
	if((lp=(str_list_t)realloc(*list,sizeof(char*)*(count+1)))==NULL)
		return(NULL);

	*list=lp;
	for(i=count; i>index; i--)
		lp[i]=lp[i-1];
	lp[index]=str;

	return(str);
}

char* strListRemove(str_list_t* list, size_t index)
{
	char*	str;
	size_t	i;
	size_t	count;
	str_list_t lp;

	count = strListCount(*list);

	if(index==STR_LIST_LAST_INDEX && count)
		index = count-1;

	if(index >= count)	/* invalid index, do nothing */
		return(NULL);

	count--;
	if((lp=(str_list_t)realloc(*list,sizeof(char*)*(count+1)))==NULL)
		return(NULL);

	*list=lp;
	str=lp[index];
	for(i=index; i<count; i++)
		lp[i]=lp[i+1];
	lp[count]=NULL;

	return(str);
}

BOOL strListDelete(str_list_t* list, size_t index)
{
	char*	str;

	if((str=strListRemove(list, index))==NULL)
		return(FALSE);

	free(str);

	return(TRUE);
}

char* strListReplace(const str_list_t list, size_t index, const char* str)
{
	char*	buf;
	size_t	count;

	count = strListCount(list);

	if(index==STR_LIST_LAST_INDEX && count)
		index = count-1;

	if(index >= count)	/* invalid index, do nothing */
		return(NULL);

	if((buf=(char*)realloc(list[index],strlen(str)+1))==NULL)
		return(NULL);

	list[index]=buf;
	strcpy(buf,str);

	return(buf);
}

char* strListAppend(str_list_t* list, const char* str, size_t index)
{
	char* buf;

	if((buf=(char*)malloc(strlen(str)+1))==NULL)
		return(NULL);

	strcpy(buf,str);

	if(index==STR_LIST_LAST_INDEX)
		index=strListCount(*list);

	return(str_list_append(list,buf,index));
}

size_t	strListAppendList(str_list_t* list, const str_list_t add_list)
{
	size_t	i;
	size_t	count;

	count=strListCount(*list);
	for(i=0; add_list[i]!=NULL; i++)
		strListAppend(list,add_list[i],count++);

	return(count);
}

char* strListInsert(str_list_t* list, const char* str, size_t index)
{
	char* buf;

	if((buf=(char*)malloc(strlen(str)+1))==NULL)
		return(NULL);

	strcpy(buf,str);

	return(str_list_insert(list,buf,index));
}

size_t strListInsertList(str_list_t* list, const str_list_t add_list, size_t index)
{
	size_t	i;


	for(i=0; add_list[i]!=NULL; i++)
		if(strListInsert(list,add_list[i],index++)==NULL)
			break;

	return(i);
}

str_list_t strListSplit(str_list_t* lp, char* str, const char* delimit)
{
	size_t	count;
	char*	token;
	str_list_t	list;

	if(lp==NULL) {
		if((list = strListInit())==NULL)
			return(0);
		lp=&list;
		count=0;
	} else
		count=strListCount(*lp);

	for(token = strtok(str, delimit); token!=NULL; token=strtok(NULL, delimit))
		if(strListAppend(lp, token, count++)==NULL)
			break;

	return(*lp);
}

str_list_t strListSplitCopy(str_list_t* list, const char* str, const char* delimit)
{
	char*	buf;

	if((buf=malloc(strlen(str)+1))==NULL)
		return(NULL);

	strcpy(buf,str);

	*list=strListSplit(list,buf,delimit);

	free(buf);

	return(*list);
}

size_t	strListMerge(str_list_t* list, str_list_t add_list)
{
	size_t	i;
	size_t	count;

	count=strListCount(*list);
	for(i=0; add_list[i]!=NULL; i++)
		str_list_append(list,add_list[i],count++);

	return(i);
}

static int strListCompareAlpha(const void *arg1, const void *arg2)
{
   return stricmp(*(char**)arg1, *(char**)arg2);
}

static int strListCompareAlphaReverse(const void *arg1, const void *arg2)
{
   return stricmp(*(char**)arg2, *(char**)arg1);
}

static int strListCompareAlphaCase(const void *arg1, const void *arg2)
{
   return strcmp(*(char**)arg1, *(char**)arg2);
}

static int strListCompareAlphaCaseReverse(const void *arg1, const void *arg2)
{
   return strcmp(*(char**)arg2, *(char**)arg1);
}

void strListSortAlpha(str_list_t list)
{
	qsort(list,strListCount(list),sizeof(char*),strListCompareAlpha);
}

void strListSortAlphaReverse(str_list_t list)
{
	qsort(list,strListCount(list),sizeof(char*),strListCompareAlphaReverse);
}

void strListSortAlphaCase(str_list_t list)
{
	qsort(list,strListCount(list),sizeof(char*),strListCompareAlphaCase);
}

void strListSortAlphaCaseReverse(str_list_t list)
{
	qsort(list,strListCount(list),sizeof(char*),strListCompareAlphaCaseReverse);
}

void strListFreeStrings(str_list_t list)
{
	size_t i;

	if(list!=NULL) {
		for(i=0; list[i]!=NULL; i++)
			free(list[i]);
		list[0]=NULL;	/* terminate */
	}
}

void strListFree(str_list_t* list)
{
	if(*list!=NULL) {
		strListFreeStrings(*list);
		free(*list);
	}
}

str_list_t strListReadFile(FILE* fp, str_list_t* lp, size_t max_line_len)
{
	char*		buf=NULL;
	size_t		count;
	str_list_t	list;

	if(max_line_len<1)
		max_line_len=2048;

	if(lp==NULL) {
		if((list = strListInit())==NULL)
			return(NULL);
		lp=&list;
	}

	count = strListCount(*lp);
	while(!feof(fp)) {
		if(buf==NULL && (buf=malloc(max_line_len+1))==NULL)
			return(NULL);
		
		if(fgets(buf,max_line_len+1,fp)==NULL)
			break;
		strListAppend(lp, buf, count++);
	}

	if(buf!=NULL)
		free(buf);

	return(*lp);
}

size_t strListWriteFile(FILE* fp, const str_list_t list, const char* separator)
{
	size_t		i;

	if(list==NULL)
		return(0);

	for(i=0; list[i]!=NULL; i++) {
		if(fputs(list[i],fp)==EOF)
			break;
		if(separator!=NULL && fputs(separator,fp)==EOF)
			break;
	}
	
	return(i);
}
