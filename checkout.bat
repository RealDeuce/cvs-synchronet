@echo off
echo Synchronet source code check-out for Win32
echo $Id: checkout.bat,v 1.2 2003/09/19 06:09:33 rswindell Exp $
setlocal
set HOME=c:\
set CVSROOT=:pserver:anonymous@cvs.synchro.net:/cvsroot/sbbs
cvs co src-sbbs3
cvs co lib-win32 
