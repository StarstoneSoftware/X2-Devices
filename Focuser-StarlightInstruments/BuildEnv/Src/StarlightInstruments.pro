######################################################################
######################################################################

TEMPLATE = lib
TARGET += 
DEPENDPATH += .
INCLUDEPATH += .
INCLUDEPATH += ../
INCLUDEPATH += ../../
INCLUDEPATH += ../../../../libs/hidapi-master/hidapi

CONFIG += dll

win32:		DEFINES += SB_WIN_BUILD
macx:		DEFINES += SB_MAC_BUILD
linux-g++-64:	DEFINES += SB_LINUX_BUILD
linux-g++-32:   DEFINES += SB_LINUX_BUILD
linux:		LIBS += -ludev

win32 { 
DEFINES				+= _CRT_SECURE_NO_DEPRECATE
DEFINES				+= _CRT_SECURE_NO_WARNINGS
}
# Input
HEADERS += main.h \
	       StarlightFocuser.h \
	       x2focuser.h 
	   
SOURCES += main.cpp \
		   StarlightFocuser.cpp \
	       x2focuser.cpp \
		../../../../libs/hidapi-master/linux/hid.c
	
