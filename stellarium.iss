; Stellarium installer

[Setup]
DisableStartupPrompt=yes
WizardSmallImageFile=data\icon.bmp
WizardImageFile=data\splash.bmp
WizardImageStretch=no
WizardImageBackColor=clBlack
AppName=Stellarium
AppVerName=Stellarium 0.7.0
DefaultDirName={pf}\Stellarium
DefaultGroupName=Stellarium
UninstallDisplayIcon={app}\stellarium.exe
LicenseFile=COPYING
Compression=zip/9

[Files]
Source: "src\stellarium.exe"; DestDir: "{app}"
Source: "README"; DestDir: "{app}"; Flags: isreadme; DestName: "README.rtf"
Source: "INSTALL"; DestDir: "{app}"; DestName: "INSTALL.txt"
Source: "COPYING"; DestDir: "{app}"; DestName: "GPL.txt"
Source: "AUTHORS"; DestDir: "{app}"; DestName: "AUTHORS.txt"
Source: "ChangeLog"; DestDir: "{app}";
Source: "SDL.dll"; DestDir: "{app}";
Source: "libpng13.dll"; DestDir: "{app}";
Source: "zlib1.dll"; DestDir: "{app}";
Source: "data\*"; DestDir: "{app}\data"
Source: "data\sky_cultures\*"; DestDir: "{app}\data\sky_cultures"
Source: "data\sky_cultures\polynesian\*"; DestDir: "{app}\data\sky_cultures\polynesian\"
Source: "data\sky_cultures\western\*"; DestDir: "{app}\data\sky_cultures\western\"
Source: "data\sky_cultures\chinese\*"; DestDir: "{app}\data\sky_cultures\chinese\"
Source: "data\sky_cultures\egyptian\*"; DestDir: "{app}\data\sky_cultures\egyptian\"
Source: "config\*"; DestDir: "{app}\config"
Source: "textures\*"; DestDir: "{app}\textures"
Source: "textures\landscapes\*"; DestDir: "{app}\textures\landscapes"
Source: "textures\constellation-art\*"; DestDir: "{app}\textures\constellation-art"

[UninstallDelete]
Type: files; Name: "{app}\stdout.txt"
Type: files; Name: "{app}\stderr.txt"

[Icons]
Name: "{group}\Stellarium"; Filename: "{app}\stellarium.exe"; WorkingDir: "{app}"; IconFilename: "{app}\data\stellarium.ico"
Name: "{group}\Uninstall Stellarium"; Filename: "{uninstallexe}"
Name: "{group}\config.ini"; Filename: "{app}\config\config.ini"

