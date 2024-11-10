# This is only used for Linux builds...

TEMPLATE = lib
CONFIG += dll
TARGET += libMallincamX2
DEPENDPATH += .
INCLUDEPATH += .
INCLUDEPATH += ../
INCLUDEPATH += ../../
INCLUDEPATH += ../procomsdk/inc

win32:		DEFINES += SB_WIN_BUILD
macx:		DEFINES += SB_MAC_BUILD
DEFINES += SB_LINUX_BUILD
DEFINES += OS_LINUX
LIBS += -ludev
#LIBS += -L/../procomsdk/linux/armhf
LIBS += ../procomsdk/linux/arm64/libmallincam.so

# Input
HEADERS += main.h \
           x2camera.h \
           ../procomsdk/inc/mallincam.h \
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

SOURCES += main.cpp x2camera.cpp 

