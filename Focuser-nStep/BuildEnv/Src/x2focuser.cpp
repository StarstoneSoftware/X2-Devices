#include <string.h>
#include "x2focuser.h"

#ifdef SB_MAC_BUILD
#include <stdbool.h>
#endif
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>


#include "../../../../licensedinterfaces/theskyxfacadefordriversinterface.h"
#include "../../../../licensedinterfaces/sleeperinterface.h"
#include "../../../../licensedinterfaces/loggerinterface.h"
#include "../../../../licensedinterfaces/basiciniutilinterface.h"
#include "../../../../licensedinterfaces/mutexinterface.h"
#include "../../../../licensedinterfaces/basicstringinterface.h"
#include "../../../../licensedinterfaces/tickcountinterface.h"
#include "../../../../licensedinterfaces/serxinterface.h"
#include "../../../../licensedinterfaces/sberrorx.h"
#include "StopWatch.h"

#define PARENT_KEY_STRING  "NSTEPX2"
#define NSTEP_SERIAL_NAME  "SERIALPORT"
#define NSTEP_STEP_MODE    "STEPMODE"
#define NSTEP_BACKLASH     "BACKLASH"
#define NSTEP_REVERSE      "REVERSE_DIR"

#define X2_FOC_NAM "X2 nStep"
#define DRIVER_VERSION      1.03

// 1.02 March 8, 2017
// When focuser position returns 0, requery at least once.


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
	m_nPosition = 0;
    fTempOffset = -5.0f;
    fLastTemp = -1000.0f;

    // Read in settings
    if (m_pIniUtil) {
        m_pIniUtil->readString(PARENT_KEY_STRING, NSTEP_SERIAL_NAME,  "", serialName, 255);
        nStepMode = m_pIniUtil->readInt(PARENT_KEY_STRING, NSTEP_STEP_MODE, 2);
        nBackLash = -m_pIniUtil->readInt(PARENT_KEY_STRING, NSTEP_BACKLASH, 0);
        nOrientation = m_pIniUtil->readInt(PARENT_KEY_STRING, NSTEP_REVERSE, 1);
        }
    
    nFWWiringPhase = 0;
    nFWStepRate = 1;
    nFWMaxStepSpeed = 1;
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

	return ERR_NOT_IMPL;
}

void X2Focuser::driverInfoDetailedInfo(BasicStringInterface& str) const
    {
        str = "nStep Focuser Control System";
    }

double X2Focuser::driverInfoVersion(void) const
    {
	return DRIVER_VERSION;
    }

void X2Focuser::deviceInfoNameShort(BasicStringInterface& str) const
    {
	str = X2_FOC_NAM;
    }

void X2Focuser::deviceInfoNameLong(BasicStringInterface& str) const
    {
	str = X2_FOC_NAM;
    }

void X2Focuser::deviceInfoDetailedDescription(BasicStringInterface& str) const
    {
	str = "nStep X2 Bisque Native";
    }

////////////////////////////////////////////////////////////////////////////
// Return the firmware version as a string. Assignments to str are safe because
// the equal operator is overloaded to copy the data.
void X2Focuser::deviceInfoFirmwareVersion(BasicStringInterface& str)				
    {
    str = "Not Available";
    }
void X2Focuser::deviceInfoModel(BasicStringInterface& str)							
{
    str = "nStep";
}

bool X2Focuser::WriteCommand(const char *szCommand, char *cResponse, unsigned long expectedBytes) const
    {
    unsigned long writeBytes = strlen(szCommand);
    unsigned long wroteBytes;
    
    m_pSerX->purgeTxRx();
	m_pSleeper->sleep(10);
    m_pSerX->writeFile((void*)szCommand, writeBytes, wroteBytes);
    m_pSerX->flushTx();
	if (writeBytes != wroteBytes) {
		printf("nStep: Command %s failed\n", szCommand);
		return false;
		}

	// Again, I come across a focuser controller that only on
	// windows seems to need a moment to collect itself after writing a command.
	m_pSleeper->sleep(500);

    // Some commands have a response, some do not
    if(expectedBytes != 0 && cResponse != NULL) {
        unsigned long readBytes;
		if (0 != m_pSerX->readFile((void*)cResponse, expectedBytes, readBytes)) {
			//printf("nStep: No response from command %s\n", szCommand);
			return false;
			}
        
        cResponse[readBytes] = NULL;
        }
        
    return true;
    }

///////////////////////////////////////////////////////////////////////////
// Establish a link to the device.
int	X2Focuser::establishLink(void)
    {
	X2MutexLocker ml(GetMutex());

	if(0 != m_pSerX->open(serialName,9600)) {
		printf("Failed to open port %s\r\n", serialName);
		m_bLinked = false;
		return ERR_NOLINK;
		}
        
    // Max steps/second
    char cReturn[32];
    char cBuff[32] = ":RS";
    if(!WriteCommand(cBuff, cReturn, 3))
        return ERR_CMDFAILED;
    else
        nFWMaxStepSpeed = atoi(cReturn);
        
	m_pSleeper->sleep(100);	// Give time to recover

    // Step rate
    strcpy(cBuff, ":RO");
    if(!WriteCommand(cBuff, cReturn, 3))
        return ERR_CMDFAILED;
    else
        nFWStepRate = atoi(cReturn);

	m_pSleeper->sleep(100);	// Give time to recover

    // Phase
    strcpy(cBuff, ":RW");
    if(!WriteCommand(cBuff, cReturn, 3))
        return ERR_CMDFAILED;
    else
        nFWWiringPhase = atoi(cReturn);

	m_pSleeper->sleep(100);	// Give time to recover

    // Energize coils?
    strcpy(cBuff, ":RC");
    if(!WriteCommand(cBuff, cReturn, 1))
        return ERR_CMDFAILED;
    else
        bFWEnergized = (cReturn[0] == '0'); // Yes, zero is keep on, 1 is turn off

	m_pSleeper->sleep(100);	// Give time to recover

    m_bLinked = true;
	return SB_OK;
    }

/////////////////////////////////////////////////////////////////
// Break the conneciton with the device
int	X2Focuser::terminateLink(void)
    {
	X2MutexLocker ml(GetMutex());
	m_pSerX->close();

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


//////////////////////////////////////////////////////
// The minimum limit of the focuser. For Robofocus, this is alwasy 2
int	X2Focuser::focMinimumLimit(int& nMinLimit) 		
    {
	nMinLimit = -65000;
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
    nMaxLimit = 65000;
    return SB_OK;
    }

///////////////////////////////////////////////////////////
int	X2Focuser::focAbort()
    {
    // This we let through
	X2MutexLocker ml(GetMutex());
    
	char szCommand[32];
    if(nLastMove*nOrientation < 0)
        sprintf(szCommand, ":F0%d%03d#", nStepMode, 0);
	else
        sprintf(szCommand, ":F1%d%03d#",nStepMode, 0);
    
    if(!WriteCommand(szCommand, NULL, 0))
        return ERR_CMDFAILED;
	
	return SB_OK;
    }

/////////////////////////////////////////////////////////////////
// Quickie sign detection
inline int sign(int x) {
    return (x > 0) - (x < 0);
}

/////////////////////////////////////////////////////////////////
int	X2Focuser::startFocGoto(const int& nRelativeOffset)
    {
	X2MutexLocker ml(GetMutex());

    if(!m_bLinked)
        return ERR_NOLINK;

    int nOffset = nRelativeOffset * nOrientation;
    
    // The direction we are now moving
	nLastMove = nRelativeOffset;
    
    // If we are moving in the backlash direction, add the backlash to the offset
    if(sign(nOffset) == sign(nBackLash*nOrientation))
        nOffset += (nBackLash*nOrientation);
        
    // We can't move more than 999 steps at a time
    if(nOffset > 999)
        nOffset = 999;
        
    if(nOffset < -999)
        nOffset = -999;
        
	char szCommand[32];
    if(nOffset < 0)
        sprintf(szCommand, ":F0%d%03d#", nStepMode, -nOffset);
	
    if(nOffset > 0)
        sprintf(szCommand, ":F1%d%03d#",nStepMode, nOffset);
    
    if(!WriteCommand(szCommand, NULL, 0))
        return ERR_CMDFAILED;
        
	return SB_OK;
    }


///////////////////////////////////////////////////////////////
// Get the current position of the focuser.
int	X2Focuser::focPosition(int& nPosition) 			
    {
	static int iLastPosition = 0;
	X2MutexLocker ml(GetMutex());

    if(!m_bLinked)
        return ERR_NOLINK;

    const char *szCmd = "#:RP";
    char cResponse[16];
    if(WriteCommand(szCmd, cResponse, 7)) {
        nPosition = atoi(cResponse) *nOrientation;
        iLastPosition = nPosition;
        }
	else 
		nPosition = iLastPosition;	// Occasional timeouts, just return last position (yeah, cheating)
		
    // Le-Sigh... it seems if the position is queried too soon after a move, it returns zero. In testing
    // it alwasy works the 2nd time. The drawback to this is that if the position actually IS zero, it will query
    // it twice, but that's not going to happen very often, and it's harmless.
    if(iLastPosition == 0) {
        m_pSleeper->sleep(150);
//        printf("nStep reported zero last position\n");
        focPosition(nPosition);
        iLastPosition = nPosition;
//        printf("2nd try reports %d\n", nPosition);
        }
        
    // If we are moving against the backlash direction, return the actual position. If moving in the backlash direction
    // offset the value by the backlash
    nPosition *= nOrientation;

	return SB_OK;
    }

////////////////////////////////////////////////////////////
int	X2Focuser::isCompleteFocGoto(bool& bComplete) const
    {
    char buff[32];
    char cResponse[32];
    sprintf(buff,"S");
    if(!WriteCommand(buff, cResponse, 1))
        return ERR_CMDFAILED;
    
    if(cResponse[0] == '0')
        bComplete = true;
    else
        bComplete = false;
    
    // If we are moving in the direction of backlash, we've overshot, and
    // now we need to backup
    int nRetries = 0;
    if(bComplete && (sign(nLastMove*nOrientation) == sign(nBackLash*nOrientation)))
        {
        int nOffset = -nBackLash * nOrientation;
//        printf("Performing backlash adjustment of %d\n", nOffset);
    	char szCommand[32];
        if(nOffset < 0)
            sprintf(szCommand, ":F0%d%03d#", nStepMode, -nOffset);
	
        if(nOffset > 0)
            sprintf(szCommand, ":F1%d%03d#",nStepMode, nOffset);
    
        if(!WriteCommand(szCommand, NULL, 0))
            return ERR_CMDFAILED;
        
        // Now we have to wait for the backlash movement to complete
        do {
            m_pSleeper->sleep(200);
            
            if(!WriteCommand(buff, cResponse, 1))
                return ERR_CMDFAILED;
    
            nRetries++;
            } while (cResponse[0] != '0' && (nRetries < 20));  // 10 seconds?
        }

    if(nRetries == 20)
        return ERR_RXTIMEOUT;
    
    return SB_OK;
    }

//
int	X2Focuser::endFocGoto(void)							
    {
	return SB_OK;
    }


int X2Focuser::focTemperature(double &dTemperature)
    {
    static bool bFirstTime = true;
    X2MutexLocker ml(GetMutex());
    
    if(!m_bLinked)
        return ERR_NOLINK;
        
    ///////////////////////////////////////////////////
    // It seems, TheSkyX is calling this every single time it moves
    // the motor. This is a significant delay in the responsiveness of clicking
    // the in or out buttons when focusing manually. I'm going to slow this down, and only allow temperature
    // readings once per minute, which ought to be sufficient.
    static CStopWatch timer;
    if(timer.GetElapsedSeconds() > 20.0f || bFirstTime) {
        timer.Reset();
        bFirstTime = false;
        const char *szCmd = ":RT";
        char cResponse[16];
        if(!WriteCommand(szCmd, cResponse, 4)) {    // This failure may be benign...
            dTemperature = fLastTemp;
            return SB_OK;
            }

        //printf("Temperature read: ->%s<-\n", cResponse);
        if(atoi(cResponse) == -888) {   // Report last/default temperature
            dTemperature = fLastTemp;            
            return SB_OK;
            }
        
        float temp = atoi(cResponse);
        temp *= 0.1;
        fLastTemp = temp;// + 275.0f;
        dTemperature = fLastTemp;
        }
        
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
		case 3: strDisplayName="Max (999 steps)";	nAmount=999;break;
	}
	return SB_OK;
}

int	X2Focuser::amountIndexFocGoto(void)
{
	return 0;
}


void X2Focuser::findDevices(X2GUIExchangeInterface *dx)
	{
	dx->invokeMethod("deviceList","clear");

    serialDevices.FindSerialDevices();

	// No devices were found!
    int nSelected = 0;
	if (serialDevices.GetNumDevices() == 0)
		dx->comboBoxAppendString("deviceList","No serial devices found!");
    else
        {
        for(int i = 0; i < serialDevices.GetNumDevices(); i++)  {
            dx->comboBoxAppendString("deviceList", serialDevices.GetDeviceName(i));
            
            if(strcmp(serialName, serialDevices.GetDeviceName(i)) == 0)
                nSelected = i;
            }

        // If the last selected device is in the list, select it
        dx->setCurrentIndex("deviceList", nSelected);
        }
	}



int X2Focuser::execModalSettingsDialog()
{
	int nErr = SB_OK;
	X2ModalUIUtil uiutil(this, GetTheSkyXFacadeForDrivers());
	X2GUIInterface*					ui = uiutil.X2UI();
	X2GUIExchangeInterface*			dx = NULL;//Comes after ui is loaded
	bool bPressedOK = false;

	if (NULL == ui)
		return ERR_POINTER;

	if ((nErr = ui->loadUserInterface("nStep.ui", deviceType(), m_nPrivateISIndex)))
		return nErr;

	if (NULL == (dx = uiutil.X2DX()))
		return ERR_POINTER;

	findDevices(dx);

    if(!m_bLinked) {
        // Disable the controls
        dx->setEnabled("groupBox_4", false);
        }
    else
        {
        dx->setPropertyInt("spinBox_Backlash", "value", nBackLash);    // Always positive
        dx->setChecked("checkBox_Reverse", nOrientation < 0);
        
        dx->setEnabled("groupBox_4", true);
        dx->setChecked("checkBox_energized", bFWEnergized);

        // Step modd
        dx->comboBoxAppendString("comboBox","Wave");  // 0
        dx->comboBoxAppendString("comboBox","Half Alternate");  // 1
        dx->comboBoxAppendString("comboBox","Full Torque");  // 2
        dx->setCurrentIndex("comboBox", nStepMode);
        
	
        // Displayed only
        if(fLastTemp < -99.0f)
            dx->setPropertyString("label_9", "text", "Prob not detected.");
        else {
            float tempC = fLastTemp;
            float tempF = (tempC * 1.8f) + 32.0f;
            char cBuffer[64];
            sprintf(cBuffer, "%.1fC (%.1fF)", tempC, tempF);
            dx->setPropertyString("label_9", "text", cBuffer);
            }
            
        // Saved in firmware
        // Fill in controls with current values from firmware
        dx->setPropertyInt("spinBox_StepSpeed", "value", nFWMaxStepSpeed);
        dx->setPropertyInt("spinBox_StepRate", "value", nFWStepRate);
        dx->setPropertyInt("spinBox_WirePhase", "value", nFWWiringPhase);
		}


	//Display the user interface
	if((nErr = ui->exec(bPressedOK)))
		return nErr;

	//Retrieve values from the user interface
	if (bPressedOK)
        {
        // Get the name of the serial device
        int index = dx->currentIndex("deviceList");
        if(serialDevices.GetNumDevices() > 0) {
            strcpy(serialName, serialDevices.GetDeviceName(index));

            // Save configuration to ini file
            if (m_pIniUtil)
                m_pIniUtil->writeString(PARENT_KEY_STRING, NSTEP_SERIAL_NAME,  serialName);
            }

		// If linked, write configuration settings to device
		if(m_bLinked) {
            // Step mode is set in .INI file
            nStepMode = dx->currentIndex("comboBox");
            if (m_pIniUtil)
                m_pIniUtil->writeInt(PARENT_KEY_STRING, NSTEP_STEP_MODE, nStepMode);

            // Get values that are to be saved to firmware
            dx->propertyInt("spinBox_StepSpeed", "value", nFWMaxStepSpeed);
            dx->propertyInt("spinBox_StepRate", "value", nFWStepRate);
            dx->propertyInt("spinBox_WirePhase", "value", nFWWiringPhase);
            
            dx->propertyInt("spinBox_Backlash", "value", nBackLash);
            if(dx->isChecked("checkBox_Reverse"))
                nOrientation = -1;
            else
                nOrientation = 1;
            
            // Write them to the firmware
            char buf[32];
            sprintf(buf,":CS%03d#", nFWMaxStepSpeed);
            if(!WriteCommand(buf, NULL, 0))
                return ERR_CMDFAILED;

			m_pSleeper->sleep(200);// XTra time for device recovery

            sprintf(buf,":CO%03d#", nFWStepRate);
            if(!WriteCommand(buf, NULL, 0))
                return ERR_CMDFAILED;

			m_pSleeper->sleep(200);// XTra time for device recovery

            sprintf(buf,":CW%01d#", nFWWiringPhase);
            if(!WriteCommand(buf, NULL, 0))
                return ERR_CMDFAILED;

			m_pSleeper->sleep(200);// XTra time for device recovery

			bFWEnergized = dx->isChecked("checkBox_energized");
            if(bFWEnergized)
                strcpy(buf, ":CC0");
            else
                strcpy(buf, ":CC1");
            
            if(!WriteCommand(buf, NULL, 0))
               return ERR_CMDFAILED;
			
			m_pSleeper->sleep(200);// XTra time for device recovery

            // This option is not persistent.
            if(dx->isChecked("checkBox_Reset")) {
                sprintf(buf,"#:CP+000000#");
                if(!WriteCommand(buf, NULL, 0))
                    return ERR_CMDFAILED;

				m_pSleeper->sleep(200);// XTra time for device recovery
                }

            m_pIniUtil->writeInt(PARENT_KEY_STRING, NSTEP_BACKLASH, -nBackLash);
            m_pIniUtil->writeInt(PARENT_KEY_STRING, NSTEP_REVERSE, nOrientation);
            }

        }

	return nErr;
}



void X2Focuser::uiEvent(X2GUIExchangeInterface* uiex, const char* pszEvent)
    {
	if (!strcmp(pszEvent, "on_pushButton_clicked"))
		 findDevices(uiex);
    }

