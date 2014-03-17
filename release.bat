@ECHO OFF

CALL build release
IF NOT %ERRORLEVEL% == 0 EXIT /B
CALL pack
