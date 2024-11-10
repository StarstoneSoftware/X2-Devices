
TEMPLATE = lib
CONFIG += dll
TARGET += 
DEPENDPATH += .
INCLUDEPATH += .
INCLUDEPATH += ../
INCLUDEPATH += ../../

win32:		DEFINES += SB_WIN_BUILD
macx:		DEFINES += SB_MAC_BUILD
DEFINES += SB_LINUX_BUILD
DEFINES += OS_LINUX

# Input
HEADERS += main.h \
           x2mount.h \
           ../../../../licensedinterfaces/cameradriverinterface.h \
           ../../../../licensedinterfaces/driverrootinterface.h \
           ../../../../licensedinterfaces/linkinterface.h \
           ../../../../licensedinterfaces/deviceinfointerface.h \
           ../../../../licensedinterfaces/driverinfointerface.h \
           ../../../../licensedinterfaces/sberrorx.h \
           ../../../../licensedinterfaces/serxinterface.h \
           ../../../../licensedinterfaces/theskyxfacadefordriversinterface.h \
           ../../../../licensedinterfaces/sleeperinterface.h \
           ../../../../licensedinterfaces/loggerinterface.h \
           ../../../../licensedinterfaces/basiciniutilinterface.h \
           ../../../../licensedinterfaces/mutexinterface.h \
           ../../../../licensedinterfaces/tickcountinterface.h \
           ../../../../licensedinterfaces/basicstringinterface.h
SOURCES += main.cpp x2mount.cpp

