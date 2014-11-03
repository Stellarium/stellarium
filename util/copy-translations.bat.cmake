@echo off
REM Translation files copying script (by Bogdan Marinov)
REM This script copies compiled translation files from the build directory to
REM the project's root directory, allowing translations to be tested without
REM packing Stellarium or manually copying files.
REM 
REM Can be run from Qt Creator if added in "Tools"->"External"->"Configure..."
REM Just set the "Working directory" field to "%{CurrentProject:Path}"
REM 
REM WARNING: The .cmake file just generates the script, you need the .bat file
REM generated after CMake is first run!

setlocal EnableDelayedExpansion
rd @PROJECT_SOURCE_DIR_WINPATH@\locale
cd @PROJECT_BINARY_DIR_WINPATH@\po\stellarium

for %%f in (*.gmo) do (
set d=@PROJECT_SOURCE_DIR_WINPATH@\locale\%%~nf\LC_MESSAGES\
md !d!
copy .\%%f !d!\stellarium.mo
)
