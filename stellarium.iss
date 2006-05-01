; Stellarium installer

[Setup]
DisableStartupPrompt=yes
WizardSmallImageFile=data\icon.bmp
WizardImageFile=data\splash.bmp
WizardImageStretch=no
WizardImageBackColor=clBlack
AppName=Stellarium
AppVerName=Stellarium 0.8.0
DefaultDirName={pf}\Stellarium
DefaultGroupName=Stellarium
UninstallDisplayIcon={app}\stellarium.exe
LicenseFile=COPYING
Compression=zip/9

[Files]
Source: "src\stellarium.exe"; DestDir: "{app}"
Source: "README"; DestDir: "{app}"; Flags: isreadme; DestName: "README.rtf"
Source: "INSTALL"; DestDir: "{app}"; DestName: "INSTALL.rtf"
Source: "COPYING"; DestDir: "{app}"; DestName: "GPL.rtf"
Source: "AUTHORS"; DestDir: "{app}"; DestName: "AUTHORS.rtf"
Source: "ChangeLog"; DestDir: "{app}";
Source: "SDL.dll"; DestDir: "{app}";
Source: "libpng13.dll"; DestDir: "{app}";
Source: "zlib1.dll"; DestDir: "{app}";
Source: "mingwm10.dll"; DestDir: "{app}";
Source: "libstlport.5.0.dll"; DestDir: "{app}";
Source: "libfreetype-6.dll"; DestDir: "{app}";
Source: "data\*"; DestDir: "{app}\data"
Source: "data\sky_cultures\*"; DestDir: "{app}\data\sky_cultures"
Source: "data\sky_cultures\polynesian\*"; DestDir: "{app}\data\sky_cultures\polynesian\"
Source: "data\sky_cultures\western\*"; DestDir: "{app}\data\sky_cultures\western\"
Source: "data\sky_cultures\chinese\*"; DestDir: "{app}\data\sky_cultures\chinese\"
Source: "data\sky_cultures\egyptian\*"; DestDir: "{app}\data\sky_cultures\egyptian\"
Source: "data\scripts\*"; DestDir: "{app}\data\scripts\"
; Locales
Source: "data\locale\cs\LC_MESSAGES\*"; DestDir: "{app}\data\locale\cs\LC_MESSAGES\"
Source: "data\locale\de\LC_MESSAGES\*"; DestDir: "{app}\data\locale\de\LC_MESSAGES\"
Source: "data\locale\en_GB\LC_MESSAGES\*"; DestDir: "{app}\data\locale\en_GB\LC_MESSAGES\"
Source: "data\locale\fa\LC_MESSAGES\*"; DestDir: "{app}\data\locale\fa\LC_MESSAGES\"
Source: "data\locale\fr\LC_MESSAGES\*"; DestDir: "{app}\data\locale\fr\LC_MESSAGES\"
Source: "data\locale\it\LC_MESSAGES\*"; DestDir: "{app}\data\locale\it\LC_MESSAGES\"
Source: "data\locale\ko\LC_MESSAGES\*"; DestDir: "{app}\data\locale\ko\LC_MESSAGES\"
Source: "data\locale\nb\LC_MESSAGES\*"; DestDir: "{app}\data\locale\nb\LC_MESSAGES\"
Source: "data\locale\pl\LC_MESSAGES\*"; DestDir: "{app}\data\locale\pl\LC_MESSAGES\"
Source: "data\locale\ro\LC_MESSAGES\*"; DestDir: "{app}\data\locale\ro\LC_MESSAGES\"
Source: "data\locale\tr\LC_MESSAGES\*"; DestDir: "{app}\data\locale\tr\LC_MESSAGES\"
Source: "data\locale\zh_HK\LC_MESSAGES\*"; DestDir: "{app}\data\locale\zh_HK\LC_MESSAGES\"
Source: "data\locale\da\LC_MESSAGES\*"; DestDir: "{app}\data\locale\da\LC_MESSAGES\"
Source: "data\locale\en\LC_MESSAGES\*"; DestDir: "{app}\data\locale\en\LC_MESSAGES\"
Source: "data\locale\es\LC_MESSAGES\*"; DestDir: "{app}\data\locale\es\LC_MESSAGES\"
Source: "data\locale\fi\LC_MESSAGES\*"; DestDir: "{app}\data\locale\fi\LC_MESSAGES\"
Source: "data\locale\hu\LC_MESSAGES\*"; DestDir: "{app}\data\locale\hu\LC_MESSAGES\"
Source: "data\locale\ja\LC_MESSAGES\*"; DestDir: "{app}\data\locale\ja\LC_MESSAGES\"
Source: "data\locale\lv\LC_MESSAGES\*"; DestDir: "{app}\data\locale\lv\LC_MESSAGES\"
Source: "data\locale\nl\LC_MESSAGES\*"; DestDir: "{app}\data\locale\nl\LC_MESSAGES\"
Source: "data\locale\pt_BR\LC_MESSAGES\*"; DestDir: "{app}\data\locale\pt_BR\LC_MESSAGES\"
Source: "data\locale\ru\LC_MESSAGES\*"; DestDir: "{app}\data\locale\ru\LC_MESSAGES\"
Source: "data\locale\zh_CN\LC_MESSAGES\*"; DestDir: "{app}\data\locale\zh_CN\LC_MESSAGES\"
Source: "data\locale\zh_TW\LC_MESSAGES\*"; DestDir: "{app}\data\locale\zh_TW\LC_MESSAGES\"

Source: "config\config.ini"; DestDir: "{app}\config\"
Source: "textures\*"; DestDir: "{app}\textures"
Source: "textures\landscapes\*"; DestDir: "{app}\textures\landscapes"
Source: "textures\constellation-art\*"; DestDir: "{app}\textures\constellation-art"

[UninstallDelete]
Type: files; Name: "{app}\config\config.ini"
Type: files; Name: "{app}\stdout.txt"
Type: files; Name: "{app}\stderr.txt"

[Icons]
Name: "{group}\Stellarium"; Filename: "{app}\stellarium.exe"; WorkingDir: "{app}"; IconFilename: "{app}\data\stellarium.ico"
Name: "{group}\Uninstall Stellarium"; Filename: "{uninstallexe}"
Name: "{group}\config.ini"; Filename: "{app}\config\config.ini"

