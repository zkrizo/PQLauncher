QT       += core gui widgets webenginewidgets

CONFIG += qt

TARGET = PQLauncher
TEMPLATE = vcapp

SOURCES += main.cpp\
    appwindow.cpp \
    md5.cpp \
    md5batch.cpp \
    updatecheck.cpp \
    updatefiles.cpp

HEADERS  += \
    appwindow.h \
    md5.h \
    md5batch.h \
    updatecheck.h \
    updatefiles.h


INCLUDEPATH += "G:\Program Files\Boost\boost_1_55_0"
INCLUDEPATH += "G:\Development\URDL\include"
INCLUDEPATH += "G:\Development\OpenSSL\include"
INCLUDEPATH += L"/Data/bin/"
LIBS += -L"G:\Development\URDL\lib"
LIBS += -L"G:\Program Files\Boost\boost_1_55_0\stage\lib"
LIBS += shell32.lib
LIBS += Ole32.lib
LIBS += Qt5PlatformSupport.lib

RC_FILE = pqicon.rc
