stellarium-icon ICON "stellarium.ico"
1 VERSIONINFO
FILEVERSION     @PACKAGE_VERSION_RC@
PRODUCTVERSION  @PACKAGE_VERSION_RC@
FILEOS          0x00040004L
FILETYPE        0x00000001L
FILESUBTYPE     0x0L
BEGIN
    BLOCK "StringFileInfo"
    BEGIN
        BLOCK "000004B0"
        BEGIN
            VALUE "CompanyName",      "Stellarium team"
            VALUE "FileDescription",  "Stellarium is a free open source planetarium"
            VALUE "FileVersion",      "@PACKAGE_VERSION@"
            VALUE "LegalCopyright",   "Copyright (C) @COPYRIGHT_YEARS@ Stellarium team"
            VALUE "ProductName",      "Stellarium"
            VALUE "ProductVersion",   "@PACKAGE_VERSION@"
        END
    END
END
