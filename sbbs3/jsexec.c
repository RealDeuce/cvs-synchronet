/* jsexec.c */

/* Execute a Synchronet JavaScript module from the command-line */

/* $Id$ */

/****************************************************************************
 * @format.tab-size 4		(Plain Text/Source Code File Header)			*
 * @format.use-tabs true	(see http://www.synchro.net/ptsc_hdr.html)		*
 *																			*
 * Copyright 2004 Rob Swindell - http://www.synchro.net/copyright.html		*
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

#ifndef JAVASCRIPT
#define JAVASCRIPT
#endif

#ifdef __unix__
#include <signal.h>
#endif

#include "sbbs.h"

#define DEFAULT_LOG_MASK	0xff	/* Display all LOG levels */
#define DEFAULT_ERR_LOG_LVL	LOG_WARNING

JSRuntime*	js_runtime;
JSContext*	js_cx;
JSObject*	js_glob;
js_branch_t	branch;
scfg_t		scfg;
ulong		js_max_bytes=JAVASCRIPT_MAX_BYTES;
ulong		js_cx_stack=JAVASCRIPT_CONTEXT_STACK;
FILE*		confp;
FILE*		errfp;
FILE*		nulfp;
FILE*		statfp;
char		revision[16];
char*		host_name=NULL;
char		host_name_buf[128];
BOOL		pause_on_exit=FALSE;
BOOL		pause_on_error=FALSE;
BOOL		terminated=FALSE;
DWORD		log_mask=DEFAULT_LOG_MASK;
int  		err_level=DEFAULT_ERR_LOG_LVL;

void banner(FILE* fp)
{
	fprintf(fp,"\nJSexec v%s%c-%s (rev %s) - "
		"Execute Synchronet JavaScript Module\n"
		,VERSION,REVISION
		,PLATFORM_DESC
		,revision
		);
}

void usage(FILE* fp)
{
	banner(fp);

	fprintf(fp,"\nusage: jsexec [-opts] [path]module[.js] [args]\n"
		"\navailable opts:\n\n"
		"\t-c<ctrl_dir>   specify path to Synchronet CTRL directory\n"
		"\t-m<bytes>      set maximum heap size (default=%u bytes)\n"
		"\t-s<bytes>      set context stack size (default=%u bytes)\n"
		"\t-b<limit>      set branch limit (default=%u, 0=unlimited)\n"
		"\t-y<interval>   set yield interval (default=%u, 0=never)\n"
		"\t-g<interval>   set garbage collection interval (default=%u, 0=never)\n"
		"\t-h[hostname]   use local or specified host name (instead of SCFG value)\n"
		"\t-L<mask>       set log level mask (default=0x%x)\n"
		"\t-E<level>      set error log level threshold (default=%d)\n"
		"\t-a             append instead of overwriting message output files\n"
		"\t-e<filename>   send error messages to file in addition to stderr\n"
		"\t-o<filename>   send console messages to file instead of stdout\n"
		"\t-n             send status messages to %s instead of stderr\n"
		"\t-q             send console messages to %s instead of stdout\n"
		"\t-x             disable auto-termination on local abort signal\n"
		"\t-l             loop until intentionally terminated\n"
		"\t-p             wait for keypress (pause) on exit\n"
		"\t-!             wait for keypress (pause) on error\n"
		,JAVASCRIPT_MAX_BYTES
		,JAVASCRIPT_CONTEXT_STACK
		,JAVASCRIPT_BRANCH_LIMIT
		,JAVASCRIPT_YIELD_INTERVAL
		,JAVASCRIPT_GC_INTERVAL
		,DEFAULT_LOG_MASK
		,DEFAULT_ERR_LOG_LVL
		,_PATH_DEVNULL
		,_PATH_DEVNULL
		);
}

/* Log printf */
int lprintf(int level, char *fmt, ...)
{
	va_list argptr;
	char sbuf[1024];
	int ret=0;

	if(!(log_mask&(1<<level)))
		return(0);

    va_start(argptr,fmt);
    vsnprintf(sbuf,sizeof(sbuf),fmt,argptr);
	sbuf[sizeof(sbuf)-1]=0;
    va_end(argptr);
	if(level<=err_level)
		ret=fprintf(errfp,"%s",sbuf);
	if(level>err_level || (errfp!=stderr && errfp!=confp))
		ret=fprintf(confp,"%s",sbuf);
    return(ret);
}

#if defined(_WINSOCKAPI_)

WSADATA WSAData;
#define SOCKLIB_DESC WSAData.szDescription
static BOOL WSAInitialized=FALSE;

static BOOL winsock_startup(void)
{
	int		status;             /* Status Code */

    if((status = WSAStartup(MAKEWORD(1,1), &WSAData))==0) {
/*		fprintf(statfp,"%s %s\n",WSAData.szDescription, WSAData.szSystemStatus); */
		WSAInitialized=TRUE;
		return(TRUE);
	}

    lprintf(LOG_ERR,"!WinSock startup ERROR %d\n", status);
	return(FALSE);
}

#else /* No WINSOCK */

#define winsock_startup()	(TRUE)
#define SOCKLIB_DESC NULL

#endif

void bail(int code)
{

#if defined(_WINSOCKAPI_)
	if(WSAInitialized && WSACleanup()!=0) 
		lprintf(LOG_ERR,"!WSACleanup ERROR %d\n",ERROR_VALUE);
#endif

	if(pause_on_exit || (code && pause_on_error)) {
		fprintf(statfp,"\nHit enter to continue...");
		getchar();
	}

	if(code)
		fprintf(statfp,"\nReturning error code: %d\n",code);
	exit(code);
}

static JSBool
js_log(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
    uintN		i=0;
	int32		level=LOG_INFO;
    JSString*	str=NULL;

	if(JSVAL_IS_NUMBER(argv[i]))
		JS_ValueToInt32(cx,argv[i++],&level);

	for(; i<argc; i++) {
		if((str=JS_ValueToString(cx, argv[i]))==NULL)
			return(JS_FALSE);
		lprintf(level,"%s",JS_GetStringBytes(str));
	}
	if(argc)
		lprintf(level,"\n");

	if(str==NULL)
		*rval = JSVAL_VOID;
	else
		*rval = STRING_TO_JSVAL(str);

    return(JS_TRUE);
}

static JSBool
js_read(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
	char*	buf;
	int		rd;
	int32	len=128;

	*rval = JSVAL_VOID;

	if(argc)
		JS_ValueToInt32(cx,argv[0],&len);
	if((buf=malloc(len))==NULL)
		return(JS_TRUE);

	rd=fread(buf,sizeof(char),len,stdin);

	if(rd>=0)
		*rval = STRING_TO_JSVAL(JS_NewStringCopyN(cx,buf,rd));

	free(buf);
    return(JS_TRUE);
}

static JSBool
js_readln(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
	char*	buf;
	char*	p;
	int32	len=128;

	*rval = JSVAL_VOID;

	if(argc)
		JS_ValueToInt32(cx,argv[0],&len);
	if((buf=malloc(len+1))==NULL)
		return(JS_TRUE);

	p=fgets(buf,len+1,stdin);

	if(p!=NULL)
		*rval = STRING_TO_JSVAL(JS_NewStringCopyZ(cx,p));

	free(buf);
    return(JS_TRUE);
}


static JSBool
js_write(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
    uintN		i;
    JSString*	str=NULL;

    for (i = 0; i < argc; i++) {
		if((str=JS_ValueToString(cx, argv[i]))==NULL)
		    return(JS_FALSE);
		fprintf(confp,"%s",JS_GetStringBytes(str));
	}

	if(str==NULL)
		*rval = JSVAL_VOID;
	else
		*rval = STRING_TO_JSVAL(str);
    return(JS_TRUE);
}

static JSBool
js_writeln(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
	if(!js_write(cx,obj,argc,argv,rval))
		return(JS_FALSE);

	fprintf(confp,"\n");
    return(JS_TRUE);
}

static JSBool
js_printf(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
	char*		p;
    uintN		i;
	JSString *	fmt;
    JSString *	str;
	va_list		arglist[64];

	if((fmt = JS_ValueToString(cx, argv[0]))==NULL)
		return(JS_FALSE);

	memset(arglist,0,sizeof(arglist));	// Initialize arglist to NULLs

    for (i = 1; i < argc && i<sizeof(arglist)/sizeof(arglist[0]); i++) {
		if(JSVAL_IS_DOUBLE(argv[i]))
			arglist[i-1]=(char*)(unsigned long)*JSVAL_TO_DOUBLE(argv[i]);
		else if(JSVAL_IS_INT(argv[i]))
			arglist[i-1]=(char *)JSVAL_TO_INT(argv[i]);
		else {
			if((str=JS_ValueToString(cx, argv[i]))==NULL) {
				JS_ReportError(cx,"JS_ValueToString failed");
			    return(JS_FALSE);
			}
			arglist[i-1]=JS_GetStringBytes(str);
		}
	}
	
	if((p=JS_vsmprintf(JS_GetStringBytes(fmt),(char*)arglist))==NULL)
		return(JS_FALSE);

	fprintf(confp,"%s",p);

	*rval = STRING_TO_JSVAL(JS_NewStringCopyZ(cx, p));

	JS_smprintf_free(p);

    return(JS_TRUE);
}

static JSBool
js_alert(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
    JSString *	str;

	if((str=JS_ValueToString(cx, argv[0]))==NULL)
	    return(JS_FALSE);

	fprintf(confp,"!%s\n",JS_GetStringBytes(str));

	*rval = JSVAL_VOID;
    return(JS_TRUE);
}

static JSBool
js_confirm(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
    JSString *	str;

	if((str=JS_ValueToString(cx, argv[0]))==NULL)
	    return(JS_FALSE);

	printf("%s (Y/N)?", JS_GetStringBytes(str));

	*rval = BOOLEAN_TO_JSVAL(FALSE);
	return(JS_TRUE);
}

static JSBool
js_prompt(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
	char		instr[81];
    JSString *	prompt;
    JSString *	str;

	if(!JSVAL_IS_VOID(argv[0])) {
		if((prompt=JS_ValueToString(cx, argv[0]))==NULL)
			return(JS_FALSE);
		fprintf(confp,"%s: ",JS_GetStringBytes(prompt));
	}

	if(argc>1) {
		if((str=JS_ValueToString(cx, argv[1]))==NULL)
		    return(JS_FALSE);
		SAFECOPY(instr,JS_GetStringBytes(str));
	} else
		instr[0]=0;

	if(!fgets(instr,sizeof(instr),stdin)) {
		*rval = JSVAL_VOID;
		return(JS_TRUE);
	}

	if((str=JS_NewStringCopyZ(cx, instr))==NULL)
	    return(JS_FALSE);

	*rval = STRING_TO_JSVAL(str);
    return(JS_TRUE);
}

static JSBool
js_chdir(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
	char*		p;

	if((p=JS_GetStringBytes(JS_ValueToString(cx, argv[0])))==NULL) {
		*rval = INT_TO_JSVAL(-1);
		return(JS_TRUE);
	}

	*rval = BOOLEAN_TO_JSVAL(chdir(p)==0);
	return(JS_TRUE);
}

static JSBool
js_putenv(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
	char*		p;

	if((p=JS_GetStringBytes(JS_ValueToString(cx, argv[0])))==NULL) {
		*rval = INT_TO_JSVAL(-1);
		return(JS_TRUE);
	}

	*rval = BOOLEAN_TO_JSVAL(putenv(p)==0);
	return(JS_TRUE);
}

static jsSyncMethodSpec js_global_functions[] = {
	{"log",				js_log,				1},
	{"read",			js_read,            1},
	{"readln",			js_readln,          1},
    {"write",           js_write,           0},
    {"writeln",         js_writeln,         0},
    {"print",           js_writeln,         0},
    {"printf",          js_printf,          1},	
	{"alert",			js_alert,			1},
	{"prompt",			js_prompt,			1},
	{"confirm",			js_confirm,			1},
	{"chdir",			js_chdir,			1},
	{"putenv",			js_putenv,			1},
    {0}
};

static void
js_ErrorReporter(JSContext *cx, const char *message, JSErrorReport *report)
{
	char	line[64];
	char	file[MAX_PATH+1];
	const char*	warning;

	if(report==NULL) {
		lprintf(LOG_ERR,"!JavaScript: %s\n", message);
		return;
    }

	if(report->filename)
		sprintf(file," %s",report->filename);
	else
		file[0]=0;

	if(report->lineno)
		sprintf(line," line %d",report->lineno);
	else
		line[0]=0;

	if(JSREPORT_IS_WARNING(report->flags)) {
		if(JSREPORT_IS_STRICT(report->flags))
			warning="strict warning";
		else
			warning="warning";
	} else
		warning="";

	lprintf(LOG_ERR,"!JavaScript %s%s%s: %s\n",warning,file,line,message);
}

static JSBool
js_BranchCallback(JSContext *cx, JSScript *script)
{
	branch.counter++;

	/* Infinite loop? */
	if(branch.limit && branch.counter > branch.limit) {
		JS_ReportError(cx,"Infinite loop (%lu branches) detected",branch.counter);
		branch.counter=0;
		return(JS_FALSE);
	}
	/* Give up timeslices every once in a while */
	if(branch.yield_interval && (branch.counter%branch.yield_interval)==0)
		YIELD();

	if(branch.gc_interval && (branch.counter%branch.gc_interval)==0)
		JS_MaybeGC(cx), branch.gc_attempts++;

	if(branch.auto_terminate && terminated) {
		JS_ReportError(cx,"Terminated");
		return(JS_FALSE);
	}

    return(JS_TRUE);
}

static BOOL js_CreateEnvObject(JSContext* cx, JSObject* glob, char** env)
{
	char		name[256];
	char*		val;
	uint		i;
	JSObject*	js_env;

	if((js_env=JS_NewObject(js_cx, NULL, NULL, glob))==NULL)
		return(FALSE);

	if(!JS_DefineProperty(cx, glob, "env", OBJECT_TO_JSVAL(js_env)
		,NULL,NULL,JSPROP_READONLY|JSPROP_ENUMERATE))
		return(FALSE);

	for(i=0;env[i]!=NULL;i++) {
		SAFECOPY(name,env[i]);
		truncstr(name,"=");
		val=strchr(env[i],'=');
		if(val==NULL)
			val="";
		else
			val++;
		if(!JS_DefineProperty(cx, js_env, name, STRING_TO_JSVAL(JS_NewStringCopyZ(cx,val))
			,NULL,NULL,JSPROP_READONLY|JSPROP_ENUMERATE))
			return(FALSE);
	}

	return(TRUE);
}

static BOOL js_init(char** environ)
{
	fprintf(statfp,"%s\n",(char *)JS_GetImplementationVersion());

	fprintf(statfp,"JavaScript: Creating runtime: %lu bytes\n"
		,js_max_bytes);

	if((js_runtime = JS_NewRuntime(js_max_bytes))==NULL)
		return(FALSE);

	fprintf(statfp,"JavaScript: Initializing context (stack: %lu bytes)\n"
		,js_cx_stack);

    if((js_cx = JS_NewContext(js_runtime, js_cx_stack))==NULL)
		return(FALSE);

	JS_SetErrorReporter(js_cx, js_ErrorReporter);

	/* Global Object */
	if((js_glob=js_CreateGlobalObjects(js_cx, &scfg, js_global_functions
		,time(NULL), host_name, SOCKLIB_DESC	/* system */
		,&branch								/* js */
		,NULL,INVALID_SOCKET					/* client */
		))==NULL)
		return(FALSE);

	/* Environment Object (associative array) */
	if(!js_CreateEnvObject(js_cx, js_glob, environ))
		return(FALSE);

	return(TRUE);
}

static const char* js_ext(const char* fname)
{
	if(strchr(fname,'.')==NULL)
		return(".js");
	return("");
}

long js_exec(const char *fname, char** args)
{
	int			argc=0;
	uint		line_no;
	char		path[MAX_PATH+1];
	char		line[1024];
	char		compiler[32];
	char		rev_detail[256];
	size_t		len;
	char*		js_buf=NULL;
	size_t		js_buflen;
	JSScript*	js_script=NULL;
	JSString*	arg;
	JSObject*	argv;
	FILE*		fp=stdin;
	jsval		val;
	jsval		rval=JSVAL_VOID;
	int32		result=0;
	
	if(fname!=NULL) {
		if(strcspn(fname,"/\\")==strlen(fname)) {
			sprintf(path,"%s%s%s",scfg.mods_dir,fname,js_ext(fname));
			if(scfg.mods_dir[0]==0 || !fexistcase(path))
				sprintf(path,"%s%s%s",scfg.exec_dir,fname,js_ext(fname));
		} else
			SAFECOPY(path,fname);

		if(!fexistcase(path)) {
			lprintf(LOG_ERR,"!Module file (%s) doesn't exist\n",path);
			return(-1); 
		}

		if((fp=fopen(path,"r"))==NULL) {
			lprintf(LOG_ERR,"!Error %d (%s) opening %s\n",errno,STRERROR(errno),path);
			return(-1);
		}
	}
	JS_ClearPendingException(js_cx);


	argv=JS_NewArrayObject(js_cx, 0, NULL);
	JS_DefineProperty(js_cx, js_glob, "argv", OBJECT_TO_JSVAL(argv)
		,NULL,NULL,JSPROP_READONLY|JSPROP_ENUMERATE);

	for(argc=0;args[argc];argc++) {
		arg = JS_NewStringCopyZ(js_cx, args[argc]);
		if(arg==NULL)
			break;
		val=STRING_TO_JSVAL(arg);
		if(!JS_SetElement(js_cx, argv, argc, &val))
			break;
	}
	JS_DefineProperty(js_cx, js_glob, "argc", INT_TO_JSVAL(argc)
		,NULL,NULL,JSPROP_READONLY|JSPROP_ENUMERATE);

	JS_DefineProperty(js_cx, js_glob, "jsexec_revision"
		,STRING_TO_JSVAL(JS_NewStringCopyZ(js_cx,revision))
		,NULL,NULL,JSPROP_READONLY|JSPROP_ENUMERATE);

	DESCRIBE_COMPILER(compiler);

	sprintf(rev_detail,"JSexec %s%s  "
		"Compiled %s %s with %s"
		,revision
#ifdef _DEBUG
		," Debug"
#else
		,""
#endif
		,__DATE__, __TIME__, compiler
		);

	JS_DefineProperty(js_cx, js_glob, "jsexec_revision_detail"
		,STRING_TO_JSVAL(JS_NewStringCopyZ(js_cx,rev_detail))
		,NULL,NULL,JSPROP_READONLY|JSPROP_ENUMERATE);

	JS_SetBranchCallback(js_cx, js_BranchCallback);

	if(fp==stdin) 	 /* Using stdin for script source */
		SAFECOPY(path,"stdin");

	fprintf(statfp,"Reading script from %s\n",path);
	line_no=0;
	js_buflen=0;
	while(!feof(fp)) {
		if(!fgets(line,sizeof(line),fp))
			break;
		line_no++;
#if defined(__unix__)	/* Support Unix Shell Scripts that start with #!/path/to/jsexec */
		if(line_no==1 && strncmp(line,"#!",2)==0)
			strcpy(line,"\n");	/* To keep line count correct */
#endif
		len=strlen(line);
		if((js_buf=realloc(js_buf,js_buflen+len))==NULL) {
			lprintf(LOG_ERR,"!Error allocating %u bytes of memory\n"
				,js_buflen+len);
			return(-1);
		}
		memcpy(js_buf+js_buflen,line,len);
		js_buflen+=len;
	}
	if(fp!=NULL && fp!=stdin)
		fclose(fp);

	if((js_script=JS_CompileScript(js_cx, js_glob, js_buf, js_buflen, fname==NULL ? NULL : path, 1))==NULL) {
		lprintf(LOG_ERR,"!Error compiling script from %s\n",path);
		return(-1);
	}
	JS_ExecuteScript(js_cx, js_glob, js_script, &rval);

	JS_GetProperty(js_cx, js_glob, "exit_code", &rval);

	if(rval!=JSVAL_VOID) {
		fprintf(statfp,"Using JavaScript exit_code: %s\n",JS_GetStringBytes(JS_ValueToString(js_cx,rval)));
		JS_ValueToInt32(js_cx,rval,&result);
	}

	JS_DestroyScript(js_cx, js_script);

	JS_GC(js_cx);

	if(js_buf!=NULL)
		free(js_buf);

	return(result);
}

void break_handler(int type)
{
	fprintf(statfp,"\n-> Terminated Locally (signal: %d)\n",type);
	terminated=TRUE;
}

#if defined(_WIN32)
BOOL WINAPI ControlHandler(DWORD CtrlType)
{
	break_handler((int)CtrlType);
	return TRUE;
}
#endif

/*********************/
/* Entry point (duh) */
/*********************/
int main(int argc, char **argv, char** environ)
{
	char	error[512];
	char*	module=NULL;
	char*	p;
	char*	omode="w";
	int		argn;
	long	result;
	BOOL	loop=FALSE;

	confp=stdout;
	errfp=stderr;
	if((nulfp=fopen(_PATH_DEVNULL,"w+"))==NULL) {
		perror(_PATH_DEVNULL);
		bail(-1);
	}
	if(isatty(fileno(stderr)))
		statfp=stderr;
	else	/* if redirected, don't send status messages to stderr */
		statfp=nulfp;

	branch.limit=JAVASCRIPT_BRANCH_LIMIT;
	branch.yield_interval=JAVASCRIPT_YIELD_INTERVAL;
	branch.gc_interval=JAVASCRIPT_GC_INTERVAL;
	branch.terminated=&terminated;
	branch.auto_terminate=TRUE;

	sscanf("$Revision$", "%*s %s", revision);

	memset(&scfg,0,sizeof(scfg));
	scfg.size=sizeof(scfg);

	if(!winsock_startup())
		bail(2);

	for(argn=1;argn<argc && module==NULL;argn++) {
		if(argv[argn][0]=='-') {
			p=argv[argn]+2;
			switch(argv[argn][1]) {
				case 'a':
					omode="a";
					break;
				case 'm':
					if(*p==0) p=argv[++argn];
					js_max_bytes=strtoul(p,NULL,0);
					break;
				case 's':
					if(*p==0) p=argv[++argn];
					js_cx_stack=strtoul(p,NULL,0);
					break;
				case 'b':
					if(*p==0) p=argv[++argn];
					branch.limit=strtoul(p,NULL,0);
					break;
				case 'y':
					if(*p==0) p=argv[++argn];
					branch.yield_interval=strtoul(p,NULL,0);
					break;
				case 'g':
					if(*p==0) p=argv[++argn];
					branch.gc_interval=strtoul(p,NULL,0);
					break;
				case 'h':
					if(*p==0)
						gethostname(host_name=host_name_buf,sizeof(host_name_buf));
					else
						host_name=p;
					break;
				case 'L':
					if(*p==0) p=argv[++argn];
					log_mask=strtol(p,NULL,0);
					break;
				case 'E':
					if(*p==0) p=argv[++argn];
					err_level=strtol(p,NULL,0);
					break;
				case 'e':
					if(*p==0) p=argv[++argn];
					if((errfp=fopen(p,omode))==NULL) {
						perror(p);
						bail(1);
					}
					break;
				case 'o':
					if(*p==0) p=argv[++argn];
					if((confp=fopen(p,omode))==NULL) {
						perror(p);
						bail(1);
					}
					break;
				case 'q':
					confp=nulfp;
					break;
				case 'n':
					statfp=nulfp;
					break;
				case 'x':
					branch.auto_terminate=FALSE;
					break;
				case 'l':
					loop=TRUE;
					break;
				case 'p':
					pause_on_exit=TRUE;
					break;
				case '!':
					pause_on_error=TRUE;
					break;
				case 'c':
					if(*p==0) p=argv[++argn];
					SAFECOPY(scfg.ctrl_dir,p);
					break;
				default:
					fprintf(errfp,"\n!Unsupported option: %s\n",argv[argn]);
				case '?':
					usage(errfp);
					bail(1);
			}
			continue;
		}
		module=argv[argn];
	}

	if(scfg.ctrl_dir[0]==0) {
		if((p=getenv("SBBSCTRL"))==NULL) {
			lprintf(LOG_ERR,"\nSBBSCTRL environment variable not set and -c option not specified.\n");
			lprintf(LOG_ERR,"\nExample: SET SBBSCTRL=/sbbs/ctrl\n");
			lprintf(LOG_ERR,"\n     or: %s -c /sbbs/ctrl [module]\n",argv[0]);
			bail(1); 
		}
		SAFECOPY(scfg.ctrl_dir,p);
	}	

	if(module==NULL && isatty(fileno(stdin))) {
		lprintf(LOG_ERR,"\n!Module name not specified\n");
		usage(errfp);
		bail(1); 
	}

	banner(statfp);

	if(chdir(scfg.ctrl_dir)!=0)
		lprintf(LOG_ERR,"!ERROR changing directory to: %s", scfg.ctrl_dir);

	fprintf(statfp,"\nLoading configuration files from %s\n",scfg.ctrl_dir);
	if(!load_cfg(&scfg,NULL,TRUE,error)) {
		lprintf(LOG_ERR,"!ERROR loading configuration files: %s\n",error);
		bail(1);
	}
	prep_dir(scfg.data_dir, scfg.temp_dir, sizeof(scfg.temp_dir));

	if(host_name==NULL)
		host_name=scfg.sys_inetaddr;

	if(!(scfg.sys_misc&SM_LOCAL_TZ))
		putenv("TZ=UTC0");

	/* Don't cache error log */
	setvbuf(errfp,NULL,_IONBF,0);

	/* Install Ctrl-C/Break signal handler here */
#if defined(_WIN32)
	SetConsoleCtrlHandler(ControlHandler, TRUE /* Add */);
#elif defined(__unix__)
	signal(SIGQUIT,break_handler);
	signal(SIGINT,break_handler);
	signal(SIGTERM,break_handler);

	/* Don't die on SIGPIPE or SIGHUP */
	signal(SIGPIPE,SIG_IGN);
	signal(SIGHUP,SIG_IGN);
#endif

	do {

		if(!js_init(environ)) {
			lprintf(LOG_ERR,"!JavaScript initialization failure\n");
			bail(1);
		}
		fprintf(statfp,"\n");

		result=js_exec(module,&argv[argn]);

		fprintf(statfp,"\n");
		fprintf(statfp,"JavaScript: Destroying context\n");
		JS_DestroyContext(js_cx);
		fprintf(statfp,"JavaScript: Destroying runtime\n");
		JS_DestroyRuntime(js_runtime);	

	} while(loop && !terminated);

	bail(result);

	return(-1);	/* never gets here */
}

