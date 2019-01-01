; Script generated by the Inno Setup Script Wizard.
; SEE THE DOCUMENTATION FOR DETAILS ON CREATING INNO SETUP SCRIPT FILES!

#define MyAppName "Synchronet BBS Software"
#define MyAppVersion "3.17b"
#define MyAppPublisher "Rob Swindell"
#define MyAppURL "http://www.synchro.net/"
#define MyAppSupportURL "http://wiki.synchro.net/howto:support"
#define MyAppDownloadURL "http://www.synchro.net/download.html"
#define MyAppExeName "sbbsctrl.exe"
#define sbbsCtrlPanel "Synchronet Control Panel"
#define release "release"
#define source "e:\src\sbbs317"

[Setup]
; NOTE: The value of AppId uniquely identifies this application.
; Do not use the same AppId value in installers for other applications.
; (To generate a new GUID, click Tools | Generate GUID inside the IDE.)
AppId={{2AC457EF-EDA4-4BDC-B6C0-D49ADB44ABF5}
AppName={#MyAppName}
AppVersion={#MyAppVersion}
AppVerName={#MyAppName} v{#MyAppVersion}
AppPublisher={#MyAppPublisher}
AppPublisherURL={#MyAppURL}
AppSupportURL={#MyAppSupportURL}
AppUpdatesURL={#MyAppDownloadURL}
AppCopyright=Copyright 2019 {#MyAppPublisher}
VersionInfoVersion=3.17.1
AppMutex=sbbs_running,sbbsctrl_running
DefaultDirName={sd}\sbbs
DefaultGroupName={#MyAppName}
DisableProgramGroupPage=yes
AllowNoIcons=yes
; OutputDir=.
OutputBaseFilename=setup
Compression=lzma
SolidCompression=yes
MinVersion = 5.1
InfoBeforeFile=e:\stock\docs\install.rtf
ChangesEnvironment=yes
SourceDir={#source}
UninstallDisplayIcon="{app}\exec\sbbsctrl.exe"

[Languages]
Name: "english"; MessagesFile: "compiler:Default.isl"

#define servers_group "TCP/IP Servers"
#define services_group "TCP and UDP Services"
#define ntsvcs_group "NT Services (for use by advanced system operators)"
#define webui_group "Web Interface"
[Tasks]
Name: "desktopicon"; Description: "{cm:CreateDesktopIcon}"; GroupDescription: "{cm:AdditionalIcons}"; Flags: unchecked
Name: "startupicon"; Description: "Create a shortcut in the Startup folder"; GroupDescription: "{cm:AdditionalIcons}";
Name: "enable_bbs"; Description: "Enable Terminal Server (Telnet, RLogin, and SSH protocols)"; GroupDescription: {#servers_group}
Name: "enable_mail"; Description: "Enable Mail Server (SMTP and POP3 protocols)"; GroupDescription: {#servers_group}
Name: "enable_ftp"; Description: "Enable FTP Server (FTP protocol)"; GroupDescription: {#servers_group}
Name: "enable_web"; Description: "Enable Web Server (HTTP protocol)"; GroupDescription: {#servers_group}
Name: "enable_svcs"; Description: "Enable Synchronet Services (Finger, Gopher, NNTP, IRC, IMAP, etc.)"; GroupDescription: {#services_group}
Name: "enable_ntsvcs"; Description: "Enable Synchronet NT services"; GroupDescription: {#ntsvcs_group}; Flags: unchecked
Name: "enable_sexpots"; Description: "Enable POTS (dial-up modem) support service"; GroupDescription: {#ntsvcs_group}; Flags: unchecked
name: "web_runemaster"; Description: "Enable Legacy/Runemaster Web Interface"; GroupDescription: {#webui_group}; Flags: exclusive
name: "web_echicken"; Description: "Enable echicken's Web Interface (v4)"; GroupDescription: {#webui_group}; Flags: exclusive unchecked

[Files]
Source: "src\sbbs3\ctrl\sbbsctrl.exe";                         DestDir: "{app}\exec";  Flags: ignoreversion
Source: "src\sbbs3\chat\chat.exe";                             DestDir: "{app}\exec";  Flags: ignoreversion
Source: "src\sbbs3\useredit\useredit.exe";                     DestDir: "{app}\exec";  Flags: ignoreversion
Source: "src\sbbs3\msvc.win32.exe.{#release}\*.exe";           DestDir: "{app}\exec";  Flags: ignoreversion; Excludes: "textgen.exe, v4upgrade.exe"
Source: "src\sbbs3\msvc.win32.dll.{#release}\*.dll";           DestDir: "{app}\exec";  Flags: ignoreversion
Source: "src\sbbs3\msvc.win32.dll.release\sbbsexec.dll";       DestDir: "{sys}";
Source: "src\sbbs3\scfg\msvc.win32.exe.{#release}\scfg.exe";   DestDir: "{app}\exec";  Flags: ignoreversion
Source: "3rdp\win32.release\nspr\bin\nspr4.dll";       	       DestDir: "{app}\exec";  Flags: ignoreversion
Source: "3rdp\win32.release\mozjs\bin\mozjs185-1.0.dll";       DestDir: "{app}\exec";  Flags: ignoreversion
Source: "3rdp\win32.release\cryptlib\bin\cl32.dll";            DestDir: "{app}\exec";  Flags: ignoreversion
Source: "src\sexpots\{#release}\sexpots.exe";                     DestDir: "{app}\exec";  Flags: ignoreversion
Source: "s:\sbbs\exec\user.com";                                DestDir: "{app}\exec";  Flags: ignoreversion; Check: not IsWin64
Source: "s:\sbbs\exec\mlabels.exe";                             DestDir: "{app}\exec";  Flags: ignoreversion; Check: not IsWin64
Source: "s:\sbbs\exec\svdmansi.com";                            DestDir: "{app}\exec";  Flags: ignoreversion; Check: not IsWin64
Source: "s:\sbbs\exec\dosxtrn.exe";                             DestDir: "{app}\exec";  Flags: ignoreversion
Source: "e:\stock\node1\node.cnf";                                DestDir: "{app}\node1"; Flags: ignoreversion
Source: "e:\stock\node1\node.cnf";                                DestDir: "{app}\node2"; Flags: ignoreversion
Source: "e:\stock\node1\node.cnf";                                DestDir: "{app}\node3"; Flags: ignoreversion
Source: "e:\stock\node1\node.cnf";                                DestDir: "{app}\node4"; Flags: ignoreversion
Source: "e:\stock\exec\*";   DestDir: "{app}\exec";  Flags: ignoreversion recursesubdirs createallsubdirs; Excludes: "CVS,.#*,baja.js,menu.js,sbbsedit.js,jsdocs.js,testbuild.js,load\menulib.js"
Source: "e:\stock\ctrl\*";   DestDir: "{app}\ctrl";  Flags: ignoreversion recursesubdirs createallsubdirs; Excludes: "CVS,.#*,*.?.cnf,*.?.ini"
Source: "e:\stock\text\*";   DestDir: "{app}\text";  Flags: ignoreversion recursesubdirs createallsubdirs; Excludes: "CVS,.#*"
Source: "e:\stock\xtrn\*";   DestDir: "{app}\xtrn";  Flags: ignoreversion recursesubdirs createallsubdirs; Excludes: "CVS,.#*,server.ini"
Source: "e:\stock\docs\*";   DestDir: "{app}\docs";  Flags: ignoreversion recursesubdirs createallsubdirs; Excludes: "CVS,.#*"
Source: "e:\stock\web\*";    DestDir: "{app}\web";   Flags: ignoreversion recursesubdirs createallsubdirs; Excludes: "CVS,.#*"
Source: "e:\stock\webv4\*";  DestDir: "{app}\webv4"; Flags: ignoreversion recursesubdirs createallsubdirs; Excludes: "CVS,.#*"
Source: "c:\bin\zip.exe"; DestDir: "{app}\exec";  Flags: ignoreversion
Source: "c:\bin\unzip.exe"; DestDir: "{app}\exec";  Flags: ignoreversion
; NOTE: Don't use "Flags: ignoreversion" on any shared system files

[Icons]
Name: "{group}\{#sbbsCtrlPanel}"; Filename: "{app}\exec\{#MyAppExeName}"
Name: "{group}\Synchronet Configuration"; Filename: "{app}\exec\scfg.exe"
Name: "{group}\Synchronet User Editor"; Filename: "{app}\exec\useredit.exe"; Parameters: "{app}\data\";
Name: "{group}\Synchronet Initialization File"; Filename: "{app}\ctrl\sbbs.ini"
Name: "{group}\Synchronet Documentation"; Filename: "{app}\docs"
Name: "{group}\Synchronet Web Site"; Filename: "{#MyAppURL}"
Name: "{group}\Synchronet Support"; Filename: "{#MyAppSupportURL}"
Name: "{group}\Donate to the Synchronet project"; Filename: "http://wiki.synchro.net/donate"
Name: "{group}\Get SyncTERM"; Filename: "http://sourceforge.net/projects/syncterm/"
Name: "{commonstartup}\{#sbbsCtrlPanel}"; Filename: "{app}\exec\{#MyAppExeName}"; Tasks: startupicon
Name: "{commondesktop}\{#sbbsCtrlPanel}"; Filename: "{app}\exec\{#MyAppExeName}"; Tasks: desktopicon

[Run]
Filename: "{app}\exec\{#MyAppExeName}"; Description: "{cm:LaunchProgram,{#StringChange(MyAppName, '&', '&&')}}"; Parameters: "{app}\ctrl"; Flags: nowait postinstall skipifsilent
Filename: "{app}\exec\sbbsNTsvcs.exe"; Parameters: "install"; StatusMsg: "Installing Synchronet NT services"; Tasks: enable_ntsvcs; Flags: runhidden skipifdoesntexist
Filename: "{app}\exec\sbbsNTsvcs.exe"; Parameters: "enable bbs"; StatusMsg: "Enabling Synchronet NT service: Terminal Server"; Tasks: enable_ntsvcs and enable_bbs; Flags: runhidden skipifdoesntexist
Filename: "{app}\exec\sbbsNTsvcs.exe"; Parameters: "enable ftp"; StatusMsg: "Enabling Synchronet NT service: FTP Server"; Tasks: enable_ntsvcs and enable_ftp; Flags: runhidden skipifdoesntexist
Filename: "{app}\exec\sbbsNTsvcs.exe"; Parameters: "enable web"; StatusMsg: "Enabling Synchronet NT service: Web Server"; Tasks: enable_ntsvcs and enable_web; Flags: runhidden skipifdoesntexist
Filename: "{app}\exec\sbbsNTsvcs.exe"; Parameters: "enable mail"; StatusMsg: "Enabling Synchronet NT service: Mail Server"; Tasks: enable_ntsvcs and enable_mail; Flags: runhidden skipifdoesntexist
Filename: "{app}\exec\sbbsNTsvcs.exe"; Parameters: "enable services"; StatusMsg: "Enabling Synchronet NT service: Services"; Tasks: enable_ntsvcs and enable_svcs; Flags: runhidden skipifdoesntexist
Filename: "{app}\exec\sexpots.exe"; Parameters: "-install"; StatusMsg: "Installing external POTS support service"; Tasks: enable_sexpots; Flags: runhidden skipifdoesntexist
Filename: "{sys}\sc.exe"; Parameters: "start sexpots"; StatusMsg: "Starting external POTS support service"; Tasks: enable_sexpots; Flags: runhidden skipifdoesntexist

[UninstallRun]
Filename: "{app}\exec\sbbsNTsvcs.exe"; Parameters: "remove"; StatusMsg: "Disabling Synchronet NT services"; Flags: runhidden skipifdoesntexist
Filename: "{app}\exec\sexpots.exe"; Parameters: "-remove"; StatusMsg: "Disabling external POTS support service"; Flags: runhidden skipifdoesntexist

[Registry]
Root: HKLM; Subkey: "System\CurrentControlSet\Control\Session Manager\Environment"; ValueType: string; ValueName: "SBBSCTRL"; ValueData: "{app}\ctrl"

[INI]
Filename: "{app}\ctrl\sbbs.ini"; Section: "Global";   Key: "CtrlDirectory"; String: "{app}\ctrl"
Filename: "{app}\ctrl\sbbs.ini"; Section: "BBS";      Key: "AutoStart"; String: "false"; Tasks: not enable_bbs
Filename: "{app}\ctrl\sbbs.ini"; Section: "Mail";     Key: "AutoStart"; String: "false"; Tasks: not enable_mail
Filename: "{app}\ctrl\sbbs.ini"; Section: "FTP";      Key: "AutoStart"; String: "false"; Tasks: not enable_ftp
Filename: "{app}\ctrl\sbbs.ini"; Section: "Web";      Key: "AutoStart"; String: "false"; Tasks: not enable_web
Filename: "{app}\ctrl\sbbs.ini"; Section: "Services"; Key: "AutoStart"; String: "false"; Tasks: not enable_svcs
Filename: "{app}\ctrl\sbbs.ini"; Section: "BBS";      Key: "Options"; String: "XTRN_MINIMIZED | ALLOW_RLOGIN | ALLOW_SSH | NO_DOS"; Check: IsWin64
Filename: "{app}\ctrl\sbbs.ini"; Section: "Web";      Key: "RootDirectory"; String: "../webv4/root"; Tasks: web_echicken

[Dirs]
Name: "{app}\data"
