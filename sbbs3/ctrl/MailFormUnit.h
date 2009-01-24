/* Synchronet Control Panel (GUI Borland C++ Builder Project for Win32) */

/* $Id: MailFormUnit.h,v 1.4 2009/01/24 12:23:15 rswindell Exp $ */

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

//---------------------------------------------------------------------------
#ifndef MailFormUnitH
#define MailFormUnitH
//---------------------------------------------------------------------------
#include <Classes.hpp>
#include <Controls.hpp>
#include <StdCtrls.hpp>
#include <Forms.hpp>
/* My Includes */
#include "MainFormUnit.h"
#include <ComCtrls.hpp>
#include <ToolWin.hpp>

//---------------------------------------------------------------------------
class TMailForm : public TForm
{
__published:	// IDE-managed Components
	TToolBar *ToolBar;
	TToolButton *StartButton;
	TToolButton *StopButton;
	TToolButton *ToolButton4;
	TToolButton *ConfigureButton;
	TToolButton *ToolButton6;
	TStaticText *Status;
	TToolButton *ToolButton1;
	TProgressBar *ProgressBar;
	TToolButton *RecycleButton;
    TRichEdit *Log;
    void __fastcall FormHide(TObject *Sender);
private:	// User declarations
public:		// User declarations
	__fastcall TMailForm(TComponent* Owner);
};
//---------------------------------------------------------------------------
extern PACKAGE TMailForm *MailForm;
//---------------------------------------------------------------------------
#endif
