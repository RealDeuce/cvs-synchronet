/* Synchronet Control Panel (GUI Borland C++ Builder Project for Win32) */

/* $Id: MainFormUnit.h,v 1.1 2000/10/10 11:26:47 rswindell Exp $ */

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
#ifndef MainFormUnitH
#define MainFormUnitH
//---------------------------------------------------------------------------
#include <Classes.hpp>
#include <Controls.hpp>
#include <StdCtrls.hpp>
#include <Forms.hpp>
#include <Menus.hpp>
#include <ComCtrls.hpp>
#include <ToolWin.hpp>
#include <ExtCtrls.hpp>
#include <ActnList.hpp>
//---------------------------------------------------------------------------
#include "scfgdefs.h"  	// scfg_t
#include "bbs_thrd.h" 	// bbs_startup_t
#include "mailsrvr.h"
#include "ftpsrvr.h"
#include <ImgList.hpp>
#include <Buttons.hpp>
#include <Graphics.hpp>
//---------------------------------------------------------------------------
class TMainForm : public TForm
{
__published:	// IDE-managed Components
    TMainMenu *MainMenu;
    TMenuItem *FileMenuItem;
    TMenuItem *ViewMenuItem;
    TMenuItem *ViewNodesMenuItem;
    TMenuItem *FileExitMenuItem;
	TMenuItem *ViewTelnetMenuItem;
	TMenuItem *ViewMailServerMenuItem;
	TToolBar *Toolbar;
	TToolButton *ViewTelnetButton;
	TMenuItem *ViewToolbarMenuItem;
	TMenuItem *TelnetMenuItem;
	TMenuItem *TelnetConfigureMenuItem;
	TMenuItem *TelnetStartMenuItem;
	TMenuItem *TelnetStopMenuItem;
	TActionList *ActionList;
	TAction *TelnetStart;
	TAction *TelnetStop;
	TAction *TelnetConfigure;
	TImageList *ImageList;
	TAction *NodeListStart;
	TAction *NodeListStop;
	TAction *MailStart;
	TAction *MailStop;
    TMenuItem *MailMenuItem;
    TMenuItem *MailConfigureMenuItem;
    TMenuItem *MailStartMenuItem;
    TAction *MailConfigure;
    TToolButton *ViewNodesButton;
    TToolButton *ViewMailServerButton;
    TMenuItem *MailStopMenuItem;
	TAction *ViewTelnet;
    TAction *ViewNodes;
    TAction *ViewMailServer;
	TAction *ViewFtpServer;
	TAction *FtpStart;
	TAction *FtpStop;
	TToolButton *ViewFtpServerButton;
	TMenuItem *ViewFtpServerMenuItem;
	TAction *FtpConfigure;
	TMenuItem *FtpMenuItem;
	TMenuItem *FtpConfigureMenuItem;
	TMenuItem *FtpStartMenuItem;
	TMenuItem *FtpStopMenuItem;
	TTimer *CloseTimer;
	TMenuItem *BBSMenuItem;
	TMenuItem *BBSConfigureMenuItem;
	TPanel *TopPanel;
	TSplitter *HorizontalSplitter;
	TPanel *BottomPanel;
	TTimer *StatsTimer;
	TAction *ViewStats;
	TToolButton *ViewStatsButton;
	TMenuItem *ViewStatsMenuItem;
	TPageControl *LowerLeftPageControl;
	TSplitter *Splitter3;
	TPageControl *LowerRightPageControl;
	TStatusBar *StatusBar;
	TPageControl *UpperLeftPageControl;
	TSplitter *Splitter2;
	TPageControl *UpperRightPageControl;
	TTimer *StartupTimer;
	TMenuItem *ViewStatusBarMenuItem;
	TImage *Logo;
	TMenuItem *HelpMenuItem;
	TMenuItem *HelpAboutMenuItem;
	TMenuItem *FileSoundMenuItem;
	TToolButton *SoundButton;
	TAction *SoundToggle;
	TMenuItem *BBSStatisticsLogMenuItem;
	TMenuItem *ForceNetworkCalloutMenuItem;
	TMenuItem *ForceTimedEventMenuItem;
	TMenuItem *N1;
	TMenuItem *BBSEditMenuItem;
	TMenuItem *BBSEditGuruBrain;
	TMenuItem *BBSEditTextStrings;
	TMenuItem *BBSEditColors;
	TMenuItem *BBSEditAnswer;
	TMenuItem *BBSEditSystemInfo;
	TMenuItem *BBSEditNewUserMsg;
	TMenuItem *BBSEditFeedbackMsg;
	TMenuItem *BBSEditFilters;
	TMenuItem *BBSEditNameFilter;
	TMenuItem *BBSEditIPFilter;
	TMenuItem *PhoneNumberPHONECAN1;
	TMenuItem *BBSEditFilenameFilter;
	TMenuItem *BBSEditNameFilterBadMsg;
	TMenuItem *BBSEditPhoneFilterBadMsg;
	TMenuItem *BBSEditFileFilterBadMsg;
	TMenuItem *BBSEditZipComment;
	TMenuItem *N2;
	TMenuItem *MailEditMenuItem;
	TMenuItem *MailEditAliasList;
    TTimer *UpTimer;
    TMenuItem *BBSViewErrorLogMenuItem;
    TAction *ChatToggle;
    TMenuItem *SysopAvailableforChat1;
    TToolButton *SysopAvailButton;
    TAction *ViewClients;
    TToolButton *ViewClientsButton;
    TMenuItem *Clients1;
    TMenuItem *UserMenuItem;
    TMenuItem *UserEditMenuItem;
    TToolButton *EditUserButton;
    TAction *UserEditor;
    TToolButton *ToolButton1;
    TMenuItem *N3;
    TMenuItem *FtpEditMenuItem;
    TMenuItem *FtpEditAliasList;
    TMenuItem *FtpEditLoginMessage;
    TMenuItem *FtpEditHelloMessage;
    TMenuItem *FtpEditGoodbyeMessage;
    TMenuItem *N4;
    TMenuItem *FileOpenMenuItem;
    TMenuItem *N5;
    void __fastcall FileExitMenuItemClick(TObject *Sender);
	void __fastcall ViewToolbarMenuItemClick(TObject *Sender);
	void __fastcall PanelUnDock(TObject *Sender, TControl *Client,
    	TWinControl *NewTarget, bool &Allow);
	void __fastcall FormClose(TObject *Sender, TCloseAction &Action);
	void __fastcall FormCloseQuery(TObject *Sender, bool &CanClose);
	void __fastcall TelnetStartExecute(TObject *Sender);
	void __fastcall TelnetStopExecute(TObject *Sender);
	void __fastcall TelnetConfigureExecute(TObject *Sender);
	void __fastcall NodeListStartExecute(TObject *Sender);
	void __fastcall NodeListStopExecute(TObject *Sender);
    void __fastcall MailConfigureExecute(TObject *Sender);
    void __fastcall MailStartExecute(TObject *Sender);
    void __fastcall MailStopExecute(TObject *Sender);
    void __fastcall ViewTelnetExecute(TObject *Sender);
    void __fastcall ViewNodesExecute(TObject *Sender);
    void __fastcall ViewMailServerExecute(TObject *Sender);
	void __fastcall ViewFtpServerExecute(TObject *Sender);
	void __fastcall FtpStartExecute(TObject *Sender);
	void __fastcall FtpStopExecute(TObject *Sender);
	void __fastcall FtpConfigureExecute(TObject *Sender);
	void __fastcall CloseTimerTimer(TObject *Sender);
	void __fastcall BBSConfigureMenuItemClick(TObject *Sender);
	void __fastcall FormCreate(TObject *Sender);
	void __fastcall FormShow(TObject *Sender);
	void __fastcall NodeCloseButtonClick(TObject *Sender);
	void __fastcall TelnetCloseButtonClick(TObject *Sender);
	void __fastcall MailCloseButtonClick(TObject *Sender);
	void __fastcall FtpCloseButtonClick(TObject *Sender);
	void __fastcall StatsTimerTick(TObject *Sender);
	void __fastcall ViewStatsExecute(TObject *Sender);
	void __fastcall StatsCloseButtonClick(TObject *Sender);
	void __fastcall StartupTimerTick(TObject *Sender);
	void __fastcall ViewStatusBarMenuItemClick(TObject *Sender);
	void __fastcall HelpAboutMenuItemClick(TObject *Sender);
	void __fastcall SoundToggleExecute(TObject *Sender);
	void __fastcall BBSStatisticsLogMenuItemClick(TObject *Sender);
	void __fastcall ForceTimedEventMenuItemClick(TObject *Sender);
	void __fastcall ForceNetworkCalloutMenuItemClick(TObject *Sender);
	void __fastcall BBSEditGuruBrainClick(TObject *Sender);
	void __fastcall BBSEditTextStringsClick(TObject *Sender);
	void __fastcall BBSEditColorsClick(TObject *Sender);
	void __fastcall BBSEditAnswerClick(TObject *Sender);
	void __fastcall BBSEditZipCommentClick(TObject *Sender);
	void __fastcall BBSEditSystemInfoClick(TObject *Sender);
	void __fastcall BBSEditNewUserMsgClick(TObject *Sender);
	void __fastcall BBSEditFeedbackMsgClick(TObject *Sender);
	void __fastcall BBSEditIPFilterClick(TObject *Sender);
	void __fastcall BBSEditNameFilterClick(TObject *Sender);
	void __fastcall BBSEditFilenameFilterClick(TObject *Sender);
	void __fastcall PhoneNumberPHONECAN1Click(TObject *Sender);
	void __fastcall BBSEditNameFilterBadMsgClick(TObject *Sender);
	void __fastcall BBSEditFileFilterBadMsgClick(TObject *Sender);
	void __fastcall BBSEditPhoneFilterBadMsgClick(TObject *Sender);
	void __fastcall MailEditAliasListClick(TObject *Sender);
    void __fastcall UpTimerTick(TObject *Sender);
    void __fastcall BBSViewErrorLogMenuItemClick(TObject *Sender);
    void __fastcall ChatToggleExecute(TObject *Sender);
    void __fastcall ViewClientsExecute(TObject *Sender);
    void __fastcall UserEditorExecute(TObject *Sender);
    void __fastcall FtpEditAliasListClick(TObject *Sender);
    void __fastcall FtpEditLoginMessageClick(TObject *Sender);
    void __fastcall FtpEditHelloMessageClick(TObject *Sender);
    void __fastcall FtpEditGoodbyeMessageClick(TObject *Sender);
    void __fastcall FileOpenMenuItemClick(TObject *Sender);
private:	// User declarations
public:		// User declarations
    __fastcall TMainForm(TComponent* Owner);
    bool			SysAutoStart;
    bool            MailAutoStart;
    bool            FtpAutoStart;
    bool			MailLogFile;
    bool			FtpLogFile;
    AnsiString		CtrlDirectory;
    scfg_t		    cfg;
    bbs_startup_t 	bbs_startup;
    mail_startup_t 	mail_startup;
    ftp_startup_t	ftp_startup;
	TPageControl* __fastcall PageControl(int num);
	int __fastcall  PageNum(TPageControl* obj);
    void __fastcall SaveSettings(void);
};

//---------------------------------------------------------------------------
extern PACKAGE TMainForm *MainForm;
//---------------------------------------------------------------------------
#endif
