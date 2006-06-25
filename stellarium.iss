; Stellarium installer

[Setup]
DisableStartupPrompt=yes
WizardSmallImageFile=data\icon.bmp
WizardImageFile=data\splash.bmp
WizardImageStretch=no
WizardImageBackColor=clBlack
AppName=Stellarium
AppVerName=Stellarium 0.8.1
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
Source: "po\az.gmo"; DestDir: "{app}\data\locale\az\LC_MESSAGES\";  DestName: "stellarium.mo"
Source: "po\ca.gmo"; DestDir: "{app}\data\locale\ca\LC_MESSAGES\";  DestName: "stellarium.mo"
Source: "po\cs.gmo"; DestDir: "{app}\data\locale\cs\LC_MESSAGES\";  DestName: "stellarium.mo"
Source: "po\da.gmo"; DestDir: "{app}\data\locale\da\LC_MESSAGES\";  DestName: "stellarium.mo"
Source: "po\de.gmo"; DestDir: "{app}\data\locale\de\LC_MESSAGES\";  DestName: "stellarium.mo"
Source: "po\el.gmo"; DestDir: "{app}\data\locale\el\LC_MESSAGES\";  DestName: "stellarium.mo"
Source: "po\en.gmo"; DestDir: "{app}\data\locale\en\LC_MESSAGES\";  DestName: "stellarium.mo"
Source: "po\en_GB.gmo"; DestDir: "{app}\data\locale\en_GB\LC_MESSAGES\";  DestName: "stellarium.mo"
Source: "po\es.gmo"; DestDir: "{app}\data\locale\es\LC_MESSAGES\";  DestName: "stellarium.mo"
Source: "po\fa.gmo"; DestDir: "{app}\data\locale\fa\LC_MESSAGES\";  DestName: "stellarium.mo"
Source: "po\fi.gmo"; DestDir: "{app}\data\locale\fi\LC_MESSAGES\";  DestName: "stellarium.mo"
Source: "po\fr.gmo"; DestDir: "{app}\data\locale\fr\LC_MESSAGES\";  DestName: "stellarium.mo"
Source: "po\ga.gmo"; DestDir: "{app}\data\locale\ga\LC_MESSAGES\";  DestName: "stellarium.mo"
Source: "po\hu.gmo"; DestDir: "{app}\data\locale\hu\LC_MESSAGES\";  DestName: "stellarium.mo"
Source: "po\id.gmo"; DestDir: "{app}\data\locale\id\LC_MESSAGES\";  DestName: "stellarium.mo"
Source: "po\it.gmo"; DestDir: "{app}\data\locale\it\LC_MESSAGES\";  DestName: "stellarium.mo"
Source: "po\ja.gmo"; DestDir: "{app}\data\locale\ja\LC_MESSAGES\";  DestName: "stellarium.mo"
Source: "po\ko.gmo"; DestDir: "{app}\data\locale\ko\LC_MESSAGES\";  DestName: "stellarium.mo"
Source: "po\lv.gmo"; DestDir: "{app}\data\locale\lv\LC_MESSAGES\";  DestName: "stellarium.mo"
Source: "po\nb.gmo"; DestDir: "{app}\data\locale\nb\LC_MESSAGES\";  DestName: "stellarium.mo"
Source: "po\nl.gmo"; DestDir: "{app}\data\locale\nl\LC_MESSAGES\";  DestName: "stellarium.mo"
Source: "po\nn.gmo"; DestDir: "{app}\data\locale\nn\LC_MESSAGES\";  DestName: "stellarium.mo"
Source: "po\pl.gmo"; DestDir: "{app}\data\locale\pl\LC_MESSAGES\";  DestName: "stellarium.mo"
Source: "po\pt.gmo"; DestDir: "{app}\data\locale\pt\LC_MESSAGES\";  DestName: "stellarium.mo"
Source: "po\pt_BR.gmo"; DestDir: "{app}\data\locale\pt_BR\LC_MESSAGES\";  DestName: "stellarium.mo"
Source: "po\ro.gmo"; DestDir: "{app}\data\locale\ro\LC_MESSAGES\";  DestName: "stellarium.mo"
Source: "po\ru.gmo"; DestDir: "{app}\data\locale\ru\LC_MESSAGES\";  DestName: "stellarium.mo"
Source: "po\sk.gmo"; DestDir: "{app}\data\locale\sk\LC_MESSAGES\";  DestName: "stellarium.mo"
Source: "po\sv.gmo"; DestDir: "{app}\data\locale\sv\LC_MESSAGES\";  DestName: "stellarium.mo"
Source: "po\tr.gmo"; DestDir: "{app}\data\locale\tr\LC_MESSAGES\";  DestName: "stellarium.mo"
Source: "po\uk.gmo"; DestDir: "{app}\data\locale\uk\LC_MESSAGES\";  DestName: "stellarium.mo"
Source: "po\zh_CN.gmo"; DestDir: "{app}\data\locale\zh_CN\LC_MESSAGES\";  DestName: "stellarium.mo"
Source: "po\zh_HK.gmo"; DestDir: "{app}\data\locale\zh_HK\LC_MESSAGES\";  DestName: "stellarium.mo"
Source: "po\zh_TW.gmo"; DestDir: "{app}\data\locale\zh_TW\LC_MESSAGES\";  DestName: "stellarium.mo"


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

