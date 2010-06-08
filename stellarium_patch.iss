; This creates a setup file for a patch that REPLACES some of the files in an
; existing Stellarium installation. (By default, only the executable files.)

; Unlike a regular installation file, the patch installer does not create
; entries in the Start menu or an uninstaller. (So installing files that don't
; exist in the original instalaltion means that they won't be removed
; when uninstalling Stellarium.)

[Setup]
DisableStartupPrompt=yes
WizardSmallImageFile=data\icon.bmp
WizardImageFile=data\splash.bmp
WizardImageStretch=no
WizardImageBackColor=clBlack
AppName=Stellarium
; AppId should have the same value as AppName/AppId in the original installer
AppId=Stellarium
; Unlike the regular, CMake-generated Inno Setup project file,
; you need to UPDATE THE VERSION NUMBER manually.
AppVerName=Stellarium 0.10.5
OutputBaseFilename=stellarium-0.10.5
OutputDir=installers
DefaultDirName={pf}\Stellarium
DefaultGroupName=Stellarium
UninstallDisplayIcon={app}\data\stellarium.ico
LicenseFile=COPYING
Compression=zip/9

;If uncommented, this file will be displayed before the installation begins
;(see http://www.jrsoftware.org/ishelp/index.php?topic=wizardpages)
;InfoBeforeFile=patch_notes.txt

;Detect and use Stellarium's install directory
UsePreviousAppDir=yes
CreateUninstallRegKey=no
UpdateUninstallLogAppName=no

[Files]
Source: "builds\msys\src\stellarium.exe"; DestDir: "{app}"
Source: "builds\msys\src\libstelMain.dll"; DestDir: "{app}"

;Some of these may need to be updated:
;Source: "README"; DestDir: "{app}"; Flags: isreadme; DestName: "README.rtf"
;Source: "INSTALL"; DestDir: "{app}"; DestName: "INSTALL.rtf"
;Source: "COPYING"; DestDir: "{app}"; DestName: "GPL.rtf"
;Source: "AUTHORS"; DestDir: "{app}"; DestName: "AUTHORS.rtf"
;Source: "ChangeLog"; DestDir: "{app}";

;Add further files to be updated here:

[Tasks]

[UninstallDelete]

[Icons]
