@echo off
echo Synchronet source code check-out for Win32
echo $Id: checkout.bat,v 1.3 2003/12/06 22:52:28 rswindell Exp $
setlocal
set HOME=c:\
set CVSROOT=:pserver:anonymous@cvs.synchro.net:/cvsroot/sbbs
cvs co src-sbbs3
cvs co lib-win32%1
