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
Source: "builds\msys\src\stellarium.exe"; DestDir: "{app}"
Source: "builds\msys\src\libstelmain.dll"; DestDir: "{app}"
Source: "README"; DestDir: "{app}"; Flags: isreadme; DestName: "README.rtf"
Source: "INSTALL"; DestDir: "{app}"; DestName: "INSTALL.rtf"
Source: "COPYING"; DestDir: "{app}"; DestName: "GPL.rtf"
Source: "AUTHORS"; DestDir: "{app}"; DestName: "AUTHORS.rtf"
Source: "ChangeLog"; DestDir: "{app}";
Source: "libpng13.dll"; DestDir: "{app}";
Source: "mingwm10.dll"; DestDir: "{app}";
Source: "mingwm10.dll"; DestDir: "{app}";
Source: "QtCore4.dll"; DestDir: "{app}";
Source: "QtGui4.dll"; DestDir: "{app}";
Source: "QtOpenGL4.dll"; DestDir: "{app}";
Source: "QtNetwork4.dll"; DestDir: "{app}";
Source: "QtScript4.dll"; DestDir: "{app}";
Source: "builds\msys\_CPack_Packages\win32\TGZ\Stellarium-0.10.3-win32\share\stellarium\*"; DestDir: "{app}\"; Flags: recursesubdirs
; Locales
Source: "builds\msys\_CPack_Packages\win32\TGZ\Stellarium-0.10.3-win32\share\locale\*"; DestDir: "{app}\locale\"; Flags: recursesubdirs

[UninstallDelete]

[Icons]
Name: "{group}\Stellarium"; Filename: "{app}\stellarium.exe"; WorkingDir: "{app}"; IconFilename: "{app}\data\stellarium.ico"
Name: "{group}\Uninstall Stellarium"; Filename: "{uninstallexe}"
Name: "{group}\config.ini"; Filename: "{userappdata}\Stellarium\config.ini"
Name: "{group}\Last run log"; Filename: "{userappdata}\Stellarium\log.txt"

