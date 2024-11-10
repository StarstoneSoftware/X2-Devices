//
//  StarlightFocuser.h
//  untitled
//
//  Created by Richard Wright on 5/27/18.
//  Starstone Software Systems, Inc.

#ifndef StarlightFocuser_hpp
#define StarlightFocuser_hpp

#include <stdio.h>
#include <hidapi.h>

#define MAX_FOCUSER_STRING      128

#define SL_MOTOR_NOP    0
#define SL_MOTOR_IN     1
#define SL_MOTOR_OUT    2
#define SL_MOTOR_GOTO   3
#define SL_MOTOR_SETPOS 4
#define SL_MOTOR_SETMAX 5
#define SL_MOTOR_HALT   0xff


#define SL_MOTOR_MOVING_NOT     0
#define SL_MOTOR_MOVING_IN      1
#define SL_MOTOR_MOVING_OUT     2
#define SL_MOTOR_MOVING_GOTO    3
#define SL_MOTOR_MOVING_LOCKED  5

class StarlightFocuser
    {
    public:
        StarlightFocuser(void);
        ~StarlightFocuser(void);
        
        bool Connect(void);
        void Disconnect(void);
        inline bool IsConnected(void) { return _deviceHandle != nullptr; }
        
        int getFirmware(unsigned char &major, unsigned char& minor);
        
        bool readTemp(double& dTemp);
        
        int  getMotorState(unsigned int& nState);
        int  getMotorPosition(unsigned int& nPosition);
        int  gotoPosition(unsigned int nPosition);
        
        int  getMotorPolarity(unsigned int& nState);
        int  setMotorPolarity(unsigned int nState);
        
        int  saveMotorPosition(int nOverride = -1);
        
        int  getMotorOffOnIdle(unsigned int& nState);
        int  setMotorOffOnIdle(unsigned int nState);
        
        int  getTempCompEnabled(unsigned int& nState);
        int  setTempCompEnabled(unsigned int nState);

        int  getStepPeriod(unsigned char& nDelay);
        int  setStepPeriod(unsigned char nDelay);

        int getBacklashSettings(unsigned char& nSteps, unsigned char& inOut);
        int setBacklashSettings(unsigned char nSteps, unsigned char inOut);

        int getMaxPosition(unsigned int& nPos);
        int setMaxPosition(unsigned int nPos);
        
        int getHandsetDirection(unsigned int& nDir);
        int setHandsetDirection(unsigned int nDir);
        
        int abortMove(void);

		bool hasMaxSetting(void);
        
    protected:
        hid_device*     _deviceHandle;
        wchar_t         cManufacturer[MAX_FOCUSER_STRING];
        wchar_t         cProductName[MAX_FOCUSER_STRING];
        unsigned char   firmwareMinor;
        unsigned char   firmwareMajor;
    
        int SendReceive(unsigned char cPayload[5]);
    
    
    };


#endif /* StarlightFocuser_hpp */
