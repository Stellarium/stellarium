; *** Inno Setup version 5.5.3+ Bosnian messages ***
;
; Bosnian translation by Kenan Dervisevic (kenan3008@gmail.com)
;

[LangOptions]
LanguageName=Bosanski
LanguageID=$141a
LanguageCodePage=1250
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
SetupAppTitle=Instalacija
SetupWindowTitle=Instalacija - %1
UninstallAppTitle=Deinstalacija
UninstallAppFullTitle=%1 Deinstalacija

; *** Misc. common
InformationTitle=Informacija
ConfirmTitle=Potvrda
ErrorTitle=Greška

; *** SetupLdr messages
SetupLdrStartupMessage=Zapoèeli ste instalaciju programa %1. Želite li nastaviti?
LdrCannotCreateTemp=Ne mogu kreirati privremenu datoteku. Instalacija prekinuta
LdrCannotExecTemp=Ne mogu izvršiti datoteku u privremenom folderu. Instalacija prekinuta

; *** Startup error messages
LastErrorMessage=%1.%n%nGreška %2: %3
SetupFileMissing=Datoteka %1 se ne nalazi u instalacijskom folderu. Molimo vas da riješite problem ili nabavite novu kopiju programa.
SetupFileCorrupt=Instalacijske datoteke sadrže grešku. Molimo vas da nabavite novu kopiju programa.
SetupFileCorruptOrWrongVer=Instalacijske datoteke sadrže grešku, ili nisu kompatibilne sa ovom verzijom instalacije. Molimo vas riješite problem ili nabavite novu kopiju programa.
InvalidParameter=Neispravan parametar je proslijeðen komandnoj liniji:%n%n%1
SetupAlreadyRunning=Instalacija je veæ pokrenuta.
WindowsVersionNotSupported=Ovaj program ne podržava verziju Windowsa koja je instalirana na ovom raèunaru.
WindowsServicePackRequired=Ovaj program zahtjeva %1 Service Pack %2 ili noviji.
NotOnThisPlatform=Ovaj program ne radi na %1.
OnlyOnThisPlatform=Ovaj program se mora pokrenuti na %1.
OnlyOnTheseArchitectures=Ovaj program se može instalirati samo na verzijama Windowsa napravljenim za sljedeæe arhitekture procesora:%n%n%1
MissingWOW64APIs=Verzija Windowsa koju koristite ne sadrži funkcionalnosti potrebne da bi instalacijski program mogao instalirati 64-bitnu verziju. Da bi ispravili taj problem, molimo instalirajte Service Pack %1.
WinVersionTooLowError=Ovaj program zahtjeva %1 verzije %2 ili noviju.
WinVersionTooHighError=Ovaj program se ne može instalirati na %1 verziji %2 ili novijoj.
AdminPrivilegesRequired=Morate imati administratorska prava pri instaliranju ovog programa.
PowerUserPrivilegesRequired=Morate imati administratorska prava ili biti èlan grupe Power Users prilikom instaliranja ovog programa.
SetupAppRunningError=Instalacija je detektovala da je %1 pokrenut.%n%nMolimo zatvorite program i sve njegove kopije i potom kliknite Dalje za nastavak ili Odustani za prekid.
UninstallAppRunningError=Deinstalacija je detektovala da je %1 trenutno pokrenut.%n%nMolimo zatvorite program i sve njegove kopije i potom kliknite Dalje za nastavak ili Odustani za prekid.

; *** Misc. errors
ErrorCreatingDir=Instalacija nije mogla kreirati folder "%1"
ErrorTooManyFilesInDir=Instalacija nije mogla kreirati datoteku u folderu "%1" zato što on sadrži previše datoteka

; *** Setup common messages
ExitSetupTitle=Prekid instalacije
ExitSetupMessage=Instalacija nije završena. Ako sada izaðete, program neæe biti instaliran.%n%nInstalaciju možete pokrenuti kasnije u sluèaju da je želite završiti.%n%nPrekid instalacije?
AboutSetupMenuItem=&O instalaciji...
AboutSetupTitle=O instalaciji
AboutSetupMessage=%1 verzija %2%n%3%n%n%1 poèetna stranica:%n%4
AboutSetupNote=
TranslatorNote=

; *** Buttons
ButtonBack=< Na&zad
ButtonNext=Da&lje >
ButtonInstall=&Instaliraj
ButtonOK=U redu
ButtonCancel=Otkaži
ButtonYes=&Da
ButtonYesToAll=Da za &sve
ButtonNo=&Ne
ButtonNoToAll=N&e za sve
ButtonFinish=&Završi
ButtonBrowse=&Izaberi...
ButtonWizardBrowse=Iza&beri...
ButtonNewFolder=&Napravi novi folder

; *** "Select Language" dialog messages
SelectLanguageTitle=Izaberite jezik instalacije
SelectLanguageLabel=Izaberite jezik koji želite koristiti pri instalaciji:

; *** Common wizard text
ClickNext=Kliknite na Dalje za nastavak ili Otkaži za prekid instalacije.
BeveledLabel=
BrowseDialogTitle=Izaberite folder
BrowseDialogLabel=Izaberite folder iz liste ispod, pa onda kliknite na U redu.
NewFolderName=Novi folder

; *** "Welcome" wizard page
WelcomeLabel1=Dobro došli u instalaciju programa [name]
WelcomeLabel2=Ovaj program æe instalirati [name/ver] na vaš raèunar.%n%nPreporuèujemo da zatvorite sve druge programe prije nastavka i da privremeno onemoguæite vaš antivirus i firewall.

; *** "Password" wizard page
WizardPassword=Šifra
PasswordLabel1=Instalacija je zaštiæena šifrom.
PasswordLabel3=Upišite šifru i kliknite Dalje za nastavak. Šifre su osjetljive na mala i velika slova.
PasswordEditLabel=&Šifra:
IncorrectPassword=Upisali ste pogrešnu šifru. Pokušajte ponovo.

; *** "License Agreement" wizard page
WizardLicense=Ugovor o korištenju
LicenseLabel=Molimo vas da, prije nastavka, pažljivo proèitajte sljedeæe informacije.
LicenseLabel3=Molimo vas da pažljivo proèitate Ugovor o korištenju. Morate prihvatiti uslove ugovora kako biste mogli nastaviti s instalacijom.
LicenseAccepted=&Prihvatam ugovor
LicenseNotAccepted=&Ne prihvatam ugovor

; *** "Information" wizard pages
WizardInfoBefore=Informacija
InfoBeforeLabel=Molimo vas da, prije nastavka, proèitate sljedeæe informacije.
InfoBeforeClickLabel=Kada budete spremni nastaviti instalaciju, kliknite na Dalje.
WizardInfoAfter=Informacija
InfoAfterLabel=Molimo vas da, prije nastavka, proèitate sljedeæe informacije.
InfoAfterClickLabel=Kada budete spremni nastaviti instalaciju, kliknite na Dalje.

; *** "User Information" wizard page
WizardUserInfo=Informacije o korisniku
UserInfoDesc=Upišite vaše liène informacije.
UserInfoName=&Ime korisnika:
UserInfoOrg=&Organizacija:
UserInfoSerial=&Serijski broj:
UserInfoNameRequired=Morate upisati ime.

; *** "Select Destination Location" wizard page
WizardSelectDir=Odaberite odredišni folder
SelectDirDesc=Gdje želite da instalirate [name]?
SelectDirLabel3=Instalacija æe instalirati [name] u sljedeæi folder.
SelectDirBrowseLabel=Za nastavak, kliknite Dalje. Ako želite izabrati drugi folder, kliknite Izaberi.
DiskSpaceMBLabel=Ovaj program zahtjeva najmanje [mb] MB slobodnog prostora na disku.
CannotInstallToNetworkDrive=Instalacija nije moguæa na mrežnom disku.
CannotInstallToUNCPath=Instalacija nije moguæa za UNC putanju.
InvalidPath=Morate unijeti punu putanju zajedno sa slovom diska; npr:%n%nC:\APP%n%nili UNC putanju u obliku:%n%n\\server\share
InvalidDrive=Disk ili UNC share koji ste odabrali ne postoji ili je nedostupan. Odaberite neki drugi.
DiskSpaceWarningTitle=Nedovoljno prostora na disku
DiskSpaceWarning=Instalacija zahtjeva bar %1 KB slobodnog prostora, a odabrani disk ima samo %2 KB na raspolaganju.%n%nDa li želite nastaviti?
DirNameTooLong=Naziv ili putanja do foldera su predugi.
InvalidDirName=Naziv foldera nije ispravan.
BadDirName32=Naziv foldera ne smije sadržavati niti jedan od sljedeæih znakova:%n%n%1
DirExistsTitle=Folder postoji
DirExists=Folder:%n%n%1%n%nveæ postoji. Želite li i dalje izvršiti instalaciju u njega?
DirDoesntExistTitle=Folder ne postoji
DirDoesntExist=Folder:%n%n%1%n%nne postoji. Želite li ga napraviti?

; *** "Select Components" wizard page
WizardSelectComponents=Odaberite komponente
SelectComponentsDesc=Koje komponente želite instalirati?
SelectComponentsLabel2=Odaberite komponente koje želite instalirati ili uklonite kvaèicu pored komponenti koje ne želite. Kliknite Dalje kad budete spremni da nastavite.
FullInstallation=Puna instalacija
; if possible don't translate 'Compact' as 'Minimal' (I mean 'Minimal' in your language)
CompactInstallation=Kompaktna instalacija
CustomInstallation=Instalacija prema želji
NoUninstallWarningTitle=Komponente postoje
NoUninstallWarning=Instalacija je detektovala da na vašem raèunaru veæ postoje sljedeæe komponente:%n%n%1%n%nAko ove komponente ne odaberete, neæe doæi do njihove deinstalacije.%n%nŽelite li ipak nastaviti?
ComponentSize1=%1 KB
ComponentSize2=%1 MB
ComponentsDiskSpaceMBLabel=Trenutni izbor zahtjeva bar [mb] MB prostora na disku.

; *** "Select Additional Tasks" wizard page
WizardSelectTasks=Izaberite dodatne radnje
SelectTasksDesc=Koje dodatne radnje želite da se izvrše?
SelectTasksLabel2=Izaberite radnje koje æe se izvršiti tokom instalacije programa [name], onda kliknite Dalje.

; *** "Select Start Menu Folder" wizard page
WizardSelectProgramGroup=Izaberite programsku grupu
SelectStartMenuFolderDesc=Gdje instalacija treba da napravi preèice?
SelectStartMenuFolderLabel3=Izaberite folder iz Start menija u koji želite da instalacija kreira preèicu, a zatim kliknite na Dalje.
SelectStartMenuFolderBrowseLabel=Za nastavak, kliknite Dalje. Ako želite da izaberete drugi folder, kliknite Izaberi.
MustEnterGroupName=Morate unijeti ime programske grupe.
GroupNameTooLong=Naziv foldera ili putanje je predug.
InvalidGroupName=Naziv foldera nije ispravan.
BadGroupName=Naziv foldera ne smije sadržavati niti jedan od sljedeæih znakova:%n%n%1
NoProgramGroupCheck2=&Ne kreiraj programsku grupu

; *** "Ready to Install" wizard page
WizardReady=Spreman za instalaciju
ReadyLabel1=Sada smo spremni za instalaciju [name] na vaš raèunar.
ReadyLabel2a=Kliknite na Instaliraj ako želite instalirati program ili na Nazad ako želite pregledati ili promjeniti postavke.
ReadyLabel2b=Kliknite na Instaliraj ako želite nastaviti sa instalacijom programa.
ReadyMemoUserInfo=Informacije o korisniku:
ReadyMemoDir=Odredišni folder:
ReadyMemoType=Tip instalacije:
ReadyMemoComponents=Odabrane komponente:
ReadyMemoGroup=Programska grupa:
ReadyMemoTasks=Dodatne radnje:

; *** "Preparing to Install" wizard page
WizardPreparing=Pripremam instalaciju
PreparingDesc=Pripreme za instalaciju [name] na vaš raèunar.
PreviousInstallNotCompleted=Instalacija/deinstalacija prethodnog programa nije završena. Morate restartovati vaš raèunar kako bi završili tu instalaciju.%n%nNakon toga, ponovno pokrenite ovaj program kako bi dovršili instalaciju za [name].
CannotContinue=Instalacija ne može nastaviti. Molimo vas da kliknete na Odustani za izlaz.
ApplicationsFound=Sljedeæe aplikacije koriste datoteke koje ova instalacija treba da nadogradi. Preporuèujemo vam da omoguæite instalaciji da automatski zatvori ove aplikacije.
ApplicationsFound2=Sljedeæe aplikacije koriste datoteke koje ova instalacija treba da nadogradi. Preporuèujemo vam da omoguæite instalaciji da automatski zatvori ove aplikacije. Nakon što se sve završi, bit æe izvršen pokušaj ponovnog pokretanja ovih aplikacija.
CloseApplications=&Automatski zatvori aplikacije
DontCloseApplications=&Ne zatvaraj aplikacije
ErrorCloseApplications=Instalacija nije mogla automatski zatvoriti sve aplikacije. Prije nego nastavite, preporuèujemo vam da zatvorite sve aplikacije koje koriste datoteke koje æe ova instalacija trebati da ažurira.

; *** "Installing" wizard page
WizardInstalling=Instaliram
InstallingLabel=Prièekajte dok se ne završi instalacija programa [name] na vaš raèunar.

; *** "Setup Completed" wizard page
FinishedHeadingLabel=Završavam instalaciju [name]
FinishedLabelNoIcons=Instalacija programa [name] je završena.
FinishedLabel=Instalacija programa [name] je završena. Program možete pokrenuti koristeæi instalirane ikone.
ClickFinish=Kliknite na Završi da biste izašli iz instalacije.
FinishedRestartLabel=Da biste instalaciju programa [name] završili, potrebno je restartovati raèunar. Želite li to sada uèiniti?
FinishedRestartMessage=Završetak instalacije programa [name] zahtjeva restart vašeg raèunara.%n%nŽelite li to sada uèiniti?
ShowReadmeCheck=Da, želim proèitati README datoteku.
YesRadio=&Da, restartuj raèunar sada
NoRadio=&Ne, restartovat æu raèunar kasnije
; used for example as 'Run MyProg.exe'
RunEntryExec=Pokreni %1
; used for example as 'View Readme.txt'
RunEntryShellExec=Proèitaj %1

; *** "Setup Needs the Next Disk" stuff
ChangeDiskTitle=Instalacija treba sljedeæi disk
SelectDiskLabel2=Molimo ubacite Disk %1 i kliknite U redu.%n%nAko se datoteke na ovom disku nalaze u drugom folderu a ne u onom prikazanom ispod, unesite ispravnu putanju ili kliknite na Izaberi.
PathLabel=&Putanja:
FileNotInDir2=Datoteka "%1" ne postoji u "%2". Molimo vas ubacite odgovorajuæi disk ili odaberete drugi folder.
SelectDirectoryLabel=Molimo odaberite lokaciju sljedeæeg diska.

; *** Installation phase messages
SetupAborted=Instalacija nije završena.%n%nMolimo vas da riješite problem i opet pokrenete instalaciju.
EntryAbortRetryIgnore=Kliknite na Retry da pokušate opet, Ignore da nastavite, ili Abort da prekinete instalaciju.

; *** Installation status messages
StatusClosingApplications=Zatvaram aplikacije...
StatusCreateDirs=Kreiram foldere...
StatusExtractFiles=Raspakujem datoteke...
StatusCreateIcons=Kreiram preèice...
StatusCreateIniEntries=Kreiram INI datoteke...
StatusCreateRegistryEntries=Kreiram podatke za registracijsku bazu...
StatusRegisterFiles=Registrujem datoteke...
StatusSavingUninstall=Snimam deinstalacijske informacije...
StatusRunProgram=Završavam instalaciju...
StatusRestartingApplications=Restartujem aplikaciju...
StatusRollback=Poništavam promjene...

; *** Misc. errors
ErrorInternal2=Interna greška: %1
ErrorFunctionFailedNoCode=%1 nije uspjelo
ErrorFunctionFailed=%1 nije uspjelo; kod %2
ErrorFunctionFailedWithMessage=%1 nije uspjelo; kod %2.%n%3
ErrorExecutingProgram=Ne mogu pokrenuti datoteku:%n%1

; *** Registry errors
ErrorRegOpenKey=Greška pri otvaranju registracijskog kljuèa:%n%1\%2
ErrorRegCreateKey=Greška pri kreiranju registracijskog kljuèa:%n%1\%2
ErrorRegWriteKey=Greška pri zapisivanju registracijskog kljuèa:%n%1\%2

; *** INI errors
ErrorIniEntry=Greška pri kreiranju INI podataka u datoteci "%1".

; *** File copying errors
FileAbortRetryIgnore=Kliknite Retry da pokušate ponovo, Ignore da preskoèite ovu datoteku (nije preporuèeno), ili Abort da prekinete instalaciju.
FileAbortRetryIgnore2=Kliknite Retry da pokušate ponovo, Ignore da preskoèite ovu datoteku (nije preporuèeno), ili Abort da prekinete instalaciju.
SourceIsCorrupted=Izvorna datoteka je ošteæena
SourceDoesntExist=Izvorna datoteka "%1" ne postoji
ExistingFileReadOnly=Postojeæa datoteka je oznaèena kao samo za èitanje.%n%nKliknite Retry da uklonite ovu oznaku i pokušate ponovo, Ignore da preskoèite ovu datoteku, ili Abort da prekinete instalaciju.
ErrorReadingExistingDest=Došlo je do greške prilikom pokušaja èitanja postojeæe datoteke:
FileExists=Datoteka veæ postoji.%n%nŽelite li pisati preko nje?
ExistingFileNewer=Postojeæa datoteka je novija od one koju pokušavate instalirati. Preporuèujemo vam da zadržite postojeæu datoteku.%n%nŽelite li zadržati postojeæu datoteku?
ErrorChangingAttr=Pojavila se greška prilikom pokušaja promjene atributa postojeæe datoteke:
ErrorCreatingTemp=Pojavila se greška prilikom pokušaja kreiranja datoteke u odredišnom folderu:
ErrorReadingSource=Pojavila se greška prilikom pokušaja èitanja izvorne datoteke:
ErrorCopying=Pojavila se greška prilikom pokušaja kopiranja datoteke:
ErrorReplacingExistingFile=Pojavila se greška prilikom pokušaja zamjene datoteke:
ErrorRestartReplace=Ponovno pokretanje i zamjena nije uspjela:
ErrorRenamingTemp=Pojavila se greška prilikom pokušaja preimenovanja datoteke u odredišnom folderu:
ErrorRegisterServer=Ne mogu registrovati DLL/OCX: %1
ErrorRegSvr32Failed=RegSvr32 nije ispravno izvršen, kod na kraju izvršavanja %1
ErrorRegisterTypeLib=Ne mogu registrovati tip biblioteke: %1

; *** Post-installation errors
ErrorOpeningReadme=Pojavila se greška prilikom pokušaja otvaranja README datoteke.
ErrorRestartingComputer=Instalacija ne može restartovati vaš raèunar. Molimo vas da to uèinite ruèno.

; *** Uninstaller messages
UninstallNotFound=Datoteka "%1" ne postoji. Deinstalacija prekinuta.
UninstallOpenError=Datoteka "%1" se ne može otvoriti. Deinstalacija nije moguæa
UninstallUnsupportedVer=Deinstalacijska log datoteka "%1" je u formatu koji nije prepoznat od ove verzije deinstalera. Nije moguæa deinstalacija
UninstallUnknownEntry=Nepoznat zapis (%1) je pronadjen u deinstalacijskoj log datoteci
ConfirmUninstall=Da li ste sigurni da želite ukloniti %1 i sve njegove komponente?
UninstallOnlyOnWin64=Ovaj program se može deinstalirati samo na 64-bitnom Windowsu.
OnlyAdminCanUninstall=Ova instalacija može biti uklonjena samo od korisnika sa administratorskim privilegijama.
UninstallStatusLabel=Molimo prièekajte dok %1 ne bude uklonjen s vašeg raèunara.
UninstalledAll=Program %1 je uspješno uklonjen sa vašeg raèunara.
UninstalledMost=Deinstalacija programa %1 je završena.%n%nNeke elemente nije bilo moguæe ukloniti. Molimo vas da to uèinite ruèno.
UninstalledAndNeedsRestart=Da bi završili deinstalaciju %1, Vaš raèunar morate restartati%n%nŽelite li to uèiniti sada? 
UninstallDataCorrupted="%1" datoteka je ošteæena. Deinstalacija nije moguæa.

; *** Uninstallation phase messages
ConfirmDeleteSharedFileTitle=Ukloni dijeljenu datoteku
ConfirmDeleteSharedFile2=Sistem ukazuje da sljedeæe dijeljene datoteke ne koristi nijedan drugi program. Želite li da Deinstalacija ukloni te dijeljene datoteke?%n%nAko neki programi i dalje koriste te datoteke, a one se obrišu, ti programi neæe raditi ispravno. Ako niste sigurni, odaberite Ne. Ostavljanje datoteka neæe uzrokovati štetu vašem sistemu.
SharedFileNameLabel=Datoteka:
SharedFileLocationLabel=Putanja:
WizardUninstalling=Status deinstalacije
StatusUninstalling=Deinstaliram %1...

; *** Shutdown block reasons
ShutdownBlockReasonInstallingApp=Instaliram %1.
ShutdownBlockReasonUninstallingApp=Deinstaliram %1.

; The custom messages below aren't used by Setup itself, but if you make
; use of them in your scripts, you'll want to translate them.

[CustomMessages]

NameAndVersion=%1 verzija %2
AdditionalIcons=Dodatne ikone:
CreateDesktopIcon=Kreiraj &desktop ikonu
CreateQuickLaunchIcon=Kreiraj &Quick Launch ikonu
ProgramOnTheWeb=%1 na webu
UninstallProgram=Deinstaliraj %1
LaunchProgram=Pokreni %1
AssocFileExtension=&Asociraj %1 sa %2 ekstenzijom
AssocingFileExtension=Asociram %1 sa %2 ekstenzijom...
AutoStartProgramGroupDescription=Pokretanje:
AutoStartProgram=Automatski pokreæi %1
AddonHostProgramNotFound=%1 nije mogao biti pronaðen u folderu koji ste odabrali.%n%nDa li i dalje želite nastaviti s ovom akcijom?
