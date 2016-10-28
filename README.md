# wav-spi

This project is of no use to anyone, but it can be employed as a real-life
example of usage of synchronous bitbang mode, available on many FTDI chips,
which are compatible with FTD2XX driver.

Check out the [`cbus`](https://github.com/jeremejevs/wav-spi/tree/cbus) branch
to see an earlier version of this utility, implemented in Python.

### Gotchas

##### General

* Designed to work with any M25 memory chip (but tested on Micron M25P128 only).
* GNU Compiler Collection 4.8.2 or newer is required, get it
  [here](http://sourceforge.net/projects/mingw-w64/files/Toolchains%20targetting%20Win32/Personal%20Builds/mingw-builds/)
  (posix/dwarf will do).
* Place [ftd2xx.dll](http://www.ftdichip.com/Drivers/D2XX.htm) into SysWOW64.

##### Qt 5

* Checkout from their stable branch. Make sure that you place the sources into a
  directory without any special symbols in its path.
* Edit `qtbase/configure.bat` so that g++ is the first choice.
* Use this command to configure it:
```
configure -platform win32-g++ -static -release -opensource -no-rtti ^
-no-openssl -no-opengl -no-vcproj -no-cetest -qt-sql-odbc -qt-sql-sqlite ^
-plugin-sql-odbc -plugin-sql-sqlite -no-opengl -no-openvg -no-incredibuild-xge ^
-nomake examples -nomake tests -skip qtquick1 -skip declarative
```
