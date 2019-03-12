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
rd D:\stellarium\locale
cd D:\stellarium\builds\qtcreator-unknown\po\stellarium

for %%f in (*.gmo) do (
set d=D:\stellarium\locale\%%~nf\LC_MESSAGES\
md !d!
copy .\%%f !d!\stellarium.mo
)
