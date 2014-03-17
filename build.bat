@ECHO OFF

IF "%1"=="debug" SET QMAKEARG="CONFIG += console debug"
IF "%1"=="release" SET QMAKEARG=""

CALL qmake %QMAKEARG%
CALL mingw32-make release
IF NOT %ERRORLEVEL% == 0 EXIT /B %ERRORLEVEL%
RMDIR /S /Q debug 2>NUL
RMDIR /S /Q release 2>NUL
DEL Makefile* wav-spi_plugin_import.cpp 2>NUL
