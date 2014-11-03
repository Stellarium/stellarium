; *** Inno Setup version 5.5.3+ Scottish Gaelic messages ***
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
LanguageName=Gàidhlig
LanguageID=$1084
LanguageCodePage=1252
; If the language you are translating to requires special font faces or
; sizes, uncomment any of the following entries and change them accordingly.
;DialogFontName=
;DialogFontSize=8
;WelcomeFontName=Verdana
;WelcomeFontSize=12
;TitleFontName=Arial
;TitleFontSize=29
;CopyrightFontName=Arial
;CopyrightFontSize=8

[Messages]

; *** Application titles
SetupAppTitle=Stàladh
SetupWindowTitle=Stàladh - %1
UninstallAppTitle=Dì-stàladh
UninstallAppFullTitle=A' dì-stàladh %1

; *** Misc. common
InformationTitle=Fiosrachadh
ConfirmTitle=Dearbhaich
ErrorTitle=Mearachd

; *** SetupLdr messages
SetupLdrStartupMessage=Thèid %1 a stàladh a-nis. A bheil thu airson leantainn air adhart?
LdrCannotCreateTemp=Cha b' urrainn dhuinn faidhle sealach a chruthachadh. Chaidh sgur dhen stàladh
LdrCannotExecTemp=Cha b' urrainn dhuinn am faidhle a ruith sa phasgan shealach. Chaidh sgur dhen stàladh

; *** Startup error messages
LastErrorMessage=%1.%n%nMearachd %2: %3
SetupFileMissing=Tha am faidhle %1 a dhìth sa phasgan stàlaidh. Feuch an càraich thu an duilgheadas seo no faigh lethbhreac ùr dhen phrògram.
SetupFileCorrupt=Tha na faidhlichean stàlaidh coirbte. Feuch am faigh thu lethbhreac ùr dhen phrògram.
SetupFileCorruptOrWrongVer=Tha na faidhlichean stàlaidh coirbte no neo-chòrdail ris an tionndadh seo aig an stàladh. Feuch an càraich thu an duilgheadas seo no faigh lethbhreac ùr dhen phrògram.
InvalidParameter=Chaidh paramadair mì-dhligheach a shìneadh air an loidhne-àithne:%n%n%1
SetupAlreadyRunning=Tha an stàladh ga ruith mu thràth.
WindowsVersionNotSupported=Cha chuir am prògram seo taic ris an tionndadh aig Windows a tha an coimpiutair agad a' ruith.
WindowsServicePackRequired=Tha %1 pacaid seirbheise %2 no tionndadh nas ùire a dhìth air a' phrògram seo.
NotOnThisPlatform=Chan urrainn dhut am prògram seo a ruith fo %1.
OnlyOnThisPlatform=Feumaidh tu am prògram seo a ruith fo %1.
OnlyOnTheseArchitectures=Chan urrainn dhut am prògram seo a ruith ach air tionndaidhean Windows a chuireas taic ri ailtearachdan nam pròiseasar seo:%n%n%1
MissingWOW64APIs=Chan eil na foincseanan a tha a dhìth airson stàladh 64-biod a dhèanamh aig an tionndadh aig Windows a tha thu a' ruith. Feuch an stàlaich thu a' phacaid sheirbheise %1 gus an duilgheadas seo a chàradh.
WinVersionTooLowError=Tha %1 tionndadh %2 no nas ùire a dhìth airson a' phrògraim seo.
WinVersionTooHighError=Cha ghabh am prògram seo a stàladh fo %1 tionndadh %2 no nas ùire.
AdminPrivilegesRequired=Feumaidh tu logadh a-steach mar rianaire gus am prògram seo a stàladh.
PowerUserPrivilegesRequired=Feumaidh tu logadh a-steach mar rianaire no mar bhall dhen bhuidheann Power Users gus am prògram seo a stàladh.
SetupAppRunningError=Mhothaich an stàladh gu bheil %1 ga ruith an-dràsta.%n%nDùin gach ionstans a tha a' ruith an-dràsta is briog air "Ceart ma-thà" air neo briog air "Sguir dheth" gus fàgail.
UninstallAppRunningError=Mhothaich an dì-stàladh gu bheil %1 ga ruith an-dràsta.%n%nDùin gach ionstans a tha a' ruith an-dràsta is briog air "Ceart ma-thà" air neo briog air "Sguir dheth" gus fàgail.

; *** Misc. errors
ErrorCreatingDir=Cha b' urrainn dhan stàladh am pasgan "%1" a chruthachadh
ErrorTooManyFilesInDir=Tha faidhle ann nach b' urrainn dhan stàladh cruthachadh sa phasgan "%1" on a tha cus fhaidhlichean ann

; *** Setup common messages
ExitSetupTitle=Fàg an stàladh
ExitSetupMessage=Chan eil an stàladh coileanta fhathast. Ma sguireas tu dheth an-dràsta, cha tèid am prògram a stàladh.%n%n'S urrainn dhut an stàladh a dhèanamh a-rithist àm eile gus a choileanadh.%n%nA bheil thu airson an stàladh fhàgail?
AboutSetupMenuItem=&Mun stàladh ...
AboutSetupTitle=Mun stàladh
AboutSetupMessage=%1 Tionndadh %2%n%3%n%n%1 Duilleag-lìn:%n%4
AboutSetupNote=
TranslatorNote=An t-eadar-theangachadh le GunChleoc (fios@foramnagaidhlig.net)

; *** Buttons
ButtonBack=< Air ai&s
ButtonNext=Air adha&rt >
ButtonInstall=&Stàlaich
ButtonOK=Ceart ma-thà
ButtonCancel=Sguir dheth
ButtonYes=&Tha
ButtonYesToAll=Th&a dhan a h-uile
ButtonNo=&Chan eil
ButtonNoToAll=Cha&n eil dhan a h-uile
ButtonFinish=&Crìochnaich
ButtonBrowse=Rùrai&ch ...
ButtonWizardBrowse=&Rùraich ...
ButtonNewFolder=&Dèan pasgan ùr

; *** "Select Language" dialog messages
SelectLanguageTitle=Tagh cànan an stàlaidh
SelectLanguageLabel=Tagh an cànan a chleachdas tu leis an stàladh

; *** Common wizard text
ClickNext=Briog air "Air adhart" gus leantainn air adhart no air "Sguir dheth" gus fàgail.
BeveledLabel=
BrowseDialogTitle=Lorg pasgan
BrowseDialogLabel=Tagh pasgan is briog air "Ceart ma-thà" an uairsin.
NewFolderName=Pasgan ùr

; *** "Welcome" wizard page
WelcomeLabel1=Fàilte dhan draoidh stàlaidh aig [name]
WelcomeLabel2=Stàlaichidh an draoidh seo [name/ver] air a' choimpiutair agad a-nis.%n%nBu chòir dhut crìoch a chur air a h-uile aplacaid eile mus lean thu air adhart leis an stàladh.

; *** "Password" wizard page
WizardPassword=Facal-faire
PasswordLabel1=Tha an stàladh seo dìonta le facal-faire.
PasswordLabel3=Cuir a-steach am facal-faire is briog air "Air adhart" an uairsin. Thoir an aire air litrichean mòra is beaga.
PasswordEditLabel=&Facal-faire:
IncorrectPassword=Chan eil am facal-faire a chuir thu ann mar bu chòir. Am feuch thu ris a-rithist?

; *** "License Agreement" wizard page
WizardLicense=Aonta ceadachais
LicenseLabel=An leugh thu am fiosrachadh cudromach seo mus lean thu air adhart?
LicenseLabel3=Feuch an leugh thu an t-aonta ceadachais seo. Feumaidh tu gabhail ri teirmichean an aonta mus fhaod thu leantainn air adhart.
LicenseAccepted=&Gabhaidh mi ris an aonta
LicenseNotAccepted=&Diùltaidh mi an t-aonta

; *** "Information" wizard pages
WizardInfoBefore=Fiosrachadh
InfoBeforeLabel=An leugh thu am fiosrachadh cudromach seo mus lean thu air adhart?
InfoBeforeClickLabel=Nuair a bhios tu deiseil gus leantainn air adhart, briog air "Air adhart".
WizardInfoAfter=Fiosrachadh
InfoAfterLabel=An leugh thu am fiosrachadh cudromach seo mus lean thu air adhart?
InfoAfterClickLabel=Nuair a bhios tu deiseil gus leantainn air adhart, briog air "Air adhart".

; *** "User Information" wizard page
WizardUserInfo=Fiosrachadh a' chleachdaiche
UserInfoDesc=An cuir thu a-steach an dàta agad?
UserInfoName=&Ainm:
UserInfoOrg=&Buidheann:
UserInfoSerial=Àireamh &shreathach:
UserInfoNameRequired=Feumaidh tu ainm a chur a-steach.

; *** "Select Destination Location" wizard page
WizardSelectDir=Tagh am pasgan-uidhe
SelectDirDesc=Càite an tèid [name] a stàladh?
SelectDirLabel3=Thèid [name] a stàladh sa phasgan seo.
SelectDirBrowseLabel=Briog air "Air adhart" gus leantainn air adhart. Briog air "Rùraich" ma tha thu airson pasgan eile a thaghadh.
DiskSpaceMBLabel=Bidh feum air co-dhiù [mb] MB de rum sàbhalaidh saor.
CannotInstallToNetworkDrive=Cha ghabh stàladh air draibh lìonraidh.
CannotInstallToUNCPath=Cha ghabh stàladh air slighe UNC.
InvalidPath=Feumaidh tu slighe iomlan le litir draibh a thoirt seachad; m.e.:%n%nC:\Ball-eisimpleir%n%nno slighe UNC leis a' chruth:%n%n\\Frithealaiche\Co-roinneadh
InvalidDrive=Chan eil an draibh no an t-slighe UNC a thug thu seachad ann no chan urrainn dhuinn inntrigeadh. Feuch an tagh thu pasgan eile.
DiskSpaceWarningTitle=Chan eil rum saor gu leòr ann
DiskSpaceWarning=Cha co-dhiù %1 KB de rum saor a dhìth airson stàladh, ach chan eil ach %2 KB ri làimh air an draibh a thagh thu.%n%nA bheil thu airson leantainn air adhart co-dhiù?
DirNameTooLong=Tha ainm a' phasgain/na slighe ro fhada.
InvalidDirName=Chan eil ainm a' phasgain dligheach.
BadDirName32=Chan fhaod na caractaran seo a bhith ann an ainm pasgain:%n%n%1
DirExistsTitle=Tha am pasgan ann mu thràth
DirExists=Tha am pasgan:%n%n%1%n%nann mu thràth. A bheil thu airson stàladh sa phasgan seo co-dhiù?
DirDoesntExistTitle=Chan eil am pasgan ann
DirDoesntExist=Chan eil am pasgan:%n%n%1%n%nann. A bheil thu airson a chruthachadh?

; *** "Select Components" wizard page
WizardSelectComponents=Tagh co-phàirtean
SelectComponentsDesc=Dè na co-phàirtean a thèid a stàladh?
SelectComponentsLabel2=Tagh na co-phàirtean a tha thu airson stàladh. Briog air "Air adhart" nuair a bhios tu ullamh.
FullInstallation=Stàladh slàn
; if possible don't translate 'Compact' as 'Minimal' (I mean 'Minimal' in your language)
CompactInstallation=Stàladh beag
CustomInstallation=Stàladh gnàthaichte
NoUninstallWarningTitle=Tha co-phàirtean ann
NoUninstallWarning=Mhothaich an stàladh gun deach na co-phàirtean seo air stàladh air a' choimpiutair agad roimhe:%n%n%1%n%nCha tèid na co-phàirtean seo nach do thagh thu tuilleadh a thoirt air falbh on choimpiutair agad.%n%nA bheil thu airson leantainn air adhart co-dhiù?
ComponentSize1=%1 KB
ComponentSize2=%1 MB
ComponentsDiskSpaceMBLabel=Thèid co-dhiù [mb] MB de rum a chleachdadh airson na thagh thu.

; *** "Select Additional Tasks" wizard page
WizardSelectTasks=Saothraichean a bharrachd
SelectTasksDesc=Dè na saothraichean a bharrachd a thèid a ruith?
SelectTasksLabel2=Tagh na saothraichean a bharrachd a tha thu airson ruith leis an stàladh aig [name] is briog air "Air adhart" an uairsin.

; *** "Select Start Menu Folder" wizard page
WizardSelectProgramGroup=Tagh pasgan sa chlàr-taice tòiseachaidh
SelectStartMenuFolderDesc=Càite an cruthaich an stàladh na ceanglaichean dhan phrògram?
SelectStartMenuFolderLabel3=Cruthaichidh an stàladh na ceanglaichean dhan phrògram sa phasgan seo sa chlàr-taice tòiseachaidh.
SelectStartMenuFolderBrowseLabel=Briog air "Air adhart" gus leantainn air adhart. Briog air "Rùraich" ma tha thu airson pasgan eile a thaghadh.
MustEnterGroupName=Feumaidh tu ainm pasgain a chur a-steach.
GroupNameTooLong=Tha ainm a' phasgain/na slighe ro fhada.
InvalidGroupName=Chan eil ainm a' phasgain dligheach.
BadGroupName=Chan fhaod na caractaran seo a bhith ann an ainm pasgain:%n%n%1
NoProgramGroupCheck2=&Na cruthaich pasgan sam bith sa chlàr-taice tòiseachaidh

; *** "Ready to Install" wizard page
WizardReady=Deiseil airson an stàlaidh
ReadyLabel1=Tha an draoidh stàlaidh deiseil gus [name] a stàladh air a' choimpiutair agad.
ReadyLabel2a=Briog air "Stàlaich" gus tòiseachadh leis an stàladh no air "Air ais" gus sùil a thoirt air na roghainnean no gus an atharrachadh.
ReadyLabel2b=Briog air "Stàlaich" gus tòiseachadh air an stàladh.
ReadyMemoUserInfo=Fiosrachadh a' chleachdaiche:
ReadyMemoDir=Pasgan-uidhe:
ReadyMemoType=Seòrsa an stàlaidh:
ReadyMemoComponents=Co-phàirtean air an taghadh:
ReadyMemoGroup=Pasgan sa chlàr-taice tòiseachaidh:
ReadyMemoTasks=Saothraichean a bharrachd:

; *** "Preparing to Install" wizard page
WizardPreparing=Ag ullachadh an stàladh
PreparingDesc=Tha an stàladh aig [name] air a' choimpiutair seo ga ullachadh.
PreviousInstallNotCompleted=Tha stàladh/dì-stàladh aig prògram roimhe nach deach a choileanadh. Feumaidh tu an coimpiutair ath-thòiseachadh gus crìoch a chur air an stàladh/dì-stàladh.%n%nAn dèidh dhut an coimpiutair agad ath-thòiseachadh, tòisich an stàladh a-rithist gus an stàladh aig [name] a dhèanamh.
CannotContinue=Chan urrainn dhan stàladh leantainn air adhart. Feuch am briog thu air "Sguir dheth" gus fàgail.
ApplicationsFound=Tha na h-aplacaidean seo a' cleachdadh faidhlichean a dh'fheumas an stàladh ùrachadh. Mholamaid gun ceadaich thu gun dùin an stàladh na h-aplacaidean sin gu fèin-obrachail.
ApplicationsFound2=Tha na h-aplacaidean seo a' cleachdadh faidhlichean a dh'fheumas an stàladh ùrachadh. Mholamaid gun ceadaich thu gun dùin an stàladh na h-aplacaidean sin gu fèin-obrachail. Nuair a bhios an stàladh coileanta, feuchaidh sinn ris na h-aplacaidean sin ath-thòiseachadh.
CloseApplications=&Dùin na h-aplacaidean gu fèin-obrachail
DontCloseApplications=&Na dùin na h-aplacaidean
ErrorCloseApplications=Cha deach leis an stàladh a h-uile aplacaid a dhùnadh gu fèin-obrachail. Mus lean thu air adhart, mholamaid gun dùin thu a h-uile aplacaid a chleachdas faidhlichean a dh'fheumas an stàladh ùrachadh.

; *** "Installing" wizard page
WizardInstalling=Ga stàladh
InstallingLabel=Fuirich ort fhad 's a tha [name] ga stàladh air a' choimpiutair agad.

; *** "Setup Completed" wizard page
FinishedHeadingLabel=A' crìochnachadh an draoidh stàlaidh aig [name]
FinishedLabelNoIcons=Tha an stàladh aig [name] air a' choimpiutair agad coileanta.
FinishedLabel=Chaidh an stàladh aig [name] a chrìochnachadh air a' choimpiutair agad. 'S urrainn dhut am prògram a thòiseachadh a-nis leis na ceanglaichean dhan phrògram a chaidh a stàladh.
ClickFinish=Briog air "Crìochnaich" gus crìoch a chur air an stàladh.
FinishedRestartLabel=Feumaidh sinn an coimpiutair ath-thòiseachadh gus an stàladh aig [name] a choileanadh. A bheil thu airson seo a dhèanamh an-dràsta?
FinishedRestartMessage=Feumaidh sinn an coimpiutair ath-thòiseachadh gus an stàladh aig [name] a choileanadh.%n%nA bheil thu airson ath-thòiseachadh an-dràsta?
ShowReadmeCheck=Tha, tha mi airson am faidhle LEUGHMI fhaicinn
YesRadio=&Tha, ath-thòisich an coimpiutair an-dràsta
NoRadio=&Chan eil, ath-thòisichidh mi fhìn an coimpiutair àm eile
; used for example as 'Run MyProg.exe'
RunEntryExec=Tòisich %1
; used for example as 'View Readme.txt'
RunEntryShellExec=Seall %1

; *** "Setup Needs the Next Disk" stuff
ChangeDiskTitle=Tha an t-ath-chlàr a dhìth aig an stàladh
SelectDiskLabel2=Cuir a-steach clàr %1 is briog air "Ceart ma-thà".%n%nMur eil na faidhlichean on chlàr-shubailte seo sa phasgan a tha ga shealltainn dhut, cuir a-steach an t-slighe cheart no briog air "Rùraich".
PathLabel=&Slighe:
FileNotInDir2=Chan eil am faidhle "%1" an-seo: "%2". Feuch an atharraich thu am pasgan no an cuir thu a-steach clàr-subailte eile.
SelectDirectoryLabel=Sònraich càite an tèid an t-ath-chlàr a chur a-steach.

; *** Installation phase messages
SetupAborted=Cha b' urrainn dhuinn an stàladh a choileanadh.%n%nFeuch an càraich thu an duilgheadas is tòisich air an stàladh a-rithist.
EntryAbortRetryIgnore=Briog air "Ath-dhèan" gus feuchainn ris a-rithist, air "Leig seachad" gus leantainn ort co-dhiù no air "Sguir dheth" gus sgur dhen stàladh.

; *** Installation status messages
StatusClosingApplications=A' dùnadh aplacaidean ...
StatusCreateDirs=A' cruthachadh pasganan ...
StatusExtractFiles=A' dì-dhùmhlachadh faidhlichean ...
StatusCreateIcons=A' cruthachadh ceanglaichean ...
StatusCreateIniEntries=A' cruthachadh innteartan INI ...
StatusCreateRegistryEntries=A' cruthachadh innteartan a' chlàr-lainn ...
StatusRegisterFiles=A' clàradh faidhlichean ...
StatusSavingUninstall=A' sàbhaladh fiosrachadh dì-stàlaidh ...
StatusRunProgram=A' crìochnachadh an stàladh ...
StatusRestartingApplications=Ag ath-thòiseachadh aplacaidean ...
StatusRollback=A' neo-dhèanamh atharraichean ...

; *** Misc. errors
ErrorInternal2=Mearachd taobh a-staigh: %1
ErrorFunctionFailedNoCode=Dh'fhàillig le %1
ErrorFunctionFailed=Dh'fhàillig le %1; còd %2
ErrorFunctionFailedWithMessage=Dh'fhàillig le %1; còd %2.%n%3
ErrorExecutingProgram=Cha ghabh am faidhle a ruith:%n%1

; *** Registry errors
ErrorRegOpenKey=Cha b' urrainn dhuinn iuchair a' chlàr-lainn fhosgladh:%n%1\%2
ErrorRegCreateKey=Cha b' urrainn dhuinn iuchair a' chlàr-lainn a chruthachadh:%n%1\%2
ErrorRegWriteKey=Mearachd le sgrìobhadh iuchair a' chlàr-lainn:%n%1\%2

; *** INI errors
ErrorIniEntry=Mearachd le cruthachadh innteart INI san fhaidhle "%1".

; *** File copying errors
FileAbortRetryIgnore=Briog air "Ath-dhèan" gus feuchainn ris a-rithist, air "Leig seachad" gus leum thairis air an fhaidhle seo (cha mholamaid seo) no air "Sguir dheth" gus sgur dhen stàladh.
FileAbortRetryIgnore2=Briog air "Ath-dhèan" gus feuchainn ris a-rithist, air "Leig seachad" gus leantainn ort co-dhiù (cha mholamaid seo) no air "Sguir dheth" gus sgur dhen stàladh.
SourceIsCorrupted=Tha am faidhle tùsail coirbte
SourceDoesntExist=Chan eil am faidhle tùsail "%1" ann
ExistingFileReadOnly=Tha dìon sgrìobhaidh air an fhaidhle a tha ann.%n%nBriog air "Ath-dhèan" gus an dìon sgrìobhaidh a thoirt air falbh, air "Leig seachad" gus leum thairis air an fhaidhle no air "Sguir dheth" gus sgur dhen stàladh.
ErrorReadingExistingDest=Mearachd leughaidh san fhaidhle:
FileExists=Tha am faidhle seo ann mu thràth.%n%nA bheil thu airson sgrìobhadh thairis air?
ExistingFileNewer=Tha am faidhle a tha ann mu thràth nas ùire na am faidhle a tha thu airson stàladh. Mholamaid gun cùm thu am faidhle a tha ann mu thràth.%n%n A bheil thu airson am faidhle a chumail a tha ann mu thràth?
ErrorChangingAttr=Thachair mearachd le atharrachadh nam feartan aig an fhaidhle a tha ann mu thràth:
ErrorCreatingTemp=Thachair mearachd a' feuchainn ri faidhle a chruthachadh sa phasgan-uidhe:
ErrorReadingSource=Thachair mearachd a' feuchainn ris am faidhle tùsail a leughadh:
ErrorCopying=Thachair mearachd a' feuchainn ri lethbhreac a dhèanamh de dh'fhaidhle:
ErrorReplacingExistingFile=Thachair mearachd le feuchainn ri cur an àite an fhaidhle a tha ann:
ErrorRestartReplace=Dh'fhàillig le ath-thòiseachadh/cur na àite:
ErrorRenamingTemp=Thachair mearachd a' feuchainn ri ainm ùr a thoirt air faidhle sa phasgan-uidhe:
ErrorRegisterServer=Cha ghabh an DLL/OCX a chlàradh: %1
ErrorRegSvr32Failed=Dh'fhàillig RegSvr32 le còd fàgail %1
ErrorRegisterTypeLib=Cha ghabh leabharlann nan seòrsa a chlàradh: %1

; *** Post-installation errors
ErrorOpeningReadme=Mearachd le fosgladh an fhaidhle LEUGHMI.
ErrorRestartingComputer=Cha deach leis an stàladh an coimpiutair agad ath-thòiseachadh. An dèan thu an t-ath-thòiseachadh a làimh?

; *** Uninstaller messages
UninstallNotFound=Chan eil am faidhle "%1" ann. Dh'fhàillig le dì-stàladh na h-aplacaid.
UninstallOpenError=Cha b' urrainn dhuinn am faidhle "%1" fhosgladh. Dh'fhàillig le dì-stàladh na h-aplacaid
UninstallUnsupportedVer=Cha b' urrainn dhuinn fòrmat an fhaidhle dì-stàlaidh "%1" a mhothachadh. Dh'fhàillig le dì-stàladh na h-aplacaid
UninstallUnknownEntry=Tha innteart neo-aithnichte (%1) san loga dì-stàlaidh
ConfirmUninstall=A bheil thu cinnteach bu bheil thu airson %1 is a h-uile co-phàirt aige a thoirt air falbh?
UninstallOnlyOnWin64=Chan urrainn dhuinn an stàladh seo a thoirt air falbh ach fo thionndaidhean Windows 64-biod.
OnlyAdminCanUninstall=Chan fhaod ach cleachdaiche le pribhleidean rianaire an stàladh seo a thoirt air falbh.
UninstallStatusLabel=Fuirich ort fhad 's a tha %1 ga dhì-stàladh on choimpiutair agad.
UninstalledAll=Chaidh %1 a thoirt air falbh on choimpiutair agad gu soirbheachail.
UninstalledMost=Tha an dì-stàladh aig %1 deiseil.%n%nTha co-phàirtean ann nach b' urrainn dhuinn toirt air falbh. 'S urrainn dhut fhèin an sguabadh às a làimh.
UninstalledAndNeedsRestart=Feumaidh sinn an coimpiutair agad ath-thòiseachadh gus an dì-stàladh aig %1 a choileanadh.%n%nA bheil thu airson ath-thòiseachadh an-dràsta?
UninstallDataCorrupted=Tha am faidhle "%1" coirbte. Dh'fhàillig le dì-stàladh na h-aplacaid.

; *** Uninstallation phase messages
ConfirmDeleteSharedFileTitle=A bheil thu airson am faidhle co-roinnte a sguabadh às?
ConfirmDeleteSharedFile2=Tha an siostam ag innse nach tèid am faidhle co-roinnte seo a chleachdadh le prògram sam bith eile. A bheil thu airson 's gun sguab sinn às dha?%nNan robh prògraman eile ann fhathast a chleachdadh am faidhle seo is nan rachadh a thoirt air falbh, dh'fhaoidte nach biodh na prògraman ud ag obrachadh mar bu chòir tuilleadh. Ma tha thu mì-chinnteach, tagh "Chan eil" gus am faidhle fhàgail san t-siostam. Cha dèan e cron dhan t-siostam agad ma chumas tu am faidhle seo air.
SharedFileNameLabel=Ainm an fhaidhle:
SharedFileLocationLabel=Pasgan:
WizardUninstalling=Staid an dì-stàlaidh
StatusUninstalling=A' dì-stàladh %1 ...

; *** Shutdown block reasons
ShutdownBlockReasonInstallingApp=A' stàladh %1.
ShutdownBlockReasonUninstallingApp=A' dì-stàladh %1.

; The custom messages below aren't used by Setup itself, but if you make
; use of them in your scripts, you'll want to translate them.

[CustomMessages]

NameAndVersion=%1 tionndadh %2
AdditionalIcons=Ìomhaigheagan a bharrachd:
CreateDesktopIcon=Cruthaich ìomhaigheag air an &deasg
CreateQuickLaunchIcon=Cruthaich ìomhaigheag &grad-thòiseachaidh
ProgramOnTheWeb=%1 air an eadar-lìon
UninstallProgram=Dì-stàlaich %1
LaunchProgram=Tòisich %1
AssocFileExtension=&Clàraich %1 leis an leudachan fhaidhle %2
AssocingFileExtension=A' clàradh %1 leis an leudachan fhaidhle %2 ...
AutoStartProgramGroupDescription=Tòiseachadh:
AutoStartProgram=Tòisich %1 gu fèin-obrachail
AddonHostProgramNotFound=Cha deach %1 a lorg sa phasgan a thagh thu.%n%nA bheil thu airson leantainn air adhart co-dhiù?