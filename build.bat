@ECHO OFF

python pyinstaller\utils\Build.py wav-spi.spec
RMDIR /S /Q build
DEL *.log
