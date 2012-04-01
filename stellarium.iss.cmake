; Stellarium installer
; Run "make install" first to generate binary and translation files.
; @CONFIGURED_FILE_WARNING@

[Setup]
DisableStartupPrompt=yes
WizardSmallImageFile=data\icon.bmp
WizardImageFile=data\splash.bmp
WizardImageStretch=no
WizardImageBackColor=clBlack
AppName=Stellarium
AppVerName=Stellarium @PACKAGE_VERSION@
OutputBaseFilename=stellarium-@PACKAGE_VERSION@-win32
OutputDir=installers
DefaultDirName={pf}\Stellarium
DefaultGroupName=Stellarium
UninstallDisplayIcon={app}\data\stellarium.ico
LicenseFile=COPYING
Compression=zip/9

[Files]
Source: "@CMAKE_INSTALL_PREFIX@\bin\stellarium.exe"; DestDir: "{app}"
Source: "@CMAKE_INSTALL_PREFIX@\lib\libstelMain.dll"; DestDir: "{app}"
Source: "README"; DestDir: "{app}"; Flags: isreadme; DestName: "README.rtf"
Source: "INSTALL"; DestDir: "{app}"; DestName: "INSTALL.rtf"
Source: "COPYING"; DestDir: "{app}"; DestName: "GPL.rtf"
Source: "AUTHORS"; DestDir: "{app}"; DestName: "AUTHORS.rtf"
Source: "ChangeLog"; DestDir: "{app}"; DestName: "ChangeLog.rtf"
Source: "@QT_BINARY_DIR@/../../mingw/bin/libgcc_s_dw2-1.dll"; DestDir: "{app}";
Source: "@QT_BINARY_DIR@/../../mingw/bin/mingwm10.dll"; DestDir: "{app}";
Source: "@ICONV_INCLUDE_DIR@/../bin/libiconv2.dll"; DestDir: "{app}";
Source: "@INTL_INCLUDE_DIR@/../bin/libintl3.dll"; DestDir: "{app}";
Source: "@ZLIB_INCLUDE_DIR@/../bin/zlib1.dll"; DestDir: "{app}";
Source: "@QT_BINARY_DIR@\phonon4.dll"; DestDir: "{app}";
Source: "@QT_BINARY_DIR@\QtSql4.dll"; DestDir: "{app}";
Source: "@QT_BINARY_DIR@\QtSvg4.dll"; DestDir: "{app}";
Source: "@QT_BINARY_DIR@\QtCore4.dll"; DestDir: "{app}";
Source: "@QT_BINARY_DIR@\QtGui4.dll"; DestDir: "{app}";
Source: "@QT_BINARY_DIR@\QtOpenGL4.dll"; DestDir: "{app}";
Source: "@QT_BINARY_DIR@\QtNetwork4.dll"; DestDir: "{app}";
Source: "@QT_BINARY_DIR@\QtScript4.dll"; DestDir: "{app}";
Source: "@QT_BINARY_DIR@\QtXml4.dll"; DestDir: "{app}";
;Source: "@QT_PLUGINS_DIR@\sqldrivers\qsqlite4.dll"; DestDir: "{app}\sqldrivers\";
Source: "@CMAKE_INSTALL_PREFIX@\share\stellarium\*"; DestDir: "{app}\"; Flags: recursesubdirs
; Locales
Source: "@CMAKE_INSTALL_PREFIX@\share\locale\*"; DestDir: "{app}\locale\"; Flags: recursesubdirs

[Tasks]
Name: desktopicon; Description: "Create a &desktop icon"; GroupDescription: "Additional icons:"
Name: desktopicon\common; Description: "For all users"; GroupDescription: "Additional icons:"; Flags: exclusive
Name: desktopicon\user; Description: "For the current user only"; GroupDescription: "Additional icons:"; Flags: exclusive unchecked
Name: removeconfig; Description: "Remove the main configuration file"; GroupDescription: "Remove files left over from a previous installation:"; Flags: unchecked
Name: removeplugins; Description: "Remove plug-ins' configuration files"; GroupDescription: "Remove files left over from a previous installation:"; Flags: unchecked
Name: removesolar; Description: "Remove Solar System file"; GroupDescription: "Remove files left over from a previous installation:"; Flags: unchecked
Name: removelandscapes; Description: "Remove user-installed landscapes"; GroupDescription: "Remove files left over from a previous installation:"; Flags: unchecked

[Run]
;An option to start Stellarium after setup has finished
Filename: "{app}\stellarium.exe"; Description: "Start Stellarium"; Flags: postinstall nowait skipifsilent unchecked

[InstallDelete]
;The old log file in all cases
Type: files; Name: "{userappdata}\Stellarium\log.txt"
Type: files; Name: "{userappdata}\Stellarium\config.ini"; Tasks: removeconfig
Type: files; Name: "{userappdata}\Stellarium\data\ssystem.ini"; Tasks: removesolar
Type: filesandordirs; Name: "{userappdata}\Stellarium\modules"; Tasks: removeplugins
Type: filesandordirs; Name: "{userappdata}\Stellarium\landscapes"; Tasks: removelandscapes

[UninstallDelete]

[Icons]
Name: "{group}\Stellarium"; Filename: "{app}\stellarium.exe"; WorkingDir: "{app}"; IconFilename: "{app}\data\stellarium.ico"
Name: "{group}\Stellarium (no OpenGL2)"; Filename: "{app}\stellarium.exe"; Parameters: "--safe-mode"; WorkingDir: "{app}"; IconFilename: "{app}\data\stellarium.ico"
Name: "{group}\Uninstall Stellarium"; Filename: "{uninstallexe}"
Name: "{group}\config.ini"; Filename: "{userappdata}\Stellarium\config.ini"
Name: "{group}\Last run log"; Filename: "{userappdata}\Stellarium\log.txt"
Name: "{group}\Change log"; Filename: "{app}\ChangeLog.rtf"
Name: "{commondesktop}\Stellarium"; Filename: "{app}\stellarium.exe"; WorkingDir: "{app}"; IconFilename: "{app}\data\stellarium.ico"; Tasks: desktopicon\common
Name: "{userdesktop}\Stellarium"; Filename: "{app}\stellarium.exe"; WorkingDir: "{app}"; IconFilename: "{app}\data\stellarium.ico"; Tasks: desktopicon\user
