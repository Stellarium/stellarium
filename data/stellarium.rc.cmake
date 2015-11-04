stellarium-icon ICON "stellarium.ico"
1 Manifest "stellarium.exe.manifest"
1 VERSIONINFO
FILEVERSION    	@PACKAGE_VERSION_RC@
PRODUCTVERSION 	@PACKAGE_VERSION_RC@
FILEOS         	VOS_NT_WINDOWS32
FILETYPE       	VFT_APP
BEGIN
    BLOCK "StringFileInfo"
    BEGIN
        BLOCK "040904B0"
        BEGIN
            VALUE "CompanyName",      "Stellarium team"
            VALUE "FileDescription",  "Stellarium is a free open source planetarium"
            VALUE "FileVersion",      "@PACKAGE_VERSION@\0"
            VALUE "InternalName",     "stellarium"
            VALUE "LegalCopyright",   "Copyright (C) @COPYRIGHT_YEARS@ Stellarium team"
            VALUE "OriginalFilename", "stellarium.exe"
            VALUE "ProductName",      "Stellarium"
            VALUE "ProductVersion",   "@PACKAGE_VERSION@\0"
        END
    END
END
