/* Synchronet Control Panel (GUI Borland C++ Builder Project for Win32) */

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

//----------------------------------------------------------------------------
#ifndef MailCfgDlgUnitH
#define MailCfgDlgUnitH
//----------------------------------------------------------------------------
#include <vcl\ExtCtrls.hpp>
#include <vcl\Buttons.hpp>
#include <vcl\StdCtrls.hpp>
#include <vcl\Controls.hpp>
#include <vcl\Forms.hpp>
#include <vcl\Graphics.hpp>
#include <vcl\Classes.hpp>
#include <vcl\SysUtils.hpp>
#include <vcl\Windows.hpp>
#include <vcl\System.hpp>
#include <Dialogs.hpp>
#include <ComCtrls.hpp>
//----------------------------------------------------------------------------
class TMailCfgDlg : public TForm
{
__published:
	TButton *OKBtn;
	TButton *CancelBtn;
    TOpenDialog *OpenDialog;
	TButton *ApplyButton;
    TPageControl *PageControl;
    TTabSheet *GeneralTabSheet;
    TCheckBox *AutoStartCheckBox;
    TLabel *InterfaceLabel;
    TEdit *NetworkInterfaceEdit;
    TLabel *MaxClientsLabel;
    TEdit *MaxClientsEdit;
    TLabel *MaxInactivityLabel;
    TEdit *MaxInactivityEdit;
    TTabSheet *SMTPTabSheet;
    TLabel *TelnetPortLabel;
    TEdit *SMTPPortEdit;
    TCheckBox *UserNumberCheckBox;
    TCheckBox *HostnameCheckBox;
    TCheckBox *DebugTXCheckBox;
    TCheckBox *LogFileCheckBox;
    TTabSheet *POP3TabSheet;
    TLabel *POP3PortLabel;
    TEdit *POP3PortEdit;
    TCheckBox *POP3LogCheckBox;
    TCheckBox *POP3EnabledCheckBox;
    TTabSheet *SendMailTabSheet;
    TRadioButton *DNSRadioButton;
    TEdit *DNSServerEdit;
    TCheckBox *TcpDnsCheckBox;
    TRadioButton *RelayRadioButton;
    TEdit *RelayServerEdit;
    TEdit *RelayPortEdit;
    TLabel *RelayPortLabel;
    TLabel *DeliveryAttemptsLabel;
    TEdit *DeliveryAttemptsEdit;
    TLabel *RescanFreqLabel;
    TEdit *RescanFreqEdit;
    TCheckBox *DebugHeadersCheckBox;
    TTabSheet *SoundTabSheet;
    TLabel *SMTPSoundLabel;
    TEdit *InboundSoundEdit;
    TButton *InboundSoundButton;
    TLabel *POP3SoundLabel;
    TEdit *POP3SoundEdit;
    TButton *POP3SoundButton;
    TLabel *OutboundSoundLabel;
    TEdit *OutboundSoundEdit;
    TButton *OutboundSoundButton;
    TCheckBox *SendMailCheckBox;
    TLabel *DefaultUserLabel;
    TEdit *DefaultUserEdit;
	TCheckBox *AllowRelayCheckBox;
	TTabSheet *DNSBLTabSheet;
	TButton *DNSBLServersButton;
	TLabel *Label1;
	TGroupBox *DNSBLGroupBox;
	TRadioButton *BLRefuseRadioButton;
	TRadioButton *BLIgnoreRadioButton;
	TRadioButton *BLBadUserRadioButton;
	TRadioButton *BLTagRadioButton;
	TEdit *BLSubjectEdit;
	TEdit *BLHeaderEdit;
	TLabel *BLSubjectLabel;
	TLabel *BLHeaderLabel;
	TEdit *LinesPerYieldEdit;
	TLabel *LinesPerYieldLabel;
	TLabel *MaxRecipientsLabel;
	TEdit *MaxRecipientsEdit;
	TButton *DNSBLExemptionsButton;
    void __fastcall InboundSoundButtonClick(TObject *Sender);
    void __fastcall OutboundSoundButtonClick(TObject *Sender);
    void __fastcall FormShow(TObject *Sender);
    void __fastcall OKBtnClick(TObject *Sender);
	void __fastcall POP3SoundButtonClick(TObject *Sender);
	void __fastcall DNSRadioButtonClick(TObject *Sender);
	void __fastcall POP3EnabledCheckBoxClick(TObject *Sender);
    void __fastcall SendMailCheckBoxClick(TObject *Sender);
	void __fastcall DNSBLRadioButtonClick(TObject *Sender);
	void __fastcall DNSBLServersButtonClick(TObject *Sender);
	void __fastcall DNSBLExemptionsButtonClick(TObject *Sender);
private:
public:
	virtual __fastcall TMailCfgDlg(TComponent* AOwner);
};
//----------------------------------------------------------------------------
extern PACKAGE TMailCfgDlg *MailCfgDlg;
//----------------------------------------------------------------------------
#endif    
