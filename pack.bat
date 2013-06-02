@ECHO OFF

IF NOT EXIST bin\release\wav-spi.exe (
    GOTO end
)
DEL bin\wav-spi.exe
upx -9 -o bin\wav-spi.exe bin\release\wav-spi.exe

:end
