/* js_msg_area.c */

/* Synchronet JavaScript "Message Area" Object */

/* $Id: js_msg_area.c,v 1.51 2006/12/28 02:45:27 rswindell Exp $ */

/****************************************************************************
 * @format.tab-size 4		(Plain Text/Source Code File Header)			*
 * @format.use-tabs true	(see http://www.synchro.net/ptsc_hdr.html)		*
 *																			*
 * Copyright 2006 Rob Swindell - http://www.synchro.net/copyright.html		*
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

#include "sbbs.h"

#ifdef JAVASCRIPT

enum {	/* msg_area Object Properties */
	 PROP_MAX_QWK_MSGS
};

#ifdef BUILD_JSDOCS

static char* msg_grp_prop_desc[] = {
	 "index into grp_list array (or -1 if not in array) <i>(introduced in v3.12)</i>"
	,"unique number for this message group"
	,"group name"
	,"group description"
	,"group access requirements"
	,NULL
};

static char* msg_area_prop_desc[] = {

	 "index into sub_list array (or -1 if not in array) <i>(introduced in v3.12)</i>"
	,"group's index into grp_list array <i>(introduced in v3.12)</i>"
	,"unique number for this sub-board"
	,"group number"
	,"group name <i>(introduced in v3.12)</i>"
	,"sub-board internal code"
	,"sub-board name"
	,"sub-board description"
	,"sub-board QWK name"
	,"newsgroup name (as configured or dymamically generated)"
	,"sub-board access requirements"
	,"sub-board reading requirements"
	,"sub-board posting requirements"
	,"sub-board operator requirements"
	,"sub-board moderated-user requirements (if non-blank)"
	,"sub-board data storage location"
	,"FidoNet origin line"
	,"QWK Network tagline"
	,"toggle options (bitfield)"
	,"index into message scan configuration/pointer file"
	,"QWK conference number"
	,"configured maximum number of message CRCs to store (for dupe checking)"
	,"configured maximum number of messages before purging"
	,"configured maximum age (in days) of messages before expiration"
	/* Insert here */
	,"user has sufficient access to read messages"
	,"user has sufficient access to post messages"
	,"user has operator access to this message area"
	,"user's posts are moderated"
	,"user's current new message scan pointer (highest-read message number)"
	,"user's message scan configuration (bitfield) see <tt>SCAN_CFG_*</tt> in <tt>sbbsdefs.js</tt> for valid bits"
	,"user's last-read message number"
	,NULL
};
#endif

BOOL DLLCALL js_CreateMsgAreaProperties(JSContext* cx, scfg_t* cfg, JSObject* subobj, uint subnum)
{
	char		str[128];
	int			c;
	JSString*	js_str;
	jsval		val;
	sub_t*		sub;

	if(subnum==INVALID_SUB || subnum>=cfg->total_subs)
		return(FALSE);

	sub=cfg->sub[subnum];

	if(!JS_DefineProperty(cx, subobj, "number", INT_TO_JSVAL(subnum)
		,NULL,NULL,JSPROP_ENUMERATE|JSPROP_READONLY))
		return(FALSE);

	if(!JS_DefineProperty(cx, subobj, "grp_number", INT_TO_JSVAL(sub->grp)
		,NULL,NULL,JSPROP_ENUMERATE|JSPROP_READONLY))
		return(FALSE);

	if((js_str=JS_NewStringCopyZ(cx, cfg->grp[sub->grp]->sname))==NULL)
		return(FALSE);
	if(!JS_DefineProperty(cx, subobj, "grp_name", STRING_TO_JSVAL(js_str)
		,NULL,NULL,JSPROP_ENUMERATE|JSPROP_READONLY))
		return(FALSE);

	if((js_str=JS_NewStringCopyZ(cx, sub->code))==NULL)
		return(FALSE);
	if(!JS_DefineProperty(cx, subobj, "code", STRING_TO_JSVAL(js_str)
		,NULL,NULL,JSPROP_ENUMERATE|JSPROP_READONLY))
		return(FALSE);

	if((js_str=JS_NewStringCopyZ(cx, sub->sname))==NULL)
		return(FALSE);
	if(!JS_DefineProperty(cx, subobj, "name", STRING_TO_JSVAL(js_str)
		,NULL,NULL,JSPROP_ENUMERATE|JSPROP_READONLY))
		return(FALSE);

	if((js_str=JS_NewStringCopyZ(cx, sub->lname))==NULL)
		return(FALSE);
	if(!JS_DefineProperty(cx, subobj, "description", STRING_TO_JSVAL(js_str)
		,NULL,NULL,JSPROP_ENUMERATE|JSPROP_READONLY))
		return(FALSE);

	if((js_str=JS_NewStringCopyZ(cx, sub->qwkname))==NULL)
		return(FALSE);
	if(!JS_DefineProperty(cx, subobj, "qwk_name", STRING_TO_JSVAL(js_str)
		,NULL,NULL,JSPROP_ENUMERATE|JSPROP_READONLY))
		return(FALSE);

	if(sub->newsgroup[0])
		SAFECOPY(str,sub->newsgroup);
	else {
		sprintf(str,"%s.%s",cfg->grp[sub->grp]->sname,sub->sname);
		for(c=0;str[c];c++)
			if(str[c]==' ')
				str[c]='_';
	}
	if((js_str=JS_NewStringCopyZ(cx, str))==NULL)
		return(FALSE);
	if(!JS_DefineProperty(cx, subobj, "newsgroup", STRING_TO_JSVAL(js_str)
		,NULL,NULL,JSPROP_ENUMERATE|JSPROP_READONLY))
		return(FALSE);

	if((js_str=JS_NewStringCopyZ(cx, sub->arstr))==NULL)
		return(FALSE);
	if(!JS_DefineProperty(cx, subobj, "ars", STRING_TO_JSVAL(js_str)
		,NULL,NULL,JSPROP_ENUMERATE|JSPROP_READONLY))
		return(FALSE);

	if((js_str=JS_NewStringCopyZ(cx, sub->read_arstr))==NULL)
		return(FALSE);
	if(!JS_DefineProperty(cx, subobj, "read_ars", STRING_TO_JSVAL(js_str)
		,NULL,NULL,JSPROP_ENUMERATE|JSPROP_READONLY))
		return(FALSE);

	if((js_str=JS_NewStringCopyZ(cx, sub->post_arstr))==NULL)
		return(FALSE);
	if(!JS_DefineProperty(cx, subobj, "post_ars", STRING_TO_JSVAL(js_str)
		,NULL,NULL,JSPROP_ENUMERATE|JSPROP_READONLY))
		return(FALSE);

	if((js_str=JS_NewStringCopyZ(cx, sub->op_arstr))==NULL)
		return(FALSE);
	if(!JS_DefineProperty(cx, subobj, "operator_ars", STRING_TO_JSVAL(js_str)
		,NULL,NULL,JSPROP_ENUMERATE|JSPROP_READONLY))
		return(FALSE);

	if((js_str=JS_NewStringCopyZ(cx, sub->mod_arstr))==NULL)
		return(FALSE);
	if(!JS_DefineProperty(cx, subobj, "moderated_ars", STRING_TO_JSVAL(js_str)
		,NULL,NULL,JSPROP_ENUMERATE|JSPROP_READONLY))
		return(FALSE);

	if((js_str=JS_NewStringCopyZ(cx, sub->data_dir))==NULL)
		return(FALSE);
	if(!JS_DefineProperty(cx, subobj, "data_dir", STRING_TO_JSVAL(js_str)
		,NULL,NULL,JSPROP_ENUMERATE|JSPROP_READONLY))
		return(FALSE);

	if((js_str=JS_NewStringCopyZ(cx, sub->origline))==NULL)
		return(FALSE);
	if(!JS_DefineProperty(cx, subobj, "fidonet_origin", STRING_TO_JSVAL(js_str)
		,NULL,NULL,JSPROP_ENUMERATE|JSPROP_READONLY))
		return(FALSE);

	if((js_str=JS_NewStringCopyZ(cx, sub->tagline))==NULL)
		return(FALSE);
	if(!JS_DefineProperty(cx, subobj, "qwknet_tagline", STRING_TO_JSVAL(js_str)
		,NULL,NULL,JSPROP_ENUMERATE|JSPROP_READONLY))
		return(FALSE);

	if(!JS_NewNumberValue(cx,sub->misc,&val))
		return(FALSE);
	if(!JS_DefineProperty(cx, subobj, "settings", val
		,NULL,NULL,JSPROP_ENUMERATE|JSPROP_READONLY))
		return(FALSE);

	if(!JS_DefineProperty(cx, subobj, "ptridx", INT_TO_JSVAL(sub->ptridx)
		,NULL,NULL,JSPROP_ENUMERATE|JSPROP_READONLY))
		return(FALSE);

	if(!JS_DefineProperty(cx, subobj, "qwk_conf", INT_TO_JSVAL(sub->qwkconf)
		,NULL,NULL,JSPROP_ENUMERATE|JSPROP_READONLY))
		return(FALSE);

	if(!JS_DefineProperty(cx, subobj, "max_crcs", INT_TO_JSVAL(sub->maxcrcs)
		,NULL,NULL,JSPROP_ENUMERATE|JSPROP_READONLY))
		return(FALSE);

	if(!JS_DefineProperty(cx, subobj, "max_msgs", INT_TO_JSVAL(sub->maxmsgs)
		,NULL,NULL,JSPROP_ENUMERATE|JSPROP_READONLY))
		return(FALSE);

	if(!JS_DefineProperty(cx, subobj, "max_age", INT_TO_JSVAL(sub->maxage)
		,NULL,NULL,JSPROP_ENUMERATE|JSPROP_READONLY))
		return(FALSE);

#ifdef BUILD_JSDOCS
	js_CreateArrayOfStrings(cx, subobj, "_property_desc_list", msg_area_prop_desc, JSPROP_READONLY);
#endif

	return(TRUE);
}

/*******************************************/
/* Re-writable Sub-board Object Properites */
/*******************************************/
enum {
	 SUB_PROP_SCAN_PTR
	,SUB_PROP_SCAN_CFG
	,SUB_PROP_LAST_READ
};


static JSBool js_sub_get(JSContext *cx, JSObject *obj, jsval id, jsval *vp)
{
    jsint       tiny;
	subscan_t*	scan;

	if((scan=(subscan_t*)JS_GetPrivate(cx,obj))==NULL)
		return(JS_TRUE);

    tiny = JSVAL_TO_INT(id);

	switch(tiny) {
		case SUB_PROP_SCAN_PTR:
			JS_NewNumberValue(cx,scan->ptr,vp);
			break;
		case SUB_PROP_SCAN_CFG:
			JS_NewNumberValue(cx,scan->cfg,vp);
			break;
		case SUB_PROP_LAST_READ:
			JS_NewNumberValue(cx,scan->last,vp);
			break;
	}

	return(JS_TRUE);
}

static JSBool js_sub_set(JSContext *cx, JSObject *obj, jsval id, jsval *vp)
{
	int32		val=0;
    jsint       tiny;
	subscan_t*	scan;

	if((scan=(subscan_t*)JS_GetPrivate(cx,obj))==NULL)
		return(JS_TRUE);

    tiny = JSVAL_TO_INT(id);

	switch(tiny) {
		case SUB_PROP_SCAN_PTR:
			JS_ValueToInt32(cx, *vp, (int32*)&scan->ptr);
			break;
		case SUB_PROP_SCAN_CFG:
			JS_ValueToInt32(cx, *vp, &val);
			scan->cfg=(ushort)val;
			break;
		case SUB_PROP_LAST_READ:
			JS_ValueToInt32(cx, *vp, (int32*)&scan->last);
			break;
	}

	return(JS_TRUE);
}

static struct JSPropertySpec js_sub_properties[] = {
/*		 name				,tinyid		,flags	*/

	{	"scan_ptr"	,SUB_PROP_SCAN_PTR	,JSPROP_ENUMERATE|JSPROP_SHARED },
	{	"scan_cfg"	,SUB_PROP_SCAN_CFG	,JSPROP_ENUMERATE|JSPROP_SHARED },
	{	"lead_read"	,SUB_PROP_LAST_READ	,JSPROP_ENUMERATE|JSPROP_SHARED },
	{0}
};


static JSClass js_sub_class = {
     "MsgSub"				/* name			*/
    ,JSCLASS_HAS_PRIVATE	/* flags		*/
	,JS_PropertyStub		/* addProperty	*/
	,JS_PropertyStub		/* delProperty	*/
	,js_sub_get				/* getProperty	*/
	,js_sub_set				/* setProperty	*/
	,JS_EnumerateStub		/* enumerate	*/
	,JS_ResolveStub			/* resolve		*/
	,JS_ConvertStub			/* convert		*/
	,JS_FinalizeStub		/* finalize		*/
};

JSObject* DLLCALL js_CreateMsgAreaObject(JSContext* cx, JSObject* parent, scfg_t* cfg
										  ,user_t* user, subscan_t* subscan)
{
	JSObject*	areaobj;
	JSObject*	allgrps;
	JSObject*	allsubs;
	JSObject*	grpobj;
	JSObject*	subobj;
	JSObject*	grp_list;
	JSObject*	sub_list;
	JSString*	js_str;
	jsval		val;
	jsuint		grp_index;
	jsuint		sub_index;
	uint		l,d;

	/* Return existing object if it's already been created */
	if(JS_GetProperty(cx,parent,"msg_area",&val) && val!=JSVAL_VOID)
		areaobj = JSVAL_TO_OBJECT(val);
	else
		areaobj = JS_DefineObject(cx, parent, "msg_area", NULL
									, NULL, JSPROP_ENUMERATE|JSPROP_READONLY);

	if(areaobj==NULL)
		return(NULL);

#ifdef BUILD_JSDOCS
	js_DescribeSyncObject(cx,areaobj,"Message Areas",310);
#endif

	/* msg_area.grp[] */
	if((allgrps=JS_NewObject(cx, NULL, NULL, areaobj))==NULL)
		return(NULL);

	val=OBJECT_TO_JSVAL(allgrps);
	if(!JS_SetProperty(cx, areaobj, "grp", &val))
		return(NULL);

	/* msg_area.sub[] */
	if((allsubs=JS_NewObject(cx, NULL, NULL, areaobj))==NULL)
		return(NULL);

	val=OBJECT_TO_JSVAL(allsubs);
	if(!JS_SetProperty(cx, areaobj, "sub", &val))
		return(NULL);

	/* msg_area.grp_list[] */
	if((grp_list=JS_NewArrayObject(cx, 0, NULL))==NULL) 
		return(NULL);

	val=OBJECT_TO_JSVAL(grp_list);
	if(!JS_SetProperty(cx, areaobj, "grp_list", &val)) 
		return(NULL);

	for(l=0;l<cfg->total_grps;l++) {

		if((grpobj=JS_NewObject(cx, NULL, NULL, NULL))==NULL)
			return(NULL);

		val=OBJECT_TO_JSVAL(grpobj);
		grp_index=-1;
		if(user==NULL || chk_ar(cfg,cfg->grp[l]->ar,user)) {

			if(!JS_GetArrayLength(cx, grp_list, &grp_index))
				return(NULL);

			if(!JS_SetElement(cx, grp_list, grp_index, &val))
				return(NULL);
		}

		/* Add as property (associative array element) */
		if(!JS_DefineProperty(cx, allgrps, cfg->grp[l]->sname, val
			,NULL,NULL,JSPROP_READONLY|JSPROP_ENUMERATE))
			return(NULL);

		val=INT_TO_JSVAL(grp_index);
		if(!JS_SetProperty(cx, grpobj, "index", &val))
			return(NULL);

		val=INT_TO_JSVAL(l);
		if(!JS_SetProperty(cx, grpobj, "number", &val))
			return(NULL);

		if((js_str=JS_NewStringCopyZ(cx, cfg->grp[l]->sname))==NULL)
			return(NULL);
		val=STRING_TO_JSVAL(js_str);
		if(!JS_SetProperty(cx, grpobj, "name", &val))
			return(NULL);

		if((js_str=JS_NewStringCopyZ(cx, cfg->grp[l]->lname))==NULL)
			return(NULL);
		val=STRING_TO_JSVAL(js_str);
		if(!JS_SetProperty(cx, grpobj, "description", &val))
			return(NULL);

		if((js_str=JS_NewStringCopyZ(cx, cfg->grp[l]->arstr))==NULL)
			return(NULL);
		if(!JS_DefineProperty(cx, grpobj, "ars", STRING_TO_JSVAL(js_str)
			,NULL,NULL,JSPROP_ENUMERATE|JSPROP_READONLY))
			return(NULL);

#ifdef BUILD_JSDOCS
		js_DescribeSyncObject(cx,grpobj,"Message Groups (current user has access to)",310);
#endif

		/* sub_list[] */
		if((sub_list=JS_NewArrayObject(cx, 0, NULL))==NULL) 
			return(NULL);

		val=OBJECT_TO_JSVAL(sub_list);
		if(!JS_SetProperty(cx, grpobj, "sub_list", &val)) 
			return(NULL);

		for(d=0;d<cfg->total_subs;d++) {
			if(cfg->sub[d]->grp!=l)
				continue;

			if((subobj=JS_NewObject(cx, &js_sub_class, NULL, NULL))==NULL)
				return(NULL);

			if(subscan!=NULL)
				JS_SetPrivate(cx,subobj,&subscan[d]);

			val=OBJECT_TO_JSVAL(subobj);
			sub_index=-1;
			if(user==NULL || chk_ar(cfg,cfg->sub[d]->ar,user)) {

				if(!JS_GetArrayLength(cx, sub_list, &sub_index))
					return(NULL);							

				if(!JS_SetElement(cx, sub_list, sub_index, &val))
					return(NULL);
			}

			/* Add as property (associative array element) */
			if(!JS_DefineProperty(cx, allsubs, cfg->sub[d]->code, val
				,NULL,NULL,JSPROP_READONLY|JSPROP_ENUMERATE))
				return(NULL);

			val=INT_TO_JSVAL(sub_index);
			if(!JS_SetProperty(cx, subobj, "index", &val))
				return(NULL);

			val=INT_TO_JSVAL(grp_index);
			if(!JS_SetProperty(cx, subobj, "grp_index", &val))
				return(NULL);

			if(!js_CreateMsgAreaProperties(cx, cfg, subobj, d))
				return(NULL);
			
			if(user==NULL || chk_ar(cfg,cfg->sub[d]->read_ar,user))
				val=BOOLEAN_TO_JSVAL(JS_TRUE);
			else
				val=BOOLEAN_TO_JSVAL(JS_FALSE);
			if(!JS_SetProperty(cx, subobj, "can_read", &val))
				return(NULL);

			if(user==NULL)
				val=BOOLEAN_TO_JSVAL(JS_TRUE);
			else if(cfg->sub[d]->misc&(SUB_QNET|SUB_FIDO|SUB_PNET|SUB_INET)
				&& user->rest&FLAG('N'))		/* network restriction? */
				val=BOOLEAN_TO_JSVAL(JS_FALSE);
			else if(!chk_ar(cfg,cfg->sub[d]->post_ar,user)
				|| user->rest&FLAG('P'))		/* post restriction? */
				val=BOOLEAN_TO_JSVAL(JS_FALSE);	
			else
				val=BOOLEAN_TO_JSVAL(JS_TRUE);
			if(!JS_SetProperty(cx, subobj, "can_post", &val))
				return(NULL);

			if(user!=NULL &&
				(user->level>=SYSOP_LEVEL ||
					(cfg->sub[d]->op_ar[0]!=0 && chk_ar(cfg,cfg->sub[d]->op_ar,user))))
				val=BOOLEAN_TO_JSVAL(JS_TRUE);
			else
				val=BOOLEAN_TO_JSVAL(JS_FALSE);
			if(!JS_SetProperty(cx, subobj, "is_operator", &val))
				return(NULL);

			if(cfg->sub[d]->mod_ar[0]!=0 && user!=NULL 
				&& chk_ar(cfg,cfg->sub[d]->mod_ar,user))
				val=BOOLEAN_TO_JSVAL(JS_TRUE);
			else
				val=BOOLEAN_TO_JSVAL(JS_FALSE);
			if(!JS_SetProperty(cx, subobj, "is_moderated", &val))
				return(NULL);

			if(!JS_DefineProperties(cx, subobj, js_sub_properties))
				return(NULL);

#ifdef BUILD_JSDOCS
			js_DescribeSyncObject(cx,subobj,"Message Sub-boards (current user has access to)</h2>"
				"(all properties are <small>READ ONLY</small> except for "
				"<i>scan_ptr</i>, <i>scan_cfg</i>, and <i>last_read</i>)"
				,310);
#endif

		}

#ifdef BUILD_JSDOCS
		js_CreateArrayOfStrings(cx, grpobj, "_property_desc_list", msg_grp_prop_desc, JSPROP_READONLY);
#endif
	}

#ifdef BUILD_JSDOCS
	js_DescribeSyncObject(cx,allgrps,"Associative array of all groups (use name as index)",312);
	JS_DefineProperty(cx,allgrps,"_dont_document",JSVAL_TRUE,NULL,NULL,JSPROP_READONLY);
#endif

#ifdef BUILD_JSDOCS
	js_DescribeSyncObject(cx,allsubs,"Associative array of all sub-boards (use internal code as index)",311);
	JS_DefineProperty(cx,allsubs,"_dont_document",JSVAL_TRUE,NULL,NULL,JSPROP_READONLY);
#endif

	return(areaobj);
}

#endif	/* JAVSCRIPT */
