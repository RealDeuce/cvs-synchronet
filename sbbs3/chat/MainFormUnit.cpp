/* MainFormUnit.cpp */

/* Local sysop chat module (GUI Borland C++ Builder Project for Win32) */

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

//---------------------------------------------------------------------------

#include <vcl.h>
#include <io.h>
#include <sys\stat.h>
#include <fcntl.h>
#include <share.h>
#include <sys/locking.h>
#include <vcl\Registry.hpp>	/* TRegistry */
#pragma hdrstop

#include "gen_defs.h"       /* BS and DEL */

#define PCHAT_LEN 1000		/* Size of Private chat file */
#define REG_KEY "\\Software\\Swindell\\Synchronet Chat\\"

#include "MainFormUnit.h"
//---------------------------------------------------------------------------
#pragma package(smart_init)
#pragma resource "*.dfm"
TMainForm *MainForm;
extern int      node_num;
extern char     ctrl_dir[];
extern char     node_dir[];
extern char     user_name[];
//---------------------------------------------------------------------------
__fastcall TMainForm::TMainForm(TComponent* Owner)
    : TForm(Owner)
{
    in=-1;
    out=-1;
    nodedab=-1;
}
bool __fastcall TMainForm::ToggleChat(bool on)
{
    int         n=node_num-1;
    static int  org_act;

    if(nodedab==-1)
        return(false);

    lseek(nodedab, n*sizeof(node_t), SEEK_SET);
    int i=locking(nodedab, LK_LOCK, sizeof(node_t));
    if(i) {
        Remote->Lines->Add("!Error "+AnsiString(i)+" reading record for"
            "node "+AnsiString(node_num));
        return(false);
    }
    read(nodedab, &node, sizeof(node_t));
    if(on) {
        org_act=node.action;
        if(org_act==NODE_PCHT)
            org_act=NODE_MAIN;
        node.misc|=NODE_LCHAT;
    } else {
        node.action=org_act;
        node.misc&=~NODE_LCHAT;
    }

    lseek(nodedab, n*sizeof(node_t), SEEK_SET);
    write(nodedab, &node, sizeof(node_t));
    lseek(nodedab, n*sizeof(node_t), SEEK_SET);
    locking(nodedab, LK_UNLCK, sizeof(node_t));

    return(true);
}
//---------------------------------------------------------------------------
void __fastcall TMainForm::FormShow(TObject *Sender)
{
    char    path[MAX_PATH+1];
    char*   p;

    Caption="Waiting for "
        +AnsiString(user_name)+" on Node "+AnsiString(node_num);

    wsprintf(path,"%sNODE.DAB",ctrl_dir);
    nodedab=_sopen(path,O_RDWR|O_BINARY|O_CREAT, SH_DENYNONE,S_IREAD|S_IWRITE);

    if(nodedab==-1) {
        Remote->Lines->Add("!Error opening NODE.DAB");
        return;
    }

    ToggleChat(true);

	wsprintf(path,"%sLCHAT.DAB",node_dir);
	if((out=_sopen(path,O_RDWR|O_CREAT|O_BINARY,O_DENYNONE
		,S_IREAD|S_IWRITE))==-1) {
		Remote->Lines->Add("!Error opening LCHAT.DAB");
		return; }

	wsprintf(path,"%sCHAT.DAB",node_dir);
#if 0
	if(!fexist(path))		/* Wait while it's created for the first time */
		mswait(2000);
#endif

	if((in=_sopen(path,O_RDWR|O_CREAT|O_BINARY,O_DENYNONE
		,S_IREAD|S_IWRITE))==-1) {
		close(out);
		Remote->Lines->Add("!Error opening CHAT.DAB");
		return;
    }

	if((p=(char *)MALLOC(PCHAT_LEN))==NULL) {
		close(in);
		close(out);
		Remote->Lines->Add("!Error allocating memory");
		return;
    }
	memset(p,0,PCHAT_LEN);
	write(in,p,PCHAT_LEN);
	write(out,p,PCHAT_LEN);
	FREE(p);
	lseek(in,0L,SEEK_SET);
	lseek(out,0L,SEEK_SET);

    Timer->Enabled=true;
}
//---------------------------------------------------------------------------
void __fastcall TMainForm::FormClose(TObject *Sender, TCloseAction &Action)
{
    ToggleChat(false);

    if(in!=-1)
        close(in);
    if(out!=-1)
        close(out);
    if(nodedab!=-1)
        close(nodedab);

    // Write Registry keys
	TRegistry* Registry=new TRegistry;
    if(!Registry->OpenKey(REG_KEY,true)) {
    	Application->MessageBox("Error opening registry key"
        	,REG_KEY,MB_OK|MB_ICONEXCLAMATION);
        Application->Terminate();
    }
   	Registry->WriteInteger("FormHeight",Height);
   	Registry->WriteInteger("FormWidth",Width);
    Registry->WriteInteger("RemoteHeight",Remote->Height);

    Registry->CloseKey();
    delete Registry;

}
//---------------------------------------------------------------------------
void __fastcall TMainForm::LocalKeyPress(TObject *Sender, char &Key)
{
    char c;

    if(out==-1) {
        Beep();
        return;
    }
    read(out,&c,1);
    lseek(out,-1L,SEEK_CUR);
    if(!c)		/* hasn't wrapped */
        write(out,&Key,1);
    else {
        if(!tell(out))
            lseek(out,0L,SEEK_END);
        lseek(out,-1L,SEEK_CUR);
        c=0;
        write(out,&c,1);
        lseek(out,-1L,SEEK_CUR);
    }
    if(tell(out)>=PCHAT_LEN)
        lseek(out,0L,SEEK_SET);
}
//---------------------------------------------------------------------------
void __fastcall TMainForm::InputTimerTick(TObject *Sender)
{
    char ch=0;

    Timer->Interval=100;

    while(in!=-1) {
        if(tell(in)>=PCHAT_LEN)
            lseek(in,0L,SEEK_SET);
        read(in,&ch,1);
        lseek(in,-1L,SEEK_CUR);
        if(!ch)
            break;					  /* char from other node */

        Timer->Interval=1;
        
        /* Got char, display it */
        if(ch=='\r')
            Remote->Lines->Add("");
        else if(ch==BS || ch==DEL)    // backspace
            Remote->Lines->Text
                =Remote->Lines->Text.SetLength(Remote->Lines->Text.Length()-1);
        else {
            Remote->SelLength=0;
            Remote->SelStart=Remote->Lines->Text.Length();
            Remote->SelText=AnsiString(ch);
        }

        /* mark char as rx'd */
        ch=0;
        write(in,&ch,1);
    }
}
//---------------------------------------------------------------------------
void __fastcall TMainForm::TimerTick(TObject *Sender)
{
    if(nodedab==-1)
        return;

    int n=node_num-1;
    lseek(nodedab, n*sizeof(node_t), SEEK_SET);
    read(nodedab, &node, sizeof(node_t));
    if(node.misc&NODE_LCHAT)
        ;
    else if(Local->ReadOnly==true) {
        MainForm->Caption=AnsiString(user_name)+" on Node "
            +AnsiString(node_num);
        Local->ReadOnly=false;
    }
    else if(!node.status || node.status>NODE_QUIET || node.action!=NODE_PCHT) {
        OutputDebugString("CHAT: User Disconnected\r\n");
        MainForm->Caption=MainForm->Caption+" - Disconnected!";
        Timer->Enabled=false;
        Local->ReadOnly=true;
    }
}
//---------------------------------------------------------------------------

void __fastcall TMainForm::FormCreate(TObject *Sender)
{
    // Read Registry keys
	TRegistry* Registry=new TRegistry;
    if(!Registry->OpenKey(REG_KEY,true)) {
    	Application->MessageBox("Error opening registry key"
        	,REG_KEY,MB_OK|MB_ICONEXCLAMATION);
        Application->Terminate();
    }
   	if(Registry->ValueExists("FormHeight"))
	   	Height=Registry->ReadInteger("FormHeight");
   	if(Registry->ValueExists("FormWidth"))
	   	Width=Registry->ReadInteger("FormWidth");
   	if(Registry->ValueExists("RemoteHeight"))
	   	Remote->Height=Registry->ReadInteger("RemoteHeight");

    Registry->CloseKey();
    delete Registry;

}
//---------------------------------------------------------------------------

