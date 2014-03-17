@ECHO OFF

RMDIR /S /Q bin 2>NUL
RMDIR /S /Q debug 2>NUL
RMDIR /S /Q release 2>NUL
DEL Makefile* wav-spi_plugin_import.cpp 2>NUL
