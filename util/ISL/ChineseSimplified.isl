; *** Inno Setup version 5.5.3+ Chinese (Simplified) messages ***
;   By Qiming Li (qiming at clault.com)
;
; To download user-contributed translations of this file, go to:
;   http://www.jrsoftware.org/files/istrans/
;
; Note: When translating this text, do not add periods (.) to the end of
; messages that didn't have them already, because on those messages Inno
; Setup adds the periods automatically (appending a period would result in
; two periods being displayed).

[LangOptions]
; The following three entries are very important. Be sure to read and 
; understand the '[LangOptions] section' topic in the help file.
LanguageName=<4E2D><6587><FF08><7B80><4F53><FF09>
LanguageID=$0804
LanguageCodePage=936
; If the language you are translating to requires special font faces or
; sizes, uncomment any of the following entries and change them accordingly.
DialogFontName=����
;DialogFontSize=8
;WelcomeFontName=Verdana
;WelcomeFontSize=12
;TitleFontName=Arial
;TitleFontSize=29
;CopyrightFontName=Arial
;CopyrightFontSize=8

[Messages]

; *** Application titles
SetupAppTitle=��װ��
SetupWindowTitle=��װ�� - %1
UninstallAppTitle=ж����
UninstallAppFullTitle=%1ж����

; *** Misc. common
InformationTitle=��Ϣ
ConfirmTitle=ȷ��
ErrorTitle=����

; *** SetupLdr messages
SetupLdrStartupMessage=��װ�򵼽������ĵ����ϰ�װ%1��ȷ��Ҫ������
LdrCannotCreateTemp=�޷�������ʱ�ļ�����װ����ֹ
LdrCannotExecTemp=�޷�������ʱ�ļ����е��ļ�����װ����ֹ

; *** Startup error messages
LastErrorMessage=%1.%n%n���� %2: %3
SetupFileMissing=��װĿ¼��ȱʧ�ļ�%1�����������⣬�����»�ȡһ�ݳ��򿽱���
SetupFileCorrupt=��װ�ļ��ѱ��𻵡������»�ȡһ�ݳ��򿽱���
SetupFileCorruptOrWrongVer=��װ�ļ��ѱ��𻵣����뱾��װ�򵼰汾�����ݡ����������⣬�����»�ȡһ�ݳ��򿽱���
InvalidParameter=��Ч�����в�����%n%n%1
SetupAlreadyRunning=��װ�����Ѿ����С�
WindowsVersionNotSupported=����֧�������������е�Windows�汾��
WindowsServicePackRequired=����Ҫ��%1 Service Pack %2����°汾��
NotOnThisPlatform=���򲻿���%1�����С�
OnlyOnThisPlatform=���������%1�����С�
OnlyOnTheseArchitectures=����ֻ����Ϊ���´������ܹ�����Ƶ�Windows�汾�ϰ�װ��%n%n%1
MissingWOW64APIs=����ʹ�õ�Windows�汾û�а�������64λ��װ����Ĺ��ܡ��밲װService Pack %1��������⡣
WinVersionTooLowError=����Ҫ��%2�汾�����ϵ�%1��
WinVersionTooHighError=���򲻿ɰ�װ��%2����߰汾��%1�ϡ�
AdminPrivilegesRequired=�������¼Ϊ����Ա���ܰ�װ�˳���
PowerUserPrivilegesRequired=�������¼Ϊ����Ա���Ȩ���û����ܰ�װ�˳���
SetupAppRunningError=��װ�򵼼�⵽%1�������С�%n%n��ر������д��ڲ������ȷ����������������ȡ�����˳���װ��
UninstallAppRunningError=ж���򵼼�⵽%1�������С�%n%n��ر������д��ڣ�Ȼ������ȷ����������������ȡ�����˳���

; *** Misc. errors
ErrorCreatingDir=��װ���޷������ļ��С�%1��
ErrorTooManyFilesInDir=�����ļ��С�%1�����ļ����࣬�޷������д����ļ�

; *** Setup common messages
ExitSetupTitle=�˳���װ��
ExitSetupMessage=��װ��δ��ɡ���������˳������򽫲��ᱻ��װ�� %n%n�������´������а�װ������ɳ���İ�װ��%n%nȷ���˳���װ����
AboutSetupMenuItem=���ڰ�װ��(&A)��
AboutSetupTitle=���ڰ�װ��
AboutSetupMessage=%1�汾%2%n%3%n%n%1��ҳ��%n%4
AboutSetupNote=
TranslatorNote=

; *** Buttons
ButtonBack=< ��һ��(&B)
ButtonNext=��һ��(&N) >
ButtonInstall=��װ(&I)
ButtonOK=ȷ��
ButtonCancel=ȡ��
ButtonYes=��(&Y)
ButtonYesToAll=ȫѡ��(&A)
ButtonNo=��(&N)
ButtonNoToAll=ȫѡ��(&O)
ButtonFinish=����(&F)
ButtonBrowse=���(&B)��
ButtonWizardBrowse=���(&R)��
ButtonNewFolder=�����ļ���(&M)

; *** "Select Language" dialog messages
SelectLanguageTitle=ѡ������
SelectLanguageLabel=ѡ��װʱʹ�����ԣ�

; *** Common wizard text
ClickNext=�������һ������������ȡ�����˳���װ�򵼡�
BeveledLabel=
BrowseDialogTitle=���ѡ���ļ���
BrowseDialogLabel=�������б���ѡȡһ���ļ��У��������ȷ������
NewFolderName=�½��ļ���

; *** "Welcome" wizard page
WelcomeLabel1=��ӭʹ��[name]��װ��
WelcomeLabel2=���򵼽������ĵ����ϰ�װ[name/ver]%n%n�������ڼ���֮ǰ�ر���������Ӧ�ó���

; *** "Password" wizard page
WizardPassword=����
PasswordLabel1=����װ���������뱣����
PasswordLabel3=���������룬���������һ�������������ִ�Сд��
PasswordEditLabel=����(&P)��
IncorrectPassword=����������벻��ȷ�������ԡ�

; *** "License Agreement" wizard page
WizardLicense=���Э��
LicenseLabel=���Ķ�������Ҫ��Ϣ��Ȼ���ٽ�����һ����
LicenseLabel3=���Ķ��������Э�顣��������ܴ�Э������Ȼ����ܼ�����װ��
LicenseAccepted=�ҽ���Э��(&A)
LicenseNotAccepted=�Ҳ�����Э��(&D)

; *** "Information" wizard pages
WizardInfoBefore=��Ϣ
InfoBeforeLabel=���Ķ�������Ҫ��Ϣ�ٽ�����һ����
InfoBeforeClickLabel=׼���ü�����װ�󣬵������һ������
WizardInfoAfter=��Ϣ
InfoAfterLabel=���Ķ�������Ҫ��Ϣ�ٽ�����һ����
InfoAfterClickLabel=׼���ü�����װ�󣬵������һ������

; *** "User Information" wizard page
WizardUserInfo=�û���Ϣ
UserInfoDesc=������������Ϣ
UserInfoName=�û�����(&U)��
UserInfoOrg=��������(&O)��
UserInfoSerial=���к���(&S)��
UserInfoNameRequired=���������û���

; *** "Select Destination Location" wizard page
WizardSelectDir=ѡ��װλ��
SelectDirDesc=��[name]��װ���δ���
SelectDirLabel3=��װ�򵼽���[name]��װ�������ļ����С�
SelectDirBrowseLabel=�������һ���������������Ҫѡ��ͬ���ļ��У��������������
DiskSpaceMBLabel=����������[mb]���ֽڣ�MB�������ô��̿ռ䡣
CannotInstallToNetworkDrive=�޷���װ��������������
CannotInstallToUNCPath=�޷���װ��UNC·����
InvalidPath=��������������̷�������·�������磺%n%nC:\Ӧ�ó���%n%n�����¸�ʽ��UNC·����%n%n\\��������\����Ŀ¼��
InvalidDrive=��ѡ�����������UNC�������ڻ򲻿ɷ��ʡ�����ѡһ����
DiskSpaceWarningTitle=���̿ռ䲻��
DiskSpaceWarning=����������%1ǧ�ֽڣ�KB�������ÿռ�ſɰ�װ������ѡ����������%2ǧ�ֽڣ�KB�����ÿռ䡣%n%n��ȷ��Ҫ������
DirNameTooLong=�ļ������ƻ�·��̫����
InvalidDirName=�ļ���������Ч��
BadDirName32=�ļ������Ʋ��ܰ��������ַ���%n%n%1
DirExistsTitle=�ļ����Ѵ���
DirExists=�ļ���%n%n%1%n%n�Ѵ��ڡ���ȷ��Ҫ��װ�����ļ�����
DirDoesntExistTitle=�ļ��в�����
DirDoesntExist=�ļ���%n%n%1%n%n�����ڡ���Ҫ�������ļ�����

; *** "Select Components" wizard page
WizardSelectComponents=ѡ�����
SelectComponentsDesc=Ҫ��װ��Щ�����
SelectComponentsLabel2=��ѡ��Ҫ��װ������������Ҫ��װ�������׼���ú�������һ������
FullInstallation=ȫ����װ
; if possible don't translate 'Compact' as 'Minimal' (I mean 'Minimal' in your language)
CompactInstallation=��లװ
CustomInstallation=�Զ��尲װ
NoUninstallWarningTitle=����Ѵ���
NoUninstallWarning=��װ�򵼼�⵽�Ѿ���װ���������%n%n%1%n%nȡ��ѡ������ж����Щ�����%n%n��ȷ��Ҫ������װ��
ComponentSize1=%1ǧ�ֽڣ�KB��
ComponentSize2=%1���ֽڣ�MB��
ComponentsDiskSpaceMBLabel=Ŀǰ��ѡ���Ҫ������[mb]���ֽڣ�MB�����̿ռ䡣

; *** "Select Additional Tasks" wizard page
WizardSelectTasks=ѡ�񸽼�����
SelectTasksDesc=Ҫִ����Щ��������
SelectTasksLabel2=��ѡ��װ[name]ʱ��Ҫִ�еĸ�������Ȼ��������һ������

; *** "Select Start Menu Folder" wizard page
WizardSelectProgramGroup=ѡ��ʼ�˵��ļ���
SelectStartMenuFolderDesc=�ѳ����ݷ�ʽ�ŵ����
SelectStartMenuFolderLabel3=��װ�򵼽������¿�ʼ�˵��ļ����д��������ݷ�ʽ��
SelectStartMenuFolderBrowseLabel=�������һ������������Ҫѡ����һ���ļ��У�������������
MustEnterGroupName=�����������ļ�������
GroupNameTooLong=�ļ������ƻ�·��̫����
InvalidGroupName=�ļ���������Ч��
BadGroupName=�ļ������Ʋ��ܰ��������ַ���%n%n%1
NoProgramGroupCheck2=��Ҫ������ʼ�˵��ļ���(&D)

; *** "Ready to Install" wizard page
WizardReady=��װ׼�����
ReadyLabel1=��װ����׼����ϣ�����ʼ�����ĵ����ϰ�װ[name]��
ReadyLabel2a=�������װ����ʼ��װ����Ҫȷ�ϻ����������������һ������
ReadyLabel2b=�������װ����ʼ��װ��
ReadyMemoUserInfo=�û���Ϣ��
ReadyMemoDir=��װλ�ã�
ReadyMemoType=��װ���ͣ�
ReadyMemoComponents=��ѡ�����
ReadyMemoGroup=��ʼ�˵��ļ��У�
ReadyMemoTasks=��������

; *** "Preparing to Install" wizard page
WizardPreparing=׼����װ
PreparingDesc=��װ������׼�������ĵ����ϰ�װ[name]��
PreviousInstallNotCompleted=�ϴγ���װ/ж��δ����ɡ�����Ҫ��������������ϴΰ�װ��%n%n��������֮�����������а�װ������װ[name]�� 
CannotContinue=��װ�޷�������������ȡ�����˳���
ApplicationsFound=��װ����Ҫ���µ��ļ�������Ӧ�ó���ռ�á���������װ���Զ��ر���ЩӦ�ó���
ApplicationsFound2=��װ����Ҫ���µ��ļ�������Ӧ�ó���ռ�á���������װ���Զ��ر���ЩӦ�ó��򡣰�װ��ɺ󣬰�װ�򵼽���������������ЩӦ�ó��� 
CloseApplications=�Զ��ر�Ӧ�ó���(&A)
DontCloseApplications=���Զ��ر�Ӧ�ó���(&D)
ErrorCloseApplications=��װ���޷��Զ��ر����е�Ӧ�ó����ڽ�����һ��֮ǰ���������ر���Щռ�ð�װ����Ҫ�����ļ���Ӧ�ó���

; *** "Installing" wizard page
WizardInstalling=���ڰ�װ
InstallingLabel=���Ժ򣬰�װ���������ĵ����ϰ�װ[name]��

; *** "Setup Completed" wizard page
FinishedHeadingLabel=[name]��װ���
FinishedLabelNoIcons=��װ���������ĵ����ϰ�װ[name]��
FinishedLabel=��װ���������ĵ����ϰ�װ[name]������ͨ���Ѱ�װ�Ŀ�ݷ�ʽ���򿪴�Ӧ�ó���
ClickFinish=������������˳���װ��
FinishedRestartLabel=Ϊ�����[name]�İ�װ����װ�򵼱����������ĵ��ԡ�Ҫ����������
FinishedRestartMessage=Ϊ�����[name]�İ�װ����װ�򵼱����������ĵ��ԡ�%n%nҪ����������
ShowReadmeCheck=�ǣ���Ҫ�Ķ������ļ�
YesRadio=�ǣ�������������(&Y)
NoRadio=���Ժ�������������(&N)
; used for example as 'Run MyProg.exe'
RunEntryExec=����%1
; used for example as 'View Readme.txt'
RunEntryShellExec=����%1

; *** "Setup Needs the Next Disk" stuff
ChangeDiskTitle=��װ����Ҫ��һ�Ŵ���
SelectDiskLabel2=��������%1 �������ȷ������%n%n����ô����е��ļ�������������ʾ�ļ����У���������ȷ��·���������������
PathLabel=·��(&P)��
FileNotInDir2=�ļ���%1�����ڡ�%2���С��������ȷ�Ĵ��̻�ѡ�������ļ��С�
SelectDirectoryLabel=��ָ����һ�Ŵ��̵�λ�á�

; *** Installation phase messages
SetupAborted=��װδ����ɡ�%n%n������������������а�װ�򵼡�
EntryAbortRetryIgnore=��������ԡ����³��ԣ���������ԡ�������װ����������ֹ��ȡ����װ��

; *** Installation status messages
StatusClosingApplications=���ڹر�Ӧ�ó���
StatusCreateDirs=���ڴ����ļ��С�
StatusExtractFiles=����ȡ���ļ���
StatusCreateIcons=���ڴ�����ݷ�ʽ��
StatusCreateIniEntries=���ڴ���INI��Ŀ��
StatusCreateRegistryEntries=���ڴ���ע�����Ŀ��
StatusRegisterFiles=���ڴ���ע�����Ŀ��
StatusSavingUninstall=���ڱ���ж����Ϣ��
StatusRunProgram=���ڽ�����װ��
StatusRestartingApplications=��������Ӧ�ó���
StatusRollback=���ڳ������ġ�

; *** Misc. errors
ErrorInternal2=�ڲ�����%1
ErrorFunctionFailedNoCode=%1ʧ��
ErrorFunctionFailed=%1ʧ�ܣ�������%2
ErrorFunctionFailedWithMessage=%1ʧ�ܣ�������%2��%n%3
ErrorExecutingProgram=�޷����г���%n%1

; *** Registry errors
ErrorRegOpenKey=��ע����ʱ����%n%1\%2
ErrorRegCreateKey=����ע����ʱ����%n%1\%2
ErrorRegWriteKey=д��ע����ʱ����%n%1\%2

; *** INI errors
ErrorIniEntry=���ļ���%1���д���INI��Ŀʱ����

; *** File copying errors
FileAbortRetryIgnore=��������ԡ����³��ԣ���������ԡ��������ļ������Ƽ�������������������ֹ��ȡ����װ��
FileAbortRetryIgnore2=��������ԡ����³��ԣ���������ԡ�������װ�����Ƽ�������������������ֹ��ȡ����װ��
SourceIsCorrupted=Դ�ļ�����
SourceDoesntExist=Դ�ļ���%1��������
ExistingFileReadOnly=�����ļ������Ϊֻ����%n%n��������ԡ��Ƴ���ֻ�����Բ����³��ԣ���������ԡ��������ļ�����������ֹ��ȡ����װ��
ErrorReadingExistingDest=��ȡ�����ļ�ʱ����
FileExists=�ļ��Ѵ��ڡ�%n%n�ð�װ�򵼸�������
ExistingFileNewer=�����ļ��Ȱ�װ����ͼ��װ�Ļ�Ҫ�¡����鱣�������ļ���%n%n��Ҫ���������ļ���
ErrorChangingAttr=���������ļ�����ʱ����
ErrorCreatingTemp=��Ŀ���ļ����д����ļ�ʱ����
ErrorReadingSource=��ȡԴ�ļ�ʱ����
ErrorCopying=�����ļ�ʱ����
ErrorReplacingExistingFile=�滻�����ļ�ʱ����
ErrorRestartReplace=�����滻ʧ�ܣ�
ErrorRenamingTemp=ΪĿ���ļ������ļ�������ʱ����
ErrorRegisterServer=�޷�ע�ᶯ̬���ؼ���DLL/OCX����%1
ErrorRegSvr32Failed=����RegSvr32ʧ�ܣ��䷵��ֵΪ��%1
ErrorRegisterTypeLib=�޷�ע�����Ϳ⣺%1

; *** Post-installation errors
ErrorOpeningReadme=�������ļ�ʱ����
ErrorRestartingComputer=��װ���޷��������ԡ����ֶ�������

; *** Uninstaller messages
UninstallNotFound=�ļ���%1�������ڡ��޷�ж�ء�
UninstallOpenError=�޷����ļ���%1�����޷�ж��
UninstallUnsupportedVer=�˰汾��ж�����޷�ʶ��ж����־�ļ���%1���ĸ�ʽ���޷�ж��
UninstallUnknownEntry=��ж����־������δ֪��Ŀ (%1)
ConfirmUninstall=���Ƿ�ȷ��Ҫ��ȫɾ��%1�������������
UninstallOnlyOnWin64=�˰�װֻ����64λWindows��ж�ء�
OnlyAdminCanUninstall=�˰�װֻ���ɾ߱�����ԱȨ�޵��û�ж�ء�
UninstallStatusLabel=���Ժ�����ɾ��%1��
UninstalledAll=�ѳɹ��ش����ĵ�����ɾ��%1��
UninstalledMost=%1ж����ϡ�%n%nĳЩ��Ŀ�޷���ж�ع�����ɾ���������ֶ�ɾ����Щ��Ŀ��
UninstalledAndNeedsRestart=��Ҫ���%1��ж�أ������������ԡ�%n%nҪ����������
UninstallDataCorrupted=�ļ���%1�����𻵡��޷�ж��

; *** Uninstallation phase messages
ConfirmDeleteSharedFileTitle=ɾ�������ļ���
ConfirmDeleteSharedFile2=ϵͳ��ʾû���κγ���ʹ�����¹����ļ���Ҫɾ���ù����ļ���%n%n����г���ʹ�ø��ļ���������ɾ������Щ��������޷��������С������ȷ������ѡ�񡰷񡱡����¸��ļ������ϵͳ����κ�Σ����
SharedFileNameLabel=�ļ�����
SharedFileLocationLabel=λ�ã�
WizardUninstalling=ж��״̬
StatusUninstalling=����ж��%1��

; *** Shutdown block reasons
ShutdownBlockReasonInstallingApp=���ڰ�װ%1��
ShutdownBlockReasonUninstallingApp=����ж��%1��

; The custom messages below aren't used by Setup itself, but if you make
; use of them in your scripts, you'll want to translate them.

[CustomMessages]

NameAndVersion=%1�汾%2
AdditionalIcons=���ӿ�ݷ�ʽ��
CreateDesktopIcon=���������ݷ�ʽ(&D)
CreateQuickLaunchIcon=����������������ݷ�ʽ(&Q)
ProgramOnTheWeb=%1��վ
UninstallProgram=ж��%1
LaunchProgram=����%1
AssocFileExtension=��%1��%2�ļ���չ������(&A)
AssocingFileExtension=���ڽ�%1��%2�ļ���չ��������
AutoStartProgramGroupDescription=������
AutoStartProgram=�Զ�����%1
AddonHostProgramNotFound=������ѡ�ļ������Ҳ���%1��%n%n�Ƿ���Ȼ������
