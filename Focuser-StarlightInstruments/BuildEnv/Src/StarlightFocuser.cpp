//
//  StarlightFocuser.cpp
//  untitled
//
//  Created by Richard Wright on 5/27/18.
//  Starstone Software Systems, Inc.

#include "StarlightFocuser.h"

StarlightFocuser::StarlightFocuser(void)
    {
    _deviceHandle = nullptr;
    hid_init();
    
    }
    

StarlightFocuser::~StarlightFocuser(void)
    {
    // Make sure device is closed
    if(_deviceHandle)
        hid_close(_deviceHandle);
    
    hid_exit();
    }
    
        
bool StarlightFocuser::Connect(void)
    {
    _deviceHandle = hid_open(0x04d8, 0xf056, 0);
    
    if(_deviceHandle == nullptr)
        return false;
        
    // Read the Manufacturer String
    int res;
    res = hid_get_manufacturer_string(_deviceHandle, cManufacturer, MAX_FOCUSER_STRING);
    
    // Read the Product String
    res = hid_get_product_string(_deviceHandle, cProductName, MAX_FOCUSER_STRING);

	// Get the firmware
	if (getFirmware(firmwareMajor, firmwareMinor) <= 0)
		return false;
  
    // Unlock if it was locked
    unsigned char cPayload[5] = { 0x72, 0x5a, 0x2 };
    if (SendReceive(cPayload) <= 0)         
        return false;

    return true;
    }
    
    
void StarlightFocuser::Disconnect(void)
    {
    hid_close(_deviceHandle);
    _deviceHandle = nullptr;
    }
    
    
////////////////////////////////////////////////////////////////
// Send and receive data with device.
// Returns 0 on timeout
// Returns -1 on error
// Returns the number of bytes read from the device
int StarlightFocuser::SendReceive(unsigned char cPayload[5])
    {
    int nRet = 0;
    if(_deviceHandle == nullptr) return -1;

    unsigned char command = cPayload[0];
    
#if SB_WIN_BUILD
    unsigned char cAdjusted[5] = { 0x0 };
    memcpy(cAdjusted + 1, cPayload, 4);
    // Send the payload
    nRet = hid_write(_deviceHandle, cAdjusted, 5);
#else
    // Send the payload
    nRet = hid_write(_deviceHandle, cPayload, 5);
#endif

    if(nRet < 0) return nRet;

    // Get result - 6 second timeout
    nRet = hid_read_timeout(_deviceHandle, cPayload, 5, 6000);
    if(nRet <= 0) return nRet;
    
    // Double check, command should always be echoed, or zero is okay for some commands
    if(cPayload[0] != command && cPayload != 0) return -1;
    
    // Return the number of bytes
    return nRet;
    }


/////////////////////////////////////////////////////////////////
// Read the firmware
int StarlightFocuser::getFirmware(unsigned char &major, unsigned char& minor)
    {
    if(_deviceHandle == nullptr) return -1;
    
    unsigned char cPayload[5] = { 0x51 };
	int nRet = SendReceive(cPayload);
	if (nRet > 0) {
		// Data is valid
		minor = cPayload[1];
		major = cPayload[2];
		}

	return nRet;
    }

    
////////////////////////////////////////////////////////////////
// Read the temperature. Returns false if no sensor, or not connected.
bool StarlightFocuser::readTemp(double& dTemp)
    {
    if(_deviceHandle == nullptr) return false;
    
    // Ask for the temperature
    unsigned char cPayload[5] = { 0x41 };
    if(SendReceive(cPayload) <= 0)
        return false;
        
    // No sensor 
    if(cPayload[1] == 0x0 && cPayload[2] == 0x80)
        return false;
        
    // Compute color
    short sTemp;
    unsigned char *cheat = (unsigned char*)&sTemp;
    cheat[0] = cPayload[1];
    cheat[1] = cPayload[2];

    dTemp = double(sTemp) / 256.0;
    return true;
    }

///////////////////////////////////////////////////////////////
// Reads the motor state. Returns 0 on timeout, -1 on other error
int StarlightFocuser::getMotorState(unsigned int& nState)
    {
    unsigned char cPayload[5] = { 0x11 };
    int nRet = SendReceive(cPayload);
    if(nRet > 0) {
        nState = cPayload[1];
        return 1;
        }
    
    return nRet;
    }
    
///////////////////////////////////////////////////////////////
// Get motor polaity. 0 = normal, else reverse
// Returns 0 on timeout, -1 on other error.
int StarlightFocuser::getMotorPolarity(unsigned int& nState)
    {
    unsigned char cPayload[5] = { 0x61 };
    int nRet = SendReceive(cPayload);
    if(nRet <= 0) return nRet;
    nState = cPayload[1];
    return nRet;
    }
    
    
////////////////////////////////////////////////////////////////
// Reverse motor polarity
// Returns 0 on timeout, -1 on other error
int StarlightFocuser::setMotorPolarity(unsigned int nReversed)
    {
    unsigned char cPayload[5] = { 0x60, (unsigned char)nReversed };
    return SendReceive(cPayload);
    }
        
////////////////////////////////////////////////////////////////
// Reverse handset knob.
// 0 is normal, otherwise it is reversed
int StarlightFocuser::getHandsetDirection(unsigned int& nDir)
    {
    unsigned char cPayload[5] = { 0x63 };
    int nRet = SendReceive(cPayload);
    if(nRet <= 0) return nRet;
    nDir = cPayload[1];
    return nRet;
    }

////////////////////////////////////////////////////////////////
// Reverse handset knob.
// 0 is normal, otherwise it is reversed    
int StarlightFocuser::setHandsetDirection(unsigned int nDir)
    {
    unsigned char cPayload[5] = { 0x62, (unsigned char)nDir };
    return SendReceive(cPayload);
    }


////////////////////////////////////////////////////////////////
// Get the motor positon. Returns 0 on timeout, -1 on other error
int StarlightFocuser::getMotorPosition(unsigned int& nPosition)
    {
    unsigned short lower16 = 0;
    unsigned short upper16 = 0;

    // Get the high 4 bits
    unsigned char cPayload[5] = { 0x29 };
    int nRet = SendReceive(cPayload);
    if(nRet <=0) return nRet;
    unsigned char *cheat;
    upper16 = cPayload[1];

    // Get the lower 16-bits
    cPayload[0] =  0x21;
    nRet = SendReceive(cPayload);
    if(nRet <= 0) return nRet;
    
    cheat = (unsigned char *)&lower16;
    cheat[0] = cPayload[1];
    cheat[1] = cPayload[2];
    
    
    nPosition = int(upper16) * 65536 + lower16;
    return 1;
    }


/////////////////////////////////////////////////////////////////
// Start a goto operation
int StarlightFocuser::gotoPosition(unsigned int nPosition)
    {
    // Set the upper bits first
    unsigned short lower16 = (nPosition & 0xffff);
    unsigned short upper16 = (nPosition >> 16) & 0xf;
    
    // Upper 4 bits
    unsigned char cPayload[5] = { 0x28 };
	cPayload[1] = upper16;
    cPayload[2] = 0x0;
    int nRet = SendReceive(cPayload);
    if(nRet <= 0) return nRet;
    
    // Lower 16-bits
    cPayload[0] = 0x20;
    cPayload[1] = lower16 & 0xff;
    cPayload[2] = lower16 >> 8;
    nRet = SendReceive(cPayload);
    if(nRet <= 0) return nRet;
    
    // Tell it to go
    cPayload[0] = 0x10;
    cPayload[1] = 0x03;
    nRet = SendReceive(cPayload);
    return nRet;
    }

////////////////////////////////////////////////////////////////
// Save the motor positon to the firmware
int StarlightFocuser::saveMotorPosition(int nOverride)
    {
    // First set to override position?
    if(nOverride >= 0) {
        // Set the upper bits first
        unsigned int nPosition = nOverride; // convert to unsigned first
        unsigned short lower16 = (nPosition & 0xffff);
        unsigned short upper16 = (nPosition >> 16) & 0xf;
        
        // Upper 4 bits
        unsigned char cPayload[5] = { 0x28 };
        cPayload[1] = upper16;
        cPayload[2] = 0x0;
        int nRet = SendReceive(cPayload);
        if(nRet <= 0) return nRet;
        
        // Lower 16-bits
        cPayload[0] = 0x20;
        cPayload[1] = lower16 & 0xff;
        cPayload[2] = lower16 >> 8;
        nRet = SendReceive(cPayload);
        if(nRet <= 0) return nRet;
        
        // Okay, now "set" it for real
        cPayload[0] = 0x10;
        cPayload[1] = 0x04;
        nRet = SendReceive(cPayload);
        if(nRet <=0) return nRet;
        }
        
    // Save to firmware too
    unsigned char cPayload[5] = { 0x70, 0xa5 };
    int nRet = SendReceive(cPayload);
    if(nRet <= 0)   // Error
        return nRet;
        
    // Still might be wrong...
    if(cPayload[1] != 0xa5)
        return -1;
    
    // Okay, return >0 for success
    return nRet;
    }
    
/////////////////////////////////////////////////////////////////
// Is the motor turned off on idle
int StarlightFocuser::getMotorOffOnIdle(unsigned int& nState)
    {
    unsigned char cPayload[5] = { 0x3b };
    int nRet = SendReceive(cPayload);
    if(nRet <= 0)
        return nRet;
        
    nState = cPayload[1];
    return nRet;
    }
    
/////////////////////////////////////////////////////////////////
int StarlightFocuser::setMotorOffOnIdle(unsigned int nState)
    {
    unsigned char cPayload[5] = { 0x3a, 0x00 };
    if(nState != 0)
        cPayload[1] = 0xff;
    
    return SendReceive(cPayload);
    }
    
////////////////////////////////////////////////////////////////
// Get the step period 0 - 100ms
int StarlightFocuser::getStepPeriod(unsigned char& nDelay)
    {
    unsigned char cPayload[5] = { 0x31 };
    int nRet = SendReceive(cPayload);
    if(nRet <= 0) return nRet;
    nDelay = cPayload[1];
    return nRet;
    }
    
////////////////////////////////////////////////////////////////
// Set the step period 0 - 100 ms
int StarlightFocuser::setStepPeriod(unsigned char nDelay)
    {
    if(nDelay > 100) nDelay = 100;
    unsigned char cPayload[5] = { 0x30, nDelay };
    return SendReceive(cPayload);
    }

////////////////////////////////////////////////////////////////
// Freeze all motor functions ;-)
int StarlightFocuser::abortMove(void)
    {
    unsigned char cPayload[5] = { 0x10, 0xff };
    return SendReceive(cPayload);
    }


/////////////////////////////////////////////////////////////////////////////////////
// Get the backlash compensation settings.
// nSteps = 0 - 255 steps of compensation
// inOut = prefered direction 0 = in, 1 = out
int StarlightFocuser::getBacklashSettings(unsigned char& nSteps, unsigned char& inOut)
    {
    unsigned char cPayload[5] = { 0x33 };
    int nRet = SendReceive(cPayload);
    if(nRet <= 0) return nRet;
    nSteps = cPayload[1];
    inOut = cPayload[2];
    return nRet;
    }
    
    
/////////////////////////////////////////////////////////////////////////////////////
// Set the backlash compensation settings.
// nSteps = 0 - 255 steps of compensation
// inOut = prefered direction 0 = in, 1 = out
int StarlightFocuser::setBacklashSettings(unsigned char nSteps, unsigned char inOut)
    {
    if(inOut > 1) inOut = 1;
    
    unsigned char cPayload[5] = { 0x32, nSteps, inOut };
    return SendReceive(cPayload);
    }

    
/////////////////////////////////////////////////////////////////
// Is temperature compensation enabled?
// nState = 0 not enabled
// nState = 0xff enabled
int StarlightFocuser::getTempCompEnabled(unsigned int& nState)
    {
    unsigned char cPayload[5] = { 0x39 };
    int nRet = SendReceive(cPayload);
    if(nRet <= 0) return nRet;
    
    nState = cPayload[1];
    return nRet;
    }
    
/////////////////////////////////////////////////////////////////
// Enable temperatrue compensation
// 0 = turn off
// Non-zero = turn on
int StarlightFocuser::setTempCompEnabled(unsigned int nState)
    {
    unsigned char cPayload[5] = { 0x38, 0x0 };
    if(nState != 0)
        cPayload[1] = 0xff;
        
    return SendReceive(cPayload);
    }


/////////////////////////////////////////////////////////////////
// Get the user settable maximum position.
int StarlightFocuser::getMaxPosition(unsigned int& nPos)
    {
	if ((firmwareMajor <= 1) && (firmwareMinor < 4))
		return -1;

    unsigned short lower16 = 0;
    unsigned short upper16 = 0;

    // Get the high 4 bits
    unsigned char cPayload[5] = { 0x2b };
    int nRet = nRet = SendReceive(cPayload);
    if(nRet <=0) return nRet;
	unsigned char *cheat;
	upper16 = cPayload[1];

    // Get the lower 16-bits
    cPayload[0] =  0x23;
    nRet = SendReceive(cPayload);
    if(nRet <= 0) return nRet;
    
    cheat = (unsigned char *)&lower16;
    cheat[0] = cPayload[1];
    cheat[1] = cPayload[2];    
    nPos = int(upper16) * 65536 + lower16;
    
    return nRet;
    }
    
    
int StarlightFocuser::setMaxPosition(unsigned int nPos)
    {
	if ((firmwareMajor <= 1) && (firmwareMinor < 4))
		return -1;

    // Set the upper bits first
    unsigned short lower16 = (nPos & 0xffff);
    unsigned short upper16 = (nPos >> 16) & 0xf;
    
    // Upper 4 bits
    unsigned char cPayload[5] = { 0x2a };
	cPayload[1] = upper16;
    cPayload[2] = 0x0;
    int nRet = SendReceive(cPayload);
    if(nRet <= 0) return nRet;
    
    // Lower 16-bits
    cPayload[0] = 0x22;
    cPayload[1] = lower16 & 0xff;
    cPayload[2] = lower16 >> 8;
    nRet = SendReceive(cPayload);
    return nRet;
    }
    
// Firmware must be 1.4 or later
bool StarlightFocuser::hasMaxSetting(void)
	{
	if (_deviceHandle == nullptr)
		return false;

	if ((firmwareMajor <= 1) && (firmwareMinor < 4))
		return false;

	return true;
	}


    
