stellarium-icon ICON "@CMAKE_SOURCE_DIR@/data/@PACKAGE_ICON@.ico"
1 VERSIONINFO
FILEVERSION     @PACKAGE_VERSION_RC@
PRODUCTVERSION  @PACKAGE_VERSION_RC@
FILEOS          0x00040004L
FILETYPE        0x00000001L
FILESUBTYPE     0x0L
BEGIN
    BLOCK "StringFileInfo"
    BEGIN
        BLOCK "040904B0" // Lang=English (United States, 0x0409, decimal 1033), CharSet=Unicode (0x04B0, decimal 1200).
        BEGIN
            VALUE "CompanyName",      "Stellarium team\0"
            VALUE "FileDescription",  "Stellarium, the free open source planetarium\0"
            VALUE "FileVersion",      "@PACKAGE_VERSION@\0"
            VALUE "LegalCopyright",   "Copyright (C) @COPYRIGHT_YEARS@ Stellarium team\0"
            VALUE "Info",             "@STELLARIUM_URL@\0"
            VALUE "ProductVersion",   "@PACKAGE_VERSION@\0"
            VALUE "ProductName",      "Stellarium\0"
            VALUE "InternalName",     "stellarium\0"
            VALUE "OriginalFilename", "stellarium.exe\0"
        END
    END
    BLOCK "VarFileInfo"
    BEGIN
        VALUE "Translation", 0x0409, 1200
    END
END
