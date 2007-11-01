; Stellarium installer

[Setup]
DisableStartupPrompt=yes
WizardSmallImageFile=data\icon.bmp
WizardImageFile=data\splash.bmp
WizardImageStretch=no
WizardImageBackColor=clBlack
AppName=Stellarium
AppVerName=Stellarium 0.9.1
DefaultDirName={pf}\Stellarium
DefaultGroupName=Stellarium
UninstallDisplayIcon={app}\stellarium.exe
LicenseFile=COPYING
Compression=zip/9

[Files]
Source: "builds\msys\_CPack_Packages\win32\TGZ\Stellarium-0.9.1-win32\bin\stellarium.exe"; DestDir: "{app}"
Source: "builds\msys\_CPack_Packages\win32\TGZ\Stellarium-0.9.1-win32\lib\libstelmain.dll"; DestDir: "{app}"
Source: "README"; DestDir: "{app}"; Flags: isreadme; DestName: "README.rtf"
Source: "INSTALL"; DestDir: "{app}"; DestName: "INSTALL.rtf"
Source: "COPYING"; DestDir: "{app}"; DestName: "GPL.rtf"
Source: "AUTHORS"; DestDir: "{app}"; DestName: "AUTHORS.rtf"
Source: "ChangeLog"; DestDir: "{app}";
Source: "SDL.dll"; DestDir: "{app}";
Source: "SDL_mixer.dll"; DestDir: "{app}";
Source: "libpng13.dll"; DestDir: "{app}";
Source: "zlib1.dll"; DestDir: "{app}";
Source: "mingwm10.dll"; DestDir: "{app}";
Source: "freetype6.dll"; DestDir: "{app}";
Source: "jpeg62.dll"; DestDir: "{app}";
Source: "intl.dll"; DestDir: "{app}";
Source: "iconv.dll"; DestDir: "{app}";
Source: "QtCore4.dll"; DestDir: "{app}";
Source: "QtGui4.dll"; DestDir: "{app}";
Source: "QtOpenGL4.dll"; DestDir: "{app}";
Source: "libeay32.dll"; DestDir: "{app}";
Source: "libssl32.dll"; DestDir: "{app}";
Source: "libcurl-4.dll"; DestDir: "{app}";
Source: "builds\msys\_CPack_Packages\win32\TGZ\Stellarium-0.9.1-win32\share\stellarium\*"; DestDir: "{app}\"; Flags: recursesubdirs
; Locales
Source: "builds\msys\_CPack_Packages\win32\TGZ\Stellarium-0.9.1-win32\share\locale\*"; DestDir: "{app}\locale\"; Flags: recursesubdirs

[UninstallDelete]

[Icons]
Name: "{group}\Stellarium"; Filename: "{app}\stellarium.exe"; WorkingDir: "{app}"; IconFilename: "{app}\data\stellarium.ico"
Name: "{group}\Uninstall Stellarium"; Filename: "{uninstallexe}"
Name: "{group}\config.ini"; Filename: "{localappdata}\..\..\Stellarium\config.ini"

