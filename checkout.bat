@echo off
echo Synchronet source code check-out for Win32
echo $Id: checkout.bat,v 1.4 2004/11/19 23:01:51 rswindell Exp $
setlocal
set HOME=c:\
set CVSROOT=:pserver:anonymous@cvs.synchro.net:/cvsroot/sbbs
cvs co src-sbbs3
cvs co lib-win32.debug
cvs co lib-win32.release
