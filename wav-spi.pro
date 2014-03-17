QT += widgets
CONFIG += static
QMAKE_CXXFLAGS += -std=c++11
TEMPLATE = app

INCLUDEPATH += include
LIBPATH += libs
LIBS += -lftd2xx -static -lpthread
HEADERS = src/*.hpp
SOURCES = src/*.cpp

DESTDIR = bin

OBJECTS_DIR = $$DESTDIR/.obj
MOC_DIR = $$DESTDIR/.moc
RCC_DIR = $$DESTDIR/.qrc
UI_DIR = $$DESTDIR/.ui
