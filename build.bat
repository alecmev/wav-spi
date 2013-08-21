@ECHO OFF

RMDIR /S /Q bin
IF "%1"=="debug" GOTO debug
IF "%1"=="release" GOTO release
GOTO end

:debug
qmake "CONFIG += console debug"
GOTO make

:release
qmake
GOTO make

:make
CALL make release
RMDIR /S /Q debug
RMDIR /S /Q release
DEL Makefile* wav-spi_plugin_import.cpp

:end
