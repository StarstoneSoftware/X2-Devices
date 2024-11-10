//
//  SerialDevices.h
//  RoboFocus
//
//  Created by Richard Wright on 3/24/13.
//
//

#ifndef __RoboFocus__SerialDevices__
#define __RoboFocus__SerialDevices__

#include <stdio.h>
#include <string.h>
#ifdef __APPLE__
#include <unistd.h>
#endif
#include <fcntl.h>
#include <errno.h>
#include <time.h>
#include <stdlib.h>

#ifdef __APPLE__
#include <sys/ioctl.h>
#include <paths.h>
#include <termios.h>
#include <sysexits.h>
#include <sys/param.h>
#include <sys/select.h>
#include <sys/time.h>
#include <CoreFoundation/CoreFoundation.h>
#include <IOKit/IOKitLib.h>
#include <IOKit/serial/IOSerialKeys.h>
#include <IOKit/IOBSD.h>
#endif

#define MAX_SERIAL_DEVICES      64
#define MAX_NAME_LEN			256
#define TIMEOUT_READ        6000

class CSerialDevice
    {
    public:
        CSerialDevice(void);
        ~CSerialDevice(void);
    
    
        int FindSerialDevices(void);
        int  GetNumDevices(void) { return nDeviceCount; }
        const char *GetDeviceName(unsigned int nIndex);
        
    protected:
        int nDeviceCount;
        char szDeviceNames[MAX_SERIAL_DEVICES][MAX_NAME_LEN];
    
    
    };






#endif /* defined(__RoboFocus__SerialDevices__) */
