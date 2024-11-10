#include <string.h>
#include "x2focuser.h"

#include "../../../../licensedinterfaces/theskyxfacadefordriversinterface.h"
#include "../../../../licensedinterfaces/sleeperinterface.h"
#include "../../../../licensedinterfaces/loggerinterface.h"
#include "../../../../licensedinterfaces/basiciniutilinterface.h"
#include "../../../../licensedinterfaces/mutexinterface.h"
#include "../../../../licensedinterfaces/basicstringinterface.h"
#include "../../../../licensedinterfaces/tickcountinterface.h"
#include "../../../../licensedinterfaces/serxinterface.h"
#include "../../../../licensedinterfaces/sberrorx.h"

#define PARENT_KEY_STRING   "STARLIGHTINSTRUMENTSX2"

#define DRIVER_VERSION      1.01

// Version 1.0  - 6/27/2018     Ready to rock... let it loose!

X2Focuser::X2Focuser(const char* pszDisplayName, 
												const int& nInstanceIndex,
												SerXInterface						* pSerXIn, 
												TheSkyXFacadeForDriversInterface	* pTheSkyXIn, 
												SleeperInterface					* pSleeperIn,
												BasicIniUtilInterface				* pIniUtilIn,
												LoggerInterface						* pLoggerIn,
												MutexInterface						* pIOMutexIn,
												TickCountInterface					* pTickCountIn)
: m_nPrivateISIndex(nInstanceIndex)

{
	m_pSerX							= pSerXIn;		
	m_pTheSkyXForMounts				= pTheSkyXIn;
	m_pSleeper						= pSleeperIn;
	m_pIniUtil						= pIniUtilIn;
	m_pLogger						= pLoggerIn;	
	m_pIOMutex						= pIOMutexIn;
	m_pTickCount					= pTickCountIn;

	m_bLinked = false;
    fLastTemp = -100.0f;
    minPosition = 0;
    maxPosition = 1048575;    
}

X2Focuser::~X2Focuser()
{
	//Delete objects used through composition
	if (GetSerX())
		delete GetSerX();
	if (GetTheSkyXFacadeForDrivers())
		delete GetTheSkyXFacadeForDrivers();
	if (GetSleeper())
		delete GetSleeper();
	if (GetSimpleIniUtil())
		delete GetSimpleIniUtil();
	if (GetLogger())
		delete GetLogger();
	if (GetMutex())
		delete GetMutex();
}

int	X2Focuser::queryAbstraction(const char* pszName, void** ppVal)
{
	*ppVal = NULL;

	if (!strcmp(pszName, LinkInterface_Name))
		*ppVal = (LinkInterface*)this;
	else if (!strcmp(pszName, FocuserGotoInterface2_Name))
		*ppVal = (FocuserGotoInterface2*)this;
	else if (!strcmp(pszName, ModalSettingsDialogInterface_Name))
		*ppVal = dynamic_cast<ModalSettingsDialogInterface*>(this);
	else if (!strcmp(pszName, X2GUIEventInterface_Name)) 
		*ppVal = dynamic_cast<X2GUIEventInterface*>(this);
    else if (!strcmp(pszName, FocuserTemperatureInterface_Name))
        *ppVal = dynamic_cast<FocuserTemperatureInterface*>(this);

	return SB_OK;
}

void X2Focuser::driverInfoDetailedInfo(BasicStringInterface& str) const
    {
        str = "Bisque X2 Native";
    }

double X2Focuser::driverInfoVersion(void) const
    {
	return  DRIVER_VERSION;
    }

void X2Focuser::deviceInfoNameShort(BasicStringInterface& str) const
    {
	str = "Starlgiht Instruments";
    }

void X2Focuser::deviceInfoNameLong(BasicStringInterface& str) const
    {
	str = "Starlight Instruments Focus Controller";
    }

void X2Focuser::deviceInfoDetailedDescription(BasicStringInterface& str) const
    {
	str = "Focus Controller";
    }

////////////////////////////////////////////////////////////////////////////
// Return the firmware version as a string. Assignments to str are safe because
// the equal operator is overloaded to copy the data.
void X2Focuser::deviceInfoFirmwareVersion(BasicStringInterface& str)				
    {    
	X2MutexLocker ml(GetMutex());

    if(focuser.IsConnected()) {
        char cBuffer[16];
        unsigned char major, minor;
        focuser.getFirmware(major, minor);
        sprintf(cBuffer, "%d.%d", major, minor);
        str = cBuffer;
        return;
        }
        
    str = "Device Not Connected.";
    }
    
void X2Focuser::deviceInfoModel(BasicStringInterface& str)							
    {
    str = "Starlight Instruments";
    }

///////////////////////////////////////////////////////////////////////////
// Establish a link to the device.
int	X2Focuser::establishLink(void)
    {
	X2MutexLocker ml(GetMutex());
	m_bLinked = false;

    if(!focuser.Connect())
        return ERR_NOLINK;

    // Read maximum location setting
	if (focuser.hasMaxSetting())
		if (focuser.getMaxPosition(maxPosition) <= 0) 
			return ERR_CMDFAILED;
    
    m_bLinked = true;
	return SB_OK;
    }

/////////////////////////////////////////////////////////////////
// Break the conneciton with the device
int	X2Focuser::terminateLink(void)
    {
	X2MutexLocker ml(GetMutex());
    focuser.Disconnect();
    
	m_bLinked = false;
	return SB_OK;
    }


/////////////////////////////////////////////////////////////////
// Are we currently connected? Rather than actually test this,
// we'll return the cached value.
bool X2Focuser::isLinked(void) const
    {
	return m_bLinked;
    }


///////////////////////////////////////////////////////////////
// Get the current position of the focuser. On the very first
// call, we query the actual postion. Thereafter we return
// the cached position of the focuser, which is returned after
// every other operation.
int	X2Focuser::focPosition(int& nPosition) 			
    {
	X2MutexLocker ml(GetMutex());

    if(!m_bLinked)
        return ERR_NOLINK;
        
    unsigned int pos;
    int nRet = focuser.getMotorPosition(pos);
    if(nRet == 0)
        return ERR_MKS_COMM_TIMEOUT;
        
    if(nRet == -1)
        return ERR_CMDFAILED;
        
    nPosition = pos;
    return SB_OK;
    }


//////////////////////////////////////////////////////
// The minimum limit of the focuser. For Robofocus, this is alwasy 2
int	X2Focuser::focMinimumLimit(int& nMinLimit) 		
    {
	nMinLimit = minPosition;
	return SB_OK;
    }

//////////////////////////////////////////////////////
// The maximum limit of the focuser. For Robofocus, this is
// in the 65,000's, but may be lower. The individual focuser
// must be calibrated (using Robofocus's manual routine),
// and the maximum limit of the focuser is then stored in
// the device.
int	X2Focuser::focMaximumLimit(int& nMaxLimit)			
    {
    X2MutexLocker ml(GetMutex());
    nMaxLimit = maxPosition;
    return SB_OK;
    }

///////////////////////////////////////////////////////////
// Any serial i/o operation aborts the current move. In this
// case only, we forgo the mutex and let the command go through
// If another thread is "moving", it will error out, and return
// prematurely. The moves on the Robofocus class are synchronous
// so care is taken to check for errors on the move command, and
// to requery the current postion before returning.
int	X2Focuser::focAbort()								
    {
	X2MutexLocker ml(GetMutex());
    focuser.abortMove();
	return SB_OK;
    }

/////////////////////////////////////////////////////////////////
// Begin a goto positon. This function blocks until the move is
// complete, but this is not strictly required by X2.
int	X2Focuser::startFocGoto(const int& nRelativeOffset)	
    {
	X2MutexLocker ml(GetMutex());

    if(!m_bLinked)
        return ERR_NOLINK;
        
    unsigned int nLastPos;
    focuser.getMotorPosition(nLastPos);
    
    int newPos = nLastPos + nRelativeOffset;
    if(newPos < 0)
        return ERR_LIMITSEXCEEDED;
        
    nLastPos = newPos;
    focuser.gotoPosition(nLastPos);
    
    return SB_OK;
    }

/////////////////////////////////////////////////////////////////
// See above. If move is synchronous, then you know you're done
// when this get's called. If move is asynchronous, then this
// function needs to tell if the movement is complete.
int	X2Focuser::isCompleteFocGoto(bool& bComplete) const	
    {
    X2MutexLocker ml(GetMutex());

    // By the time we get here, we are done
    //printf("RSW: Asked if focus was complete\r\n");
    unsigned int nState = SL_MOTOR_HALT;
	bComplete = false;
    X2Focuser *pMe = (X2Focuser*)this;
    int nRet = pMe->focuser.getMotorState(nState);
    if(nRet == 0)
        return ERR_MKS_COMM_TIMEOUT;
        
    if(nRet == -1)
        return ERR_CMDFAILED;
        
    if(nState != SL_MOTOR_MOVING_NOT) 
        return SB_OK;
        
    bComplete = true;
	return SB_OK;
    }

//
int	X2Focuser::endFocGoto(void)							
    {
    //printf("RSW: endFocGoto called. NOP really.\r\n");
	return SB_OK;
    }


int X2Focuser::focTemperature(double &dTemperature)
    {
    X2MutexLocker ml(GetMutex());

    if(!m_bLinked)
        {
        dTemperature = -100.0;
        return SB_OK;
        }
        
    int nRet = focuser.readTemp(dTemperature);
    if(nRet == 0)
        return ERR_MKS_COMM_TIMEOUT;
        
    if(nRet == -1)
        return ERR_NOT_IMPL;

    return SB_OK;
    }

int X2Focuser::amountCountFocGoto(void) const					
    {
	return 4;
    }

int	X2Focuser::amountNameFromIndexFocGoto(const int& nZeroBasedIndex, BasicStringInterface& strDisplayName, int& nAmount)
{
	switch (nZeroBasedIndex)
	{
		default:
		case 0: strDisplayName="Tiny (1 step)";			nAmount=1;break;
		case 1: strDisplayName="Small (10 steps)";		nAmount=10;break;
		case 2: strDisplayName="Medium (100 steps)";	nAmount=100;break;
		case 3: strDisplayName="Large (1000 steps)";	nAmount=1000;break;
	}
	return SB_OK;
}

int	X2Focuser::amountIndexFocGoto(void)
{
	return 0;
}



int X2Focuser::execModalSettingsDialog()
{
	X2MutexLocker ml(GetMutex());
	int nErr = SB_OK;
	X2ModalUIUtil uiutil(this, GetTheSkyXFacadeForDrivers());
	X2GUIInterface*					ui = uiutil.X2UI();
	X2GUIExchangeInterface*			dx = NULL;//Comes after ui is loaded
	bool bPressedOK = false;
    int nCurrentPosition;

	if (NULL == ui)
		return ERR_POINTER;

	if ((nErr = ui->loadUserInterface("StarlightInstruments.ui", deviceType(), m_nPrivateISIndex)))
		return nErr;

	if (NULL == (dx = uiutil.X2DX()))
		return ERR_POINTER;

    // If not connected, everything is gray and you can't do squat...
    if(!m_bLinked) {
        dx->setEnabled("groupBox", false);

        //Display the user interface
        return ui->exec(bPressedOK);
        }

    // We are connected, set GUI based on readings from controller
    unsigned int nState;
    if(focuser.getMotorPolarity(nState) < 0)
        return ERR_CMDFAILED;
        
    dx->setChecked("checkBoxReverse", nState);
    
    // Idle state
    if(focuser.getMotorOffOnIdle(nState) < 0)
        return ERR_CMDFAILED;
        
    dx->setChecked("checkBoxIdle", nState);

    // Override position
    if(focuser.getMotorPosition(nState) < 0)
        return ERR_CMDFAILED;
        
    nCurrentPosition = nState;
    dx->setPropertyInt("spinBoxOverride", "value", nCurrentPosition);
    
    // Step Period
    unsigned char period;
    if(focuser.getStepPeriod(period) < 0)
        return ERR_CMDFAILED;
    else
        dx->setChecked("checkBoxPeriodFaster", (period < 12));
    
    // Handset polarity
    unsigned int nHandDir;
    if(focuser.getHandsetDirection(nHandDir) < 0)
        return ERR_CMDFAILED;
    else    
        dx->setChecked("checkBoxRevHand", nHandDir);
    
	// Backlash
    unsigned char nSteps, inOut;
    if(focuser.getBacklashSettings(nSteps, inOut) < 0)
        return ERR_CMDFAILED;
        
    dx->setPropertyInt("backSpin", "value", nSteps);
    if(inOut == 1) 
        dx->setChecked("radioButton_IN", true);
    else
        dx->setChecked("radioButton_OUT", true);

	if (!focuser.hasMaxSetting()) {
		dx->setPropertyInt("labelMaxPosition", "visible", 0);
		dx->setPropertyInt("spinBoxMaxPosition", "visible", 0);
		}
	else
		dx->setPropertyInt("spinBoxMaxPosition", "value", maxPosition);

	//Display the user interface
	if((nErr = ui->exec(bPressedOK)))
		return nErr;

	//Retrieve values from the user interface
	if (bPressedOK) // Write everything out
        {
        int nValue; // temp variable

		// Don't reset max position if firmware is too old
		if (focuser.hasMaxSetting()) {
			int nNewMaxPos;
			dx->propertyInt("spinBoxMaxPosition", "value", nNewMaxPos);
			// Has it changed?
			if (nNewMaxPos != maxPosition) {
				// Also, don't allow chaning to smaller value than present
				if (nNewMaxPos >= nCurrentPosition) {
					focuser.setMaxPosition(nNewMaxPos);
					maxPosition = nNewMaxPos;
					}
				}
			}

        // Reverse motor polarity
        nValue = dx->isChecked("checkBoxReverse");
        if(focuser.setMotorPolarity(nValue) < 0)
            return ERR_CMDFAILED;
            
        // Handset polarity
        nHandDir = dx->isChecked("checkBoxRevHand");
        if(focuser.setHandsetDirection(nHandDir) < 0)
            return ERR_CMDFAILED;

        // Hold motor
        nValue = dx->isChecked("checkBoxIdle");
        if(focuser.setMotorOffOnIdle(nValue) < 0)
            return ERR_CMDFAILED;

        // step period
        if(dx->isChecked("checkBoxPeriodFaster"))
            focuser.setStepPeriod(8);
        else
            focuser.setStepPeriod(16);
                        
        // Backlash
        dx->propertyInt("backSpin", "value", nValue);
        bool out = (bool)dx->isChecked("radioButton_IN");
        if(focuser.setBacklashSettings(nValue, out) < 0)
            return ERR_CMDFAILED;
  
        // Reset current position. Only write to firmware if it's been changed...
        dx->propertyInt("spinBoxOverride", "value", nValue);
        if(nValue != nCurrentPosition) {
            unsigned int nNewPos = nValue;
            if(focuser.saveMotorPosition(nNewPos) < 0)
                return ERR_CMDFAILED;
            }
        }

	return nErr;
}



void X2Focuser::uiEvent(X2GUIExchangeInterface* uiex, const char* pszEvent)
    {
//    if (!strcmp(pszEvent, "on_pushButton_clicked"))
//         findDevices(uiex);    
    }

