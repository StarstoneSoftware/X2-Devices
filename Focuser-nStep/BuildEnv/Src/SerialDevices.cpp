//
//  SerialDevices.cpp
//  RoboFocus
//
//  Created by Richard Wright on 3/24/13.
//
//

#include "SerialDevices.h"


CSerialDevice::CSerialDevice(void)
    {
    nDeviceCount = 0;
	memset(szDeviceNames,'\0',MAX_SERIAL_DEVICES*MAX_NAME_LEN);
    }


CSerialDevice::~CSerialDevice(void)
    {
    
    
    }

const char *CSerialDevice::GetDeviceName(unsigned int nIndex)
    {
    if(nIndex >= nDeviceCount)
        return NULL;
    
    return szDeviceNames[nIndex];
    }



///////////////////////////////////////////////////////////////////////
// Finds the list of serial devices and returns the count
int CSerialDevice::FindSerialDevices(void)
    {
    // We must clear the list first
    nDeviceCount = 0;
    
#ifdef SB_MAC_BUILD
    io_iterator_t matchingServices;


    kern_return_t kernResult;
    mach_port_t masterPort;
    CFMutableDictionaryRef classesToMatch;
    kernResult = IOMasterPort(MACH_PORT_NULL, &masterPort);
    if (KERN_SUCCESS != kernResult)
        {
//        printf("IOMasterPort returned %d\n", kernResult);
        return 0;
        }
        
    // Serial devices are instances of class IOSerialBSDClient.
    classesToMatch = IOServiceMatching(kIOSerialBSDServiceValue);
    if (classesToMatch == NULL) {
//        printf("IOServiceMatching returned a NULL dictionary.\n");
        return 0;
        }
    else {
        CFDictionarySetValue(classesToMatch,
        CFSTR(kIOSerialBSDTypeKey),
        CFSTR(kIOSerialBSDModemType));
        // Each serial device object has a property with key
        // kIOSerialBSDTypeKey and a value that is one of
        // kIOSerialBSDAllTypes, kIOSerialBSDModemType,
        // or kIOSerialBSDRS232Type. You can change the
        // matching dictionary to find other types of serial
        // devices by changing the last parameter in the above call
        // to CFDictionarySetValue.
        }
        
    kernResult = IOServiceGetMatchingServices(masterPort, classesToMatch, &matchingServices);
    if (KERN_SUCCESS != kernResult)
        {
        //printf("IOServiceGetMatchingServices returned %d\n", kernResult);
        return 0;
        }
        
    io_object_t modemService;
    
    while ((modemService = IOIteratorNext(matchingServices)))
        {
        CFTypeRef deviceFilePathAsCFString;
        // Get the callout device's path (/dev/cu.xxxxx).
        // The callout device should almost always be
        // used. You would use the dialin device (/dev/tty.xxxxx) when
        // monitoring a serial port for
        // incoming calls, for example, a fax listener.
        deviceFilePathAsCFString = IORegistryEntryCreateCFProperty(modemService,
        CFSTR(kIOCalloutDeviceKey),
        kCFAllocatorDefault,
        0);

        if (deviceFilePathAsCFString)
            {
            // Convert the path from a CFString to a NULL-terminated C string
            // for use with the POSIX open() call.
            CFStringGetCString((CFStringRef)deviceFilePathAsCFString,
                szDeviceNames[nDeviceCount++],
                MAXPATHLEN,
                kCFStringEncodingASCII);
            
            CFRelease(deviceFilePathAsCFString);
            }
            
        
        // Release the io_service_t now that we are done with it.
        (void) IOObjectRelease(modemService);
        }
        
#endif

#ifdef SB_WIN_BUILD
	nDeviceCount = 64;
	for(int i = 1; i < nDeviceCount; i++)
		sprintf(szDeviceNames[i], "COM%d", i);
        
#endif

#ifdef SB_LINUX_BUILD
    nDeviceCount = 1;
    strcpy(szDeviceNames[0], "/dev/nStep");
#endif

    return nDeviceCount;
    }
