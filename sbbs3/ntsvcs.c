/* ntsrvcs.c */

/* Synchronet BBS as a set of Windows NT Services */

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

/* Synchronet-specific headers */
#include "sbbs.h"		/* various */
#include "sbbs_ini.h"	/* sbbs_read_ini() */
#include "ftpsrvr.h"	/* ftp_startup_t, ftp_server */
#include "websrvr.h"	/* web_startup_t, web_server */
#include "mailsrvr.h"	/* mail_startup_t, mail_server */
#include "services.h"	/* services_startup_t, services_thread */
#include "ntsvcs.h"		/* NT service names */

/* Windows-specific headers */
#include <winsvc.h>

#define NTSVC_TIMEOUT_STARTUP	30000	/* Milliseconds */
#define NTSVC_TIMEOUT_TERMINATE	30000	/* Milliseconds */


static void WINAPI bbs_ctrl_handler(DWORD dwCtrlCode);
static void WINAPI ftp_ctrl_handler(DWORD dwCtrlCode);
static void WINAPI web_ctrl_handler(DWORD dwCtrlCode);
static void WINAPI mail_ctrl_handler(DWORD dwCtrlCode);
static void WINAPI services_ctrl_handler(DWORD dwCtrlCode);

/* Global variables */

bbs_startup_t		bbs_startup;
ftp_startup_t		ftp_startup;
mail_startup_t		mail_startup;
services_startup_t	services_startup;
web_startup_t		web_startup;

typedef struct {
	char*					name;
	char*					display_name;
	char*					description;
	void*					startup;
	BOOL*					recycle_now;
	DWORD*					log_mask;
	void					(*thread)(void* arg);
	void					(*terminate)(void);
	void					(WINAPI *ctrl_handler)(DWORD);
	HANDLE					log_handle;
	HANDLE					event_handle;
	BOOL					autostart;
	BOOL					debug;
	SERVICE_STATUS			status;
	SERVICE_STATUS_HANDLE	status_handle;
} sbbs_ntsvc_t;

sbbs_ntsvc_t bbs ={	
	NTSVC_NAME_BBS,
	"Synchronet Telnet/RLogin Server",
	"Provides support for Telnet and RLogin clients and executes timed events. " \
		"This service provides the critical functions of your Synchronet BBS.",
	&bbs_startup,
	&bbs_startup.recycle_now,
	&bbs_startup.log_mask,
	bbs_thread,
	bbs_terminate,
	bbs_ctrl_handler,
	INVALID_HANDLE_VALUE
};

/* This is not (currently) a separate service, use this for logging only */
sbbs_ntsvc_t event ={	
	NTSVC_NAME_EVENT,
	NULL,
	NULL,
	NULL,
	NULL,
	&bbs_startup.log_mask,
	NULL,
	NULL,
	NULL,
	INVALID_HANDLE_VALUE
};

sbbs_ntsvc_t ftp = {
	NTSVC_NAME_FTP,
	"Synchronet FTP Server",
	"Provides support for FTP clients (including web browsers) for file transfers.",
	&ftp_startup,
	&ftp_startup.recycle_now,
	&ftp_startup.log_mask,
	ftp_server,
	ftp_terminate,
	ftp_ctrl_handler,
	INVALID_HANDLE_VALUE
};

#if !defined(NO_WEB_SERVER)
sbbs_ntsvc_t web = {
	NTSVC_NAME_WEB,
	"Synchronet Web Server",
	"Provides support for Web (HTML/HTTP) clients (browsers).",
	&web_startup,
	&web_startup.recycle_now,
	&web_startup.log_mask,
	web_server,
	web_terminate,
	web_ctrl_handler,
	INVALID_HANDLE_VALUE
};
#endif

sbbs_ntsvc_t mail = {
	NTSVC_NAME_MAIL,
	"Synchronet SMTP/POP3 Mail Server",
	"Sends and receives Internet e-mail (using SMTP) and allows users to remotely " \
		"access their e-mail using an Internet mail client (using POP3).",
	&mail_startup,
	&mail_startup.recycle_now,
	&mail_startup.log_mask,
	mail_server,
	mail_terminate,
	mail_ctrl_handler,
	INVALID_HANDLE_VALUE
};

#if !defined(NO_SERVICES)
sbbs_ntsvc_t services = {
	NTSVC_NAME_SERVICES,
	"Synchronet Services",
	"Plugin servers (usually in JavaScript) for any TCP/UDP protocol. " \
		"Stock services include Finger, Gopher, NNTP, and IRC. Edit your ctrl/services.ini " \
		"file for configuration of individual Synchronet Services.",
	&services_startup,
	&services_startup.recycle_now,
	&services_startup.log_mask,
	services_thread,
	services_terminate,
	services_ctrl_handler,
	INVALID_HANDLE_VALUE
};
#endif

/* This list is used for enumerating all services */
sbbs_ntsvc_t* ntsvc_list[] = {
	&bbs,
	&ftp,
#if !defined(NO_WEB_SERVER)
	&web,
#endif
	&mail,
#if !defined(NO_SERVICES)
	&services,
#endif
	NULL
};
							
/****************************************/
/* Service Control Handlers (Callbacks) */
/****************************************/

/* Common control handler for all services */
static void svc_ctrl_handler(sbbs_ntsvc_t* svc, DWORD dwCtrlCode)
{
	switch(dwCtrlCode) {
		case SERVICE_CONTROL_RECYCLE:
			*svc->recycle_now=TRUE;
			break;
		case SERVICE_CONTROL_STOP:
		case SERVICE_CONTROL_SHUTDOWN:
			svc->terminate();
			svc->status.dwWaitHint=NTSVC_TIMEOUT_TERMINATE;
			svc->status.dwCurrentState=SERVICE_STOP_PENDING;
			break;
	}
	SetServiceStatus(svc->status_handle, &svc->status);
}

/* Service-specific control handler stub functions */
static void WINAPI bbs_ctrl_handler(DWORD dwCtrlCode)
{
	svc_ctrl_handler(&bbs, dwCtrlCode);
}

static void WINAPI ftp_ctrl_handler(DWORD dwCtrlCode)
{
	svc_ctrl_handler(&ftp, dwCtrlCode);
}

#if !defined(NO_WEB_SERVER)
static void WINAPI web_ctrl_handler(DWORD dwCtrlCode)
{
	svc_ctrl_handler(&web, dwCtrlCode);
}
#endif

static void WINAPI mail_ctrl_handler(DWORD dwCtrlCode)
{
	svc_ctrl_handler(&mail, dwCtrlCode);
}

#if !defined(NO_SERVICES)
static void WINAPI services_ctrl_handler(DWORD dwCtrlCode)
{
	svc_ctrl_handler(&services, dwCtrlCode);
}
#endif

static WORD event_type(int level)
{
	switch(level) {
		case LOG_WARNING:
			return(EVENTLOG_WARNING_TYPE);
		case LOG_NOTICE:
		case LOG_INFO:
		case LOG_DEBUG:
			return(EVENTLOG_INFORMATION_TYPE);
	}
/*
	LOG_EMERG
	LOG_ALERT
	LOG_CRIT
	LOG_ERR
*/
	return(EVENTLOG_ERROR_TYPE);
}

/**************************************/
/* Common Service Log Ouptut Function */
/**************************************/
static int svc_lputs(void* p, int level, char* str)
{
	char	debug[1024];
	char	fname[256];
	DWORD	len;
	DWORD	wr;
	sbbs_ntsvc_t* svc = (sbbs_ntsvc_t*)p;

	/* Debug Logging */
	if(svc==NULL || svc->debug) {
		snprintf(debug,sizeof(debug),"%s: %s",svc==NULL ? "Synchronet" : svc->name, str);
		OutputDebugString(debug);
	}
	if(svc==NULL)
		return(0);

	len = strlen(str);

	/* Mailslot Logging (for sbbsctrl) */
	if(svc->log_handle != INVALID_HANDLE_VALUE /* Invalid log handle? */
		&& !GetMailslotInfo(svc->log_handle, NULL, NULL, NULL, NULL)) {
		/* Close and try to re-open */
		CloseHandle(svc->log_handle);
		svc->log_handle=INVALID_HANDLE_VALUE;
	}

	if(svc->log_handle == INVALID_HANDLE_VALUE) {
		sprintf(fname,"\\\\.\\mailslot\\%s.log",svc->name);
		svc->log_handle = CreateFile(
			fname,					// pointer to name of the file
			GENERIC_WRITE,			// access (read-write) mode
			FILE_SHARE_READ,		// share mode
			NULL,					// pointer to security attributes
			OPEN_EXISTING,			// how to create
			FILE_ATTRIBUTE_NORMAL,  // file attributes
			NULL					// handle to file with attributes to copy
			);
	}
	if(svc->log_handle != INVALID_HANDLE_VALUE) {
		if(!WriteFile(svc->log_handle,str,len,&wr,NULL) || wr!=len) {
			/* This most likely indicates the server closed the mailslot */
			sprintf(debug,"!ERROR %d writing %u bytes to %s pipe (wr=%d)"
				,GetLastError(),len,svc->name,wr);
			OutputDebugString(debug);
			/* So close the handle and attempt re-open next time */
			CloseHandle(svc->log_handle);
			svc->log_handle=INVALID_HANDLE_VALUE;
		}
	}
	
	/* Event Logging */
	if((*svc->log_mask)&(1<<level)) {
		if(svc->event_handle == NULL)
			svc->event_handle = RegisterEventSource(
				NULL,		// server name for source (NULL = local computer)
				svc->name   // source name for registered handle
				);
		if(svc->event_handle != NULL)
			ReportEvent(svc->event_handle,  // event log handle
				event_type(level),		// event type
				0,						// category zero
				0,						// event identifier
				NULL,					// no user security identifier
				1,						// one string
				0,						// no data
				&str,					// pointer to string array
				NULL);					// pointer to data
	}

    return(0);
}

/****************************************************************************/
/* Event thread local/log print routine										*/
/****************************************************************************/
static int event_lputs(int level, char *str)
{
	svc_lputs(&event,level,str);
    return(0);
}

/************************************/
/* Shared Service Callback Routines */
/************************************/

static void svc_started(void* p)
{
	sbbs_ntsvc_t* svc = (sbbs_ntsvc_t*)p;

	svc->status.dwCurrentState=SERVICE_RUNNING;
	svc->status.dwControlsAccepted|=SERVICE_ACCEPT_STOP;
	SetServiceStatus(svc->status_handle, &svc->status);
}

static void svc_terminated(void* p, int code)
{
	sbbs_ntsvc_t* svc = (sbbs_ntsvc_t*)p;

	if(code) {
		svc->status.dwWin32ExitCode=ERROR_SERVICE_SPECIFIC_ERROR;
		svc->status.dwServiceSpecificExitCode=code;
		SetServiceStatus(svc->status_handle, &svc->status);
	}
}

static void svc_clients(void* p, int active)
{
	sbbs_ntsvc_t* svc = (sbbs_ntsvc_t*)p;
}

/***************/
/* ServiceMain */
/***************/

/* Common ServiceMain for all services */
static void WINAPI svc_main(sbbs_ntsvc_t* svc, DWORD argc, LPTSTR *argv)
{
	char	str[256];
	DWORD	i;
	char*	arg;

	for(i=0;i<argc;i++) {
		arg=argv[i];
		if(*arg=='-' || *arg=='/')
			arg++;
		if(!stricmp(arg,"debug"))
			svc->debug=TRUE;
		if(!stricmp(arg,"logmask") && i+1<argc)
			(*svc->log_mask)=strtol(argv[++i],NULL,0);
	}

	sprintf(str,"Starting NT Service: %s",svc->display_name);
	svc_lputs(svc,LOG_INFO,str);

    if((svc->status_handle = RegisterServiceCtrlHandler(svc->name, svc->ctrl_handler))==0) {
		sprintf(str,"!ERROR %d registering service control handler",GetLastError());
		svc_lputs(NULL,LOG_ERR,str);
		return;
	}

	memset(&svc->status,0,sizeof(SERVICE_STATUS));
	svc->status.dwServiceType=SERVICE_WIN32_SHARE_PROCESS;
	svc->status.dwControlsAccepted=SERVICE_ACCEPT_SHUTDOWN;
	svc->status.dwWaitHint=NTSVC_TIMEOUT_STARTUP;

	svc->status.dwCurrentState=SERVICE_START_PENDING;
	SetServiceStatus(svc->status_handle, &svc->status);

	svc->thread(svc->startup);

	svc->status.dwCurrentState=SERVICE_STOPPED;
	SetServiceStatus(svc->status_handle, &svc->status);

	if(svc->log_handle!=INVALID_HANDLE_VALUE) {
		CloseHandle(svc->log_handle);
		svc->log_handle=INVALID_HANDLE_VALUE;
	}

	if(svc->event_handle!=NULL) {
		DeregisterEventSource(svc->event_handle);
		svc->event_handle=NULL;
	}
}

/* Service-specific ServiceMain stub functions */

static void WINAPI bbs_start(DWORD dwArgc, LPTSTR *lpszArgv)
{
	svc_main(&bbs, dwArgc, lpszArgv);

	/* Events are (currently) part of the BBS service */
	if(event.log_handle!=INVALID_HANDLE_VALUE) {
		CloseHandle(event.log_handle);
		event.log_handle=INVALID_HANDLE_VALUE;
	}
}

static void WINAPI ftp_start(DWORD dwArgc, LPTSTR *lpszArgv)
{
	svc_main(&ftp, dwArgc, lpszArgv);
}

#if !defined(NO_WEB_SERVER)
static void WINAPI web_start(DWORD dwArgc, LPTSTR *lpszArgv)
{
	svc_main(&web, dwArgc, lpszArgv);
}
#endif

static void WINAPI mail_start(DWORD dwArgc, LPTSTR *lpszArgv)
{
	svc_main(&mail, dwArgc, lpszArgv);
}

#if !defined(NO_SERVICES)
static void WINAPI services_start(DWORD dwArgc, LPTSTR *lpszArgv)
{
	svc_main(&services, dwArgc, lpszArgv);
}
#endif

/******************************************/
/* NT Serivce Install/Uninstall Functions */
/******************************************/

/* ChangeServiceConfig2 is a Win2K+ API function, must call dynamically */
typedef WINADVAPI BOOL (WINAPI *ChangeServiceConfig2_t)(SC_HANDLE, DWORD, LPCVOID);

static void describe_service(HANDLE hSCMlib, SC_HANDLE hService, char* description)
{
	ChangeServiceConfig2_t changeServiceConfig2;
	static SERVICE_DESCRIPTION service_desc;
  
	if(hSCMlib==NULL)
		return;

	service_desc.lpDescription=description;

	if((changeServiceConfig2 = (ChangeServiceConfig2_t)GetProcAddress(hSCMlib, "ChangeServiceConfig2A"))!=NULL)
		changeServiceConfig2(hService, SERVICE_CONFIG_DESCRIPTION, &service_desc);
}

/****************************************************************************/
/* Utility function to create a service with description (on Win2K+)		*/
/****************************************************************************/
static SC_HANDLE create_service(HANDLE hSCMlib, SC_HANDLE hSCManager
								,char* name, char* display_name, char* description, char* path
								,BOOL autostart)
{
    SC_HANDLE   hService;

	printf("Installing service: %-40s ... ", display_name);

    hService = CreateService(
        hSCManager,						// SCManager database
        name,							// name of service
        display_name,					// name to display
        SERVICE_ALL_ACCESS,				// desired access
        SERVICE_WIN32_SHARE_PROCESS,	// service type
		autostart						// start type (auto or manual)
			? SERVICE_AUTO_START : SERVICE_DEMAND_START,				
        SERVICE_ERROR_NORMAL,			// error control type
        path,							// service's binary
        NULL,							// no load ordering group
        NULL,							// no tag identifier
        "",								// dependencies
        NULL,							// LocalSystem account
        NULL);							// no password

	if(hService==NULL)
		printf("!ERROR %d\n",GetLastError());
	else {
		describe_service(hSCMlib, hService,description);
		CloseServiceHandle(hService);
		printf("Successful\n");
	}

	return(hService);
}

/****************************************************************************/
/* Install one or all services												*/
/****************************************************************************/
static int install(const char* svc_name)
{
	int			i;
	HANDLE		hSCMlib;
    SC_HANDLE   hSCManager;
    char		path[MAX_PATH+1];

	printf("Installing Synchronet NT Services...\n");

	hSCMlib = LoadLibrary("ADVAPI32.DLL");

    if(GetModuleFileName(NULL,path,sizeof(path))==0)
    {
        fprintf(stderr,"!ERROR %d getting module file name\n",GetLastError());
        return(-1);
    }

    hSCManager = OpenSCManager(
                        NULL,                   // machine (NULL == local)
                        NULL,                   // database (NULL == default)
                        SC_MANAGER_ALL_ACCESS   // access required
                        );
    if(hSCManager==NULL) {
		fprintf(stderr,"!ERROR %d opening SC manager\n",GetLastError());
		return(-1);
	}

	for(i=0;ntsvc_list[i]!=NULL;i++)
		if(svc_name==NULL	/* All? */
			|| !stricmp(ntsvc_list[i]->name, svc_name))
			create_service(hSCMlib
				,hSCManager
				,ntsvc_list[i]->name
				,ntsvc_list[i]->display_name
				,ntsvc_list[i]->description
				,path
				,ntsvc_list[i]->autostart);

	if(hSCMlib!=NULL)
		FreeLibrary(hSCMlib);

	CloseServiceHandle(hSCManager);

	return(0);
}

/****************************************************************************/
/* Utility function to remove a service cleanly (stopping if necessary)		*/
/****************************************************************************/
static void remove_service(SC_HANDLE hSCManager, char* name, char* disp_name)
{
    SC_HANDLE		hService;
	SERVICE_STATUS	status;

	printf("Removing service: %-40s ... ", disp_name);

    hService = OpenService(hSCManager, name, SERVICE_ALL_ACCESS);

	if(hService==NULL) {
		printf("\n!ERROR %d opening service: %s\n",GetLastError(),name);
		return;
	}

    // try to stop the service
    if(ControlService( hService, SERVICE_CONTROL_STOP, &status))
    {
        printf("\nStopping: %s ... ",name);

        while(QueryServiceStatus(hService, &status) && status.dwCurrentState == SERVICE_STOP_PENDING)
			Sleep(1000);

        if(status.dwCurrentState == SERVICE_STOPPED)
            printf("Stopped.\n");
        else
            printf("FAILED!\n");
    }

    // now remove the service
    if(DeleteService(hService))
		printf("Successful\n");
	else
		printf("!ERROR %d\n",GetLastError());
    CloseServiceHandle(hService);
}

/****************************************************************************/
/* Uninstall one or all services											*/
/****************************************************************************/
static int uninstall(const char* svc_name)
{
	int			i;
    SC_HANDLE   hSCManager;

    hSCManager = OpenSCManager(
                        NULL,                   // machine (NULL == local)
                        NULL,                   // database (NULL == default)
                        SC_MANAGER_ALL_ACCESS   // access required
                        );
    if(hSCManager==NULL) {
		fprintf(stderr,"!ERROR %d opening SC manager\n",GetLastError());
		return(-1);
	}

	for(i=0;ntsvc_list[i]!=NULL;i++)
		if(svc_name==NULL	/* All? */
			|| !stricmp(ntsvc_list[i]->name, svc_name))
			remove_service(hSCManager
				,ntsvc_list[i]->name
				,ntsvc_list[i]->display_name);

	CloseServiceHandle(hSCManager);

	return(0);
}

/****************************************************************************/
/* Utility function to disable a service									*/
/****************************************************************************/
static void set_service_start_type(SC_HANDLE hSCManager, char* name
								   ,char* disp_name, DWORD start_type)
{
    SC_HANDLE		hService;

	printf("Disabling service: %-40s ... ", disp_name);

    hService = OpenService(hSCManager, name, SERVICE_ALL_ACCESS);

	if(hService==NULL) {
		printf("\n!ERROR %d opening service: %s\n",GetLastError(),name);
		return;
	}

	if(!ChangeServiceConfig(
		hService,				// handle to service
		SERVICE_NO_CHANGE,		// type of service
		start_type,				// when to start service
		SERVICE_NO_CHANGE,		// severity if service fails to start
		NULL,					// pointer to service binary file name
		NULL,					// pointer to load ordering group name
		NULL,					// pointer to variable to get tag identifier
		NULL,					// pointer to array of dependency names
		NULL,					// pointer to account name of service
		NULL,					// pointer to password for service account
		NULL					// pointer to display name
		))
		printf("\n!ERROR %d disabling service: %s\n",GetLastError(),name);

    CloseServiceHandle(hService);
}

/****************************************************************************/
/* Disable one or all services												*/
/****************************************************************************/
static int disable(const char* svc_name)
{
	int			i;
    SC_HANDLE   hSCManager;

    hSCManager = OpenSCManager(
                        NULL,                   // machine (NULL == local)
                        NULL,                   // database (NULL == default)
                        SC_MANAGER_ALL_ACCESS   // access required
                        );
    if(hSCManager==NULL) {
		fprintf(stderr,"!ERROR %d opening SC manager\n",GetLastError());
		return(-1);
	}

	for(i=0;ntsvc_list[i]!=NULL;i++)
		if(svc_name==NULL	/* All? */
			|| !stricmp(ntsvc_list[i]->name, svc_name))
			set_service_start_type(hSCManager
				,ntsvc_list[i]->name
				,ntsvc_list[i]->display_name
				,SERVICE_DISABLED);

	CloseServiceHandle(hSCManager);

	return(0);
}

/****************************************************************************/
/* Enable (set to auto-start) one or all services							*/
/****************************************************************************/
static int enable(const char* svc_name)
{
	int			i;
    SC_HANDLE   hSCManager;

    hSCManager = OpenSCManager(
                        NULL,                   // machine (NULL == local)
                        NULL,                   // database (NULL == default)
                        SC_MANAGER_ALL_ACCESS   // access required
                        );
    if(hSCManager==NULL) {
		fprintf(stderr,"!ERROR %d opening SC manager\n",GetLastError());
		return(-1);
	}

	for(i=0;ntsvc_list[i]!=NULL;i++)
		if(svc_name==NULL	/* All? */
			|| !stricmp(ntsvc_list[i]->name, svc_name))
			set_service_start_type(hSCManager
				,ntsvc_list[i]->name
				,ntsvc_list[i]->display_name
				,SERVICE_AUTO_START);

	CloseServiceHandle(hSCManager);

	return(0);
}


/****************************************************************************/
/* Main Entry Point															*/
/****************************************************************************/
int main(int argc, char** argv)
{
	char*	ctrl_dir;
	char*	arg;
	char*	p;
	char	str[MAX_PATH+1];
	char	ini_file[MAX_PATH+1];
	int		i;
	FILE*	fp=NULL;

	SERVICE_TABLE_ENTRY  ServiceDispatchTable[] = 
    { 
        { bbs.name,			bbs_start		}, 
		{ ftp.name,			ftp_start		},
#if !defined(NO_WEB_SERVER)
		{ web.name,			web_start		},
#endif
		{ mail.name,		mail_start		},
#if !defined(NO_SERVICES)
		{ services.name,	services_start	},
#endif
        { NULL,				NULL			}	/* Terminator */
    }; 

	printf("\nSynchronet NT Services  Version %s%c  %s\n\n"
		,VERSION,REVISION,COPYRIGHT_NOTICE);

	ctrl_dir=getenv("SBBSCTRL");	/* read from environment variable */
	if(ctrl_dir==NULL || ctrl_dir[0]==0) {
		ctrl_dir="\\sbbs\\ctrl";		/* Not set? Use default */
		printf("!SBBSCTRL environment variable not set, using default value: %s\n\n"
			,ctrl_dir);
	}

	sbbs_get_ini_fname(ini_file, ctrl_dir, NULL /* auto-host_name */);

	/* Initialize BBS startup structure */
    memset(&bbs_startup,0,sizeof(bbs_startup));
    bbs_startup.size=sizeof(bbs_startup);
	bbs_startup.cbdata=&bbs;
	bbs_startup.lputs=svc_lputs;
	bbs_startup.event_lputs=event_lputs;
    bbs_startup.started=svc_started;
    bbs_startup.terminated=svc_terminated;
	bbs_startup.clients=svc_clients;
    strcpy(bbs_startup.ctrl_dir,ctrl_dir);

	/* Initialize FTP startup structure */
    memset(&ftp_startup,0,sizeof(ftp_startup));
	ftp_startup.cbdata=&ftp;
    ftp_startup.size=sizeof(ftp_startup);
	ftp_startup.lputs=svc_lputs;
    ftp_startup.started=svc_started;
    ftp_startup.terminated=svc_terminated;
	ftp_startup.clients=svc_clients;
    strcpy(ftp_startup.ctrl_dir,ctrl_dir);

#if !defined(NO_WEB_SERVER)
	/* Initialize Web Server startup structure */
    memset(&web_startup,0,sizeof(web_startup));
	web_startup.cbdata=&web;
    web_startup.size=sizeof(web_startup);
	web_startup.lputs=svc_lputs;
    web_startup.started=svc_started;
    web_startup.terminated=svc_terminated;
	web_startup.clients=svc_clients;
    strcpy(web_startup.ctrl_dir,ctrl_dir);
#endif

	/* Initialize Mail Server startup structure */
    memset(&mail_startup,0,sizeof(mail_startup));
	mail_startup.cbdata=&mail;
    mail_startup.size=sizeof(mail_startup);
	mail_startup.lputs=svc_lputs;
    mail_startup.started=svc_started;
    mail_startup.terminated=svc_terminated;
	mail_startup.clients=svc_clients;
    strcpy(mail_startup.ctrl_dir,ctrl_dir);

#if !defined(NO_SERVICES)
	/* Initialize Services startup structure */
    memset(&services_startup,0,sizeof(services_startup));
	services_startup.cbdata=&services;
    services_startup.size=sizeof(services_startup);
	services_startup.lputs=svc_lputs;
    services_startup.started=svc_started;
    services_startup.terminated=svc_terminated;
	services_startup.clients=svc_clients;
    strcpy(services_startup.ctrl_dir,ctrl_dir);
#endif

	/* Read .ini file here */
	if((fp=fopen(ini_file,"r"))!=NULL) {
		sprintf(str,"Reading %s",ini_file);
		svc_lputs(NULL,LOG_INFO,str);
	}

	/* We call this function to set defaults, even if there's no .ini file */
	sbbs_read_ini(fp 
		,NULL	/* global_startup */
		,&bbs.autostart			,&bbs_startup
		,&ftp.autostart			,&ftp_startup 
#if defined(NO_WEB_SERVER
		,NULL					,NULL
#else
		,&web.autostart			,&web_startup
#endif
		,&mail.autostart		,&mail_startup 
#if defined(NO_SERVICES)
		,NULL					,NULL
#else
		,&services.autostart	,&services_startup
#endif
		);

	/* close .ini file here */
	if(fp!=NULL)
		fclose(fp);

	ctrl_dir = bbs_startup.ctrl_dir;
	if(chdir(ctrl_dir)!=0) {
		sprintf(str,"!ERROR %d changing directory to: %s", errno, ctrl_dir);
		svc_lputs(NULL,LOG_ERR,str);
	}

	for(i=1;i<argc;i++) {
		arg=argv[i];
		while(*arg=='-')
			arg++;
		if(!stricmp(arg,"install"))
			return install(argv[i+1]);

		if(!stricmp(arg,"remove"))
			return uninstall(argv[i+1]);

		if(!stricmp(arg,"disable"))
			return disable(argv[i+1]);

		if(!stricmp(arg,"enable"))
			return enable(argv[i+1]);
	}

	printf("Available Services:\n\n");
	printf("%-20s %s\n","Name","Description");
	printf("%-20s %s\n","----","-----------");
	for(i=0;ntsvc_list[i]!=NULL;i++)
		printf("%-20s %s\n",ntsvc_list[i]->name,ntsvc_list[i]->display_name);

	SAFECOPY(str,getfname(argv[0]));
	if((p=getfext(str))!=NULL)
		*p=0;

	printf("\nUsage: %s [command] [service]\n", str);

	printf("\nCommands:\n");
    printf("\tinstall\t to install a service by name (default: ALL services)\n");
    printf("\tremove\t to remove a service by name (default: ALL services)\n");
    printf("\tdisable\t to disable a service by name (default: ALL services)\n");
    printf("\tenable\t to re-enable a disabled service (default: ALL services)\n");

    printf("\nStartServiceCtrlDispatcher being called.\n" );
    printf("This may take several seconds.  Please wait.\n" );

	if(!StartServiceCtrlDispatcher(ServiceDispatchTable)) 
    { 
		sprintf(str,"!StartServiceCtrlDispatcher ERROR %d",GetLastError());
		printf("%s\n",str);
        OutputDebugString(str); 
    } 

	return(0);
}
