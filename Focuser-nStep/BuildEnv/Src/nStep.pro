######################################################################
######################################################################

TEMPLATE = lib
TARGET += 
DEPENDPATH += .
INCLUDEPATH += .
INCLUDEPATH += ../
INCLUDEPATH += ../../

CONFIG += dll

win32:		DEFINES += SB_WIN_BUILD
macx:		DEFINES += SB_MAC_BUILD
linux-g++-64:	DEFINES += SB_LINUX_BUILD
DEFINES += SB_LINUX_BUILD

win32 { 
DEFINES				+= _CRT_SECURE_NO_DEPRECATE
DEFINES				+= _CRT_SECURE_NO_WARNINGS
}
# Input
HEADERS += main.h \
	   SerialDevices.h \
           x2focuser.h \
	   StopWatch.h

SOURCES += main.cpp \
	   SerialDevices.cpp \
           x2focuser.cpp 
	
