; Stellarium patch installer
; Run "make install" first to generate the executable and translation files.
; The CMake-generated patch script is supposed to be edited manually after
; generation, but all changes will be OVERWRITTEN the next time CMake is run!

; This script creates a setup file for a patch that REPLACES some of the files
; in an existing Stellarium installation. (By default, only the executables.)

; Unlike a regular installation file, the patch installer does not create
; entries in the Start menu or an uninstaller. (So installing files that don't
; exist in the original instalaltion means that they won't be removed
; when uninstalling Stellarium.)

[Setup]
@ISS_ARCHITECTURE_SPECIFIC@
DisableStartupPrompt=yes
WizardSmallImageFile=data\icon.bmp
WizardImageFile=data\splash.bmp
WizardImageStretch=no
WizardImageBackColor=clBlack
AppName=Stellarium
; AppId should have the same value as AppName/AppId in the original installer
AppId=Stellarium
AppVersion=@PACKAGE_VERSION@
AppVerName=Stellarium @PACKAGE_VERSION@
AppPublisher=Stellarium team
AppPublisherURL=http://www.stellarium.org/
OutputBaseFilename=stellarium-patch-@PACKAGE_VERSION@-@ISS_PACKAGE_PLATFORM@
OutputDir=installers
; In 64-bit mode, {pf} is equivalent to {pf64},
; see http://www.jrsoftware.org/ishelp/index.php?topic=32vs64bitinstalls
DefaultDirName={pf}\Stellarium
DefaultGroupName=Stellarium
UninstallDisplayIcon={app}\data\stellarium.ico
LicenseFile=COPYING
Compression=lzma2/max

; If uncommented, this file will be displayed before the installation begins
; (see http://www.jrsoftware.org/ishelp/index.php?topic=wizardpages)
;InfoBeforeFile=patch-warning.txt

; Detect and use Stellarium's install directory
UsePreviousAppDir=yes
CreateUninstallRegKey=no
UpdateUninstallLogAppName=no


[Files]
Source: "@CMAKE_INSTALL_PREFIX@/bin/stellarium.exe"; DestDir: "{app}"; Flags: ignoreversion
Source: "@CMAKE_INSTALL_PREFIX@/lib/libstelMain.dll"; DestDir: "{app}"; Flags: ignoreversion

; Thanks to the clever and competent design of the keyboard shortcut system,
; if your executable or changes/introduces StelAction names, you'll need this:
;Source: "@CMAKE_INSTALL_PREFIX@\share\stellarium\data\default_shortcuts.json"; DestDir: "{app}\data"

; Possibly the same file as the InfoBeforeFile; also uncomment in [Icons] below
; Note that if you uncomment the README line below, you need to decide which one
; will have the "isreadme" flag.
;Source: "release-notes.txt"; DestDir: "{app}"; Flags: isreadme; DestName: "Release Notes (@PACKAGE_VERSION@).txt"

; Some of these may need to be updated:
;Source: "README"; DestDir: "{app}"; Flags: isreadme; DestName: "README.rtf"
;Source: "INSTALL"; DestDir: "{app}"; DestName: "INSTALL.rtf"
;Source: "COPYING"; DestDir: "{app}"; DestName: "GPL.rtf"
;Source: "AUTHORS"; DestDir: "{app}"; DestName: "AUTHORS.rtf"
;Source: "ChangeLog"; DestDir: "{app}"; DestName: "ChangeLog.rtf"

; Add further files to be updated here:

; System libraries skipped - when building a patch, it's recommended to use
; the ones used in the last/appropriate release.

; Uncomment to update data and/or localization
; If only a few are changed, add them individually instead of packing all.
;Source: "@CMAKE_INSTALL_PREFIX@\share\stellarium\*"; DestDir: "{app}\"; Flags: recursesubdirs
;Source: "@CMAKE_INSTALL_PREFIX@\share\locale\*"; DestDir: "{app}\locale\"; Flags: recursesubdirs

[Tasks]
; It may be necessary to clean up the old configuration,
; so some of these can be checked by default.
; Don't forget to uncomment the actions themselves in [InstallDelete] below!

;Name: removeconfig; Description: "{cm:RemoveMainConfig}"; GroupDescription: "{cm:RemoveFromPreviousInstallation}"; Flags: unchecked
;Name: removeplugins; Description: "{cm:RemovePluginsConfig}"; GroupDescription: "{cm:RemoveFromPreviousInstallation}"; Flags: unchecked
;Name: removesolar; Description: "{cm:RemoveSolarConfig}"; GroupDescription: "{cm:RemoveFromPreviousInstallation}"; Flags: unchecked
;Name: removelandscapes; Description: "{cm:RemoveUILandscapes}"; GroupDescription: "{cm:RemoveFromPreviousInstallation}"; Flags: unchecked
;Name: removeshortcuts; Description: "{cm:RemoveShortcutsConfig}"; GroupDescription: "{cm:RemoveFromPreviousInstallation}"; Flags: unchecked

[Run]
; An option to start Stellarium after setup has finished
Filename: "{app}\stellarium.exe"; Description: "{cm:LaunchProgram,Stellarium}"; Flags: postinstall nowait skipifsilent unchecked

[InstallDelete]
;The old log file in all cases
Type: files; Name: "{userappdata}\Stellarium\log.txt"
;Type: files; Name: "{userappdata}\Stellarium\config.ini"; Tasks: removeconfig
;Type: files; Name: "{userappdata}\Stellarium\data\ssystem.ini"; Tasks: removesolar
;Type: filesandordirs; Name: "{userappdata}\Stellarium\modules"; Tasks: removeplugins
;Type: filesandordirs; Name: "{userappdata}\Stellarium\landscapes"; Tasks: removelandscapes
;Type: files; Name: "{userappdata}\Stellarium\data\shortcuts.json"; Tasks: removeshortcuts


[UninstallDelete]


[Icons]
; The default ones are already installed.

; Uncomment if you have specified a release notes file in [Files] above
;Name: "{group}\Release Notes (@PACKAGE_VERSION@)"; Filename: "{app}\Release Notes (@PACKAGE_VERSION@).txt"

; Recommended use Inno Setup 5.5.3+
[Languages]
; Official translations of GUI of Inno Setup + translation Stellarium specific lines
Name: "en"; MessagesFile: "compiler:Default.isl,util\ISL\EnglishCM.isl"
Name: "ca"; MessagesFile: "compiler:Languages\Catalan.isl,util\ISL\CatalanCM.isl"
Name: "co"; MessagesFile: "compiler:Languages\Corsican.isl"
Name: "cs"; MessagesFile: "compiler:Languages\Czech.isl"
Name: "da"; MessagesFile: "compiler:Languages\Danish.isl"
Name: "nl"; MessagesFile: "compiler:Languages\Dutch.isl"
Name: "fi"; MessagesFile: "compiler:Languages\Finnish.isl"
Name: "fr"; MessagesFile: "compiler:Languages\French.isl,util\ISL\FrenchCM.isl"
Name: "de"; MessagesFile: "compiler:Languages\German.isl"
Name: "el"; MessagesFile: "compiler:Languages\Greek.isl"
Name: "he"; MessagesFile: "compiler:Languages\Hebrew.isl"
Name: "hu"; MessagesFile: "compiler:Languages\Hungarian.isl"
Name: "it"; MessagesFile: "compiler:Languages\Italian.isl"
Name: "ja"; MessagesFile: "compiler:Languages\Japanese.isl"
Name: "no"; MessagesFile: "compiler:Languages\Norwegian.isl,util\ISL\NorwegianCM.isl"
Name: "pl"; MessagesFile: "compiler:Languages\Polish.isl"
Name: "pt_BR"; MessagesFile: "compiler:Languages\BrazilianPortuguese.isl,util\ISL\BrazilianPortugueseCM.isl"
Name: "pt"; MessagesFile: "compiler:Languages\Portuguese.isl"
Name: "ru"; MessagesFile: "compiler:Languages\Russian.isl,util\ISL\RussianCM.isl"
Name: "sr"; MessagesFile: "compiler:Languages\SerbianCyrillic.isl"
Name: "sl"; MessagesFile: "compiler:Languages\Slovenian.isl"
Name: "es"; MessagesFile: "compiler:Languages\Spanish.isl"
Name: "uk"; MessagesFile: "compiler:Languages\Ukrainian.isl,util\ISL\UkrainianCM.isl"
; Unofficial translations of GUI of Inno Setup
Name: "bg"; MessagesFile: "util\ISL\Bulgarian.isl,util\ISL\BulgarianCM.isl"
Name: "bs"; MessagesFile: "util\ISL\Bosnian.isl,util\ISL\BosnianCM.isl"
Name: "gla"; MessagesFile: "util\ISL\ScotsGaelic.isl"

