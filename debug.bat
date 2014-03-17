@ECHO OFF

CALL build debug
IF NOT %ERRORLEVEL% == 0 EXIT /B
CALL bin\wav-spi.exe
