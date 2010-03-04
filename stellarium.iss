; Stellarium installer

[Setup]
DisableStartupPrompt=yes
WizardSmallImageFile=data\icon.bmp
WizardImageFile=data\splash.bmp
WizardImageStretch=no
WizardImageBackColor=clBlack
AppName=Stellarium
AppVerName=Stellarium 0.10.3
DefaultDirName={pf}\Stellarium
DefaultGroupName=Stellarium
UninstallDisplayIcon={app}\data\stellarium.ico
LicenseFile=COPYING
Compression=zip/9

[Files]
Source: "C:\Program Files\Stellarium\bin\stellarium.exe"; DestDir: "{app}"
Source: "C:\Program Files\Stellarium\lib\libstelMain.dll"; DestDir: "{app}"
Source: "README"; DestDir: "{app}"; Flags: isreadme; DestName: "README.rtf"
Source: "INSTALL"; DestDir: "{app}"; DestName: "INSTALL.rtf"
Source: "COPYING"; DestDir: "{app}"; DestName: "GPL.rtf"
Source: "AUTHORS"; DestDir: "{app}"; DestName: "AUTHORS.rtf"
Source: "ChangeLog"; DestDir: "{app}";
Source: "libgcc_s_dw2-1.dll"; DestDir: "{app}";
Source: "libiconv2.dll"; DestDir: "{app}";
Source: "libintl3.dll"; DestDir: "{app}";
Source: "zlib1.dll"; DestDir: "{app}";
Source: "mingwm10.dll"; DestDir: "{app}";
Source: "phonon4.dll"; DestDir: "{app}";
Source: "QtSql4.dll"; DestDir: "{app}";
Source: "QtSvg4.dll"; DestDir: "{app}";
Source: "QtCore4.dll"; DestDir: "{app}";
Source: "QtGui4.dll"; DestDir: "{app}";
Source: "QtOpenGL4.dll"; DestDir: "{app}";
Source: "QtNetwork4.dll"; DestDir: "{app}";
Source: "QtScript4.dll"; DestDir: "{app}";
Source: "QtXml4.dll"; DestDir: "{app}";
Source: "sqldrivers\qsqlite4.dll"; DestDir: "{app}\sqldrivers\";
Source: "C:\Program Files\Stellarium\share\stellarium\*"; DestDir: "{app}\"; Flags: recursesubdirs
; Locales
Source: "C:\Program Files\Stellarium\share\locale\*"; DestDir: "{app}\locale\"; Flags: recursesubdirs

[Tasks]
Name: desktopicon; Description: "Create a &desktop icon"; GroupDescription: "Additional icons:"
Name: desktopicon\common; Description: "For all users"; GroupDescription: "Additional icons:"; Flags: exclusive
Name: desktopicon\user; Description: "For the current user only"; GroupDescription: "Additional icons:"; Flags: exclusive unchecked

[UninstallDelete]

[Icons]
Name: "{group}\Stellarium"; Filename: "{app}\stellarium.exe"; WorkingDir: "{app}"; IconFilename: "{app}\data\stellarium.ico"
Name: "{group}\Uninstall Stellarium"; Filename: "{uninstallexe}"
Name: "{group}\config.ini"; Filename: "{userappdata}\Stellarium\config.ini"
Name: "{group}\Last run log"; Filename: "{userappdata}\Stellarium\log.txt"
Name: "{commondesktop}\Stellarium"; Filename: "{app}\stellarium.exe"; WorkingDir: "{app}"; IconFilename: "{app}\data\stellarium.ico"; Tasks: desktopicon\common
Name: "{userdesktop}\Stellarium"; Filename: "{app}\stellarium.exe"; WorkingDir: "{app}"; IconFilename: "{app}\data\stellarium.ico"; Tasks: desktopicon\user
