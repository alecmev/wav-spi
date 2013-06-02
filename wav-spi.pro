QT += widgets
CONFIG += static
QMAKE_CXXFLAGS += -std=c++11
TEMPLATE = app

INCLUDEPATH += include
LIBPATH += libs
LIBS += -lftd2xx

HEADERS = \
	src/mainwindow.h

SOURCES = \
	src/main.cpp \
	src/mainwindow.cpp

release: DESTDIR = bin/release
debug: DESTDIR = bin/debug

OBJECTS_DIR = $$DESTDIR/.obj
MOC_DIR = $$DESTDIR/.moc
RCC_DIR = $$DESTDIR/.qrc
UI_DIR = $$DESTDIR/.ui
