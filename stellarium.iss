; Stellarium installer

[Setup]
DisableStartupPrompt=yes
WizardSmallImageFile=data\icon.bmp
WizardImageFile=data\icon.bmp
AppName=Stellarium
AppVerName=Stellarium v0.6.1
DefaultDirName={pf}\Stellarium
DefaultGroupName=Stellarium
UninstallDisplayIcon={app}\stellarium.exe
LicenseFile=COPYING
Compression=zip/9

[Files]
Source: "src\stellarium.exe"; DestDir: "{app}"
Source: "README"; DestDir: "{app}"; Flags: isreadme; DestName: "README.txt"
Source: "TODO"; DestDir: "{app}"; DestName: "TODO.txt"
Source: "INSTALL"; DestDir: "{app}"; DestName: "INSTALL.txt"
Source: "COPYING"; DestDir: "{app}"; DestName: "GPL.txt"
Source: "AUTHORS"; DestDir: "{app}"; DestName: "AUTHORS.txt"
Source: "ChangeLog"; DestDir: "{app}";
Source: "SDL.dll"; DestDir: "{app}";
Source: "data\*"; DestDir: "{app}\data"
Source: "data\sky_cultures\*"; DestDir: "{app}\data\sky_cultures"
Source: "data\sky_cultures\polynesian\*"; DestDir: "{app}\data\sky_cultures\polynesian\"
Source: "data\sky_cultures\western\*"; DestDir: "{app}\data\sky_cultures\western\"
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

