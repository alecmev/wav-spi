@ECHO OFF

IF NOT EXIST bin\wav-spi.exe (
    EXIT /B
)

DEL bin\wav-spi.raw.exe 2>NUL
MOVE bin\wav-spi.exe bin\wav-spi.raw.exe
upx -9 -o bin\wav-spi.exe bin\wav-spi.raw.exe
