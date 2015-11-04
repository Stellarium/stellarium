stellarium-icon ICON "stellarium.ico"
1 Manifest "stellarium.exe.manifest"
1 VERSIONINFO
FILEVERSION     @PACKAGE_VERSION_RC@
PRODUCTVERSION  @PACKAGE_VERSION_RC@
FILEOS          0x00040004L
FILETYPE        0x00000001L
FILESUBTYPE     0x0L
BEGIN
    BLOCK "StringFileInfo"
    BEGIN
        BLOCK "040904E4"
        BEGIN
            VALUE "CompanyName",      "Stellarium team\0"
            VALUE "FileDescription",  "Stellarium is a free open source planetarium\0"
            VALUE "FileVersion",      "@PACKAGE_VERSION@\0"
            VALUE "LegalCopyright",   "Copyright (C) @COPYRIGHT_YEARS@ Stellarium team\0"
            VALUE "ProductName",      "Stellarium\0"
            VALUE "ProductVersion",   "@PACKAGE_VERSION@\0"
        END
    END
END
