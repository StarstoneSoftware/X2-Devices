#include <string.h>
#include <stdio.h>
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
#include "../../../../licensedinterfaces/x2guiinterface.h"

#include "../../../../rotatorplugins/NightCrawler/BuildEnv/Src/x2rotator.h"

#define X2_FOC_NAM "MoonLight NiteCrawler Focuser"
// 1.01 Correct rotation conversion.
// 1.02 Corrected temperature readout
#define DRIVER_VERSION      1.02


NightCrawlerFocuserPlugIn::NightCrawlerFocuserPlugIn(const char* pszDisplayName,
												const int& nInstanceIndex,
												SerXInterface						* pSerX, 
												TheSkyXFacadeForDriversInterface	* pTheSkyX, 
												SleeperInterface					* pSleeper,
												BasicIniUtilInterface				* pIniUtil,
												LoggerInterface						* pLogger,
												MutexInterface						* pIOMutex,
												TickCountInterface					* pTickCount)
: m_nPrivateISIndex(nInstanceIndex)

    {
	m_pSerX							= pSerX;		
	m_pTheSkyXForMounts				= pTheSkyX;
	m_pSleeper						= pSleeper;
	m_pIniUtil						= pIniUtil;
	m_pLogger						= pLogger;	
	m_pIOMutex						= pIOMutex;
	m_pTickCount					= pTickCount;
	m_pSavedMutex					= pIOMutex;
	m_pSavedSerX					= pSerX;
    
	m_nInstanceIndex = nInstanceIndex;
	m_bLinked = false;
    
    // Read in settings
    if (m_pIniUtil)
        m_pIniUtil->readString(PARENT_KEY_STRING, SERIAL_NAME, "", serialName, MAX_SERIAL_PATH);
        
    
    nFocuserPositon = 0;
    bIsAuxFocuser = (strstr(pszDisplayName, "NightCrawler Auxiliary Focuser") != 0);
    }

NightCrawlerFocuserPlugIn::~NightCrawlerFocuserPlugIn()
{
	//Delete objects used through composition
	if (m_pSavedSerX)
		delete m_pSavedSerX;
	if (m_pSavedMutex)
		delete m_pSavedMutex;

	if (GetTheSkyXFacadeForDrivers())
		delete GetTheSkyXFacadeForDrivers();
	if (GetSleeper())
		delete GetSleeper();
	if (GetBasicIniUtil())
		delete GetBasicIniUtil();
	if (GetLogger())
		delete GetLogger();
}

int	NightCrawlerFocuserPlugIn::queryAbstraction(const char* pszName, void** ppVal)
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
    else if (!strcmp(pszName, SerialPortParams2Interface_Name))
        *ppVal = dynamic_cast<SerialPortParams2Interface*>(this);
	else if (!strcmp(pszName, MultiConnectionDeviceInterface_Name))
		*ppVal = dynamic_cast<MultiConnectionDeviceInterface*>(this);

	return SB_OK;
}

///////////////////////////////////////////////////////////////////////////
// expectedBytes is basically non zero if a value is returned. It is the maximum number of bytes allowed.
int NightCrawlerFocuserPlugIn::WriteCommand(const char *szCommand, char *cResponse, unsigned long expectedBytes) const
    {
    unsigned long writeBytes = strlen(szCommand);
    unsigned long wroteBytes = 0;
    if(cResponse != NULL)
        strncpy(cResponse, "BadFOOD", expectedBytes);

	// Always start with a clean slate... $%#@ happens.
	m_pSerX->purgeTxRx();

    // Write command, check for error
    m_pSerX->writeFile((void *)szCommand, writeBytes, wroteBytes);
	m_pSerX->flushTx();
    if(writeBytes != wroteBytes)
        return ERR_DATAOUT;
 
        
    // read result, check for error
    unsigned long readBytes = expectedBytes;
    int nTimeout = 5000;    // 3 seconds
    if(strcmp(szCommand, "SH#") == 0)
       nTimeout = 3600000; // 10 Minutes for home

	// Wait for at least one byte to show up back. The device will
	// always acknowledge with a #
    readBytes = 0;
    if(0 != m_pSerX->waitForBytesRx(1, nTimeout)) {
       printf("Timed out on command: %s\r\n", szCommand);
       return ERR_RXTIMEOUT;
       }
            
	char byte = 0;
	do {
       unsigned long rb = 0;
       if(SB_OK != m_pSerX->readFile(&byte, 1, rb) || rb != 1)
           return ERR_RXTIMEOUT;
            
       // Don't go past end of buffer
       if(readBytes < expectedBytes && cResponse != nullptr)
           cResponse[readBytes] = byte;
            
       readBytes++;
       } while(byte != '#');


	// Always NULL terminate the response
	if(cResponse != nullptr) {
        cResponse[readBytes] = NULL;
        if(readBytes > 0)
            cResponse[readBytes-1] = NULL;
        
        if(strncmp(cResponse, "NACK", 4) == 0)
            return ERR_UNKNOWNCMD;
        }

    return SB_OK;
    }

//! [useResource]
int NightCrawlerFocuserPlugIn::useResource(MultiConnectionDeviceInterface *pPeer)
    {
	NightCrawlerRotatorPlugIn *pRotatorPeer = dynamic_cast<NightCrawlerRotatorPlugIn*>(pPeer);
	if (pRotatorPeer == NULL)
        return ERR_POINTER;
        
											   // Use the resources held by the specified peer
	m_pIOMutex = pRotatorPeer->m_pSavedMutex;
	m_pSerX = pRotatorPeer->m_pSavedSerX;
	return SB_OK;
    }
//! [useResource]

//! [swapResource]
int NightCrawlerFocuserPlugIn::swapResource(MultiConnectionDeviceInterface *pPeer)
    {
	NightCrawlerRotatorPlugIn *pRotatorPeer = dynamic_cast<NightCrawlerRotatorPlugIn*>(pPeer);
	if (pRotatorPeer == NULL) return ERR_POINTER;
											   // Swap this driver instance's resources for the ones held by pPeer
	MutexInterface* pTempMutex = m_pSavedMutex;
	SerXInterface*  pTempSerX = m_pSavedSerX;
	m_pSavedMutex = pRotatorPeer->m_pSavedMutex;
	m_pSavedSerX = pRotatorPeer->m_pSavedSerX;
	pRotatorPeer->m_pSavedMutex = pTempMutex;
	pRotatorPeer->m_pSavedSerX = pTempSerX;

	return SB_OK;
}
//! [swapResource]


//! [isConnectionPossible]
// Much simpler for the gemini case.
int NightCrawlerFocuserPlugIn::isConnectionPossible(const int &nPeerArraySize, MultiConnectionDeviceInterface **ppPeerArray, bool &bConnectionPossible)
    {
		for (int nIndex = 0; nIndex < nPeerArraySize; ++nIndex)
		{
			NightCrawlerRotatorPlugIn *pPeer = dynamic_cast<NightCrawlerRotatorPlugIn*>(ppPeerArray[nIndex]);
			if (pPeer == NULL)
                {
				bConnectionPossible = false;
				return ERR_POINTER;
                }

		}

	bConnectionPossible = true;
	return SB_OK;
    }

//! [isConnectionPossible]

////////////////////////////////////////////////////////////////////////////
// Return the firmware version as a string. Assignments to str are safe because
// the equal operator is overloaded to copy the data.
void NightCrawlerFocuserPlugIn::deviceInfoFirmwareVersion(BasicStringInterface& str)
{
	X2MutexLocker ml(GetMutex());

	if (!isLinked())
		str = "Unknown";
	else
		str = cFirmware;
}

void NightCrawlerFocuserPlugIn::deviceInfoModel(BasicStringInterface& str)
{
	X2MutexLocker ml(GetMutex());

    if(m_bLinked)
        str = cFocuserType;
    else
        str = "Not connected";
}

/////////////////////////////////////////////////////////////////
// Are we currently connected? Rather than actually test this,
// we'll return the cached value.
bool NightCrawlerFocuserPlugIn::isLinked(void) const
    {
	return m_bLinked;
    }


void NightCrawlerFocuserPlugIn::driverInfoDetailedInfo(BasicStringInterface& str) const
    {
	str = "Software Bisque Native";
    }

double NightCrawlerFocuserPlugIn::driverInfoVersion(void) const
{
	return DRIVER_VERSION;
}

void NightCrawlerFocuserPlugIn::deviceInfoNameShort(BasicStringInterface& str) const
{
	str = "NiteCrawler Focuser"; // I don't think this one actually shows up anywhere
}

void NightCrawlerFocuserPlugIn::deviceInfoNameLong(BasicStringInterface& str) const
{
	str = X2_FOC_NAM;
}

void NightCrawlerFocuserPlugIn::deviceInfoDetailedDescription(BasicStringInterface& str) const
{
    str = "Focuser";
}


///////////////////////////////////////////////////////////////////////////
// Establish a link to the device.
int	NightCrawlerFocuserPlugIn::establishLink(void)
    {
	m_bLinked = false;
	X2MutexLocker ml(GetMutex());
    int nErr = SB_OK;
    
	if (!m_pSerX->isConnected())
		if (SB_OK != m_pSerX->open(serialName, baudRate()))
    		return ERR_NOLINK;
        
    const char* cmdGetType = "PF#";
    if(SB_OK != (nErr = WriteCommand(cmdGetType,cFocuserType, 7)))
        return nErr;

    const char* cmdGetFirmware = "PV#";
    if(SB_OK != (nErr = WriteCommand(cmdGetFirmware, cFirmware, 3)))
        return nErr;
    
    const char* cmdGetSerialNumber = "PS#";
    if(SB_OK != (nErr = WriteCommand(cmdGetSerialNumber, cSerialNumber, 6)))
        return ERR_CMDFAILED;
    
	//We made it. Now we are connected and shall report as such.
	m_bLinked = true;
    return nErr;
    }

/////////////////////////////////////////////////////////////////
// Break the conneciton with the device
int	NightCrawlerFocuserPlugIn::terminateLink(void)
    {
 	X2MutexLocker ml(GetMutex());

	if (m_pSerX && m_nInstanceCount == 1)
	{
		if (m_pSerX->isConnected())
			m_pSerX->close();
	}

	// We're not connected, so revert to our saved interfaces
	m_pSerX = m_pSavedSerX;
	m_pIOMutex = m_pSavedMutex;

	m_bLinked = false;
	return SB_OK;
    }




///////////////////////////////////////////////////////////////
// Get the current position of the focuser. On the very first
// call, we query the actual postion. Thereafter we return
// the cached position of the focuser, which is returned after
// every other operation.
int	NightCrawlerFocuserPlugIn::focPosition(int& nPosition)
    {
	X2MutexLocker ml(GetMutex());
    int nErr = SB_OK;
    
	if (!m_bLinked)
		return ERR_NOLINK;
    
    char cResponse[16];
    const char* cmdGetPosition = focusCommand("GP#");
    if(SB_OK != (nErr = WriteCommand(cmdGetPosition, cResponse, 8)))
        return nErr;

    iLastPositon = atoi(cResponse);
    nPosition = iLastPositon;
	return nErr;
    }


//////////////////////////////////////////////////////
// The minimum limit of the focuser. For Rotofocus, this is alwasy 2
int	NightCrawlerFocuserPlugIn::focMinimumLimit(int& nMinLimit)
    {
	X2MutexLocker ml(GetMutex());

    // What is the smallest value for the focuser
	nMinLimit = 0;
	return SB_OK;
    }

//////////////////////////////////////////////////////
// The maximum limit of the focuser. For Robofocus, this is
// in the 65,000's, but may be lower. The individual focuser
// must be calibrated (using Robofocus's manual routine),
// and the maximum limit of the focuser is then stored in
// the device.
int	NightCrawlerFocuserPlugIn::focMaximumLimit(int& nMaxLimit)
    {
    X2MutexLocker ml(GetMutex());

	if (!m_bLinked)
		return ERR_NOLINK;

	nMaxLimit = 95399;
    return SB_OK;
    }

///////////////////////////////////////////////////////////
int	NightCrawlerFocuserPlugIn::focAbort()
    {
    // This we let through
	X2MutexLocker ml(GetMutex());
    int nErr = SB_OK;

	if (!m_bLinked)
		return ERR_NOLINK;
	
    const char* cmdStopMotors = focusCommand("SQ#");
    if(SB_OK != (nErr = WriteCommand(cmdStopMotors)))
        return nErr;

	return nErr;
    }

/////////////////////////////////////////////////////////////////
// Begin a goto positon. This function blocks until the move is
// complete, but this is not strictly required by X2.
int	NightCrawlerFocuserPlugIn::startFocGoto(const int& nRelativeOffset)
    {
	X2MutexLocker ml(GetMutex());
    int nErr = SB_OK;
    
    if(!m_bLinked)
        return ERR_NOLINK;
        
    int iWhereToGo = iLastPositon + nRelativeOffset;
    char cmdGoThere[16];
    int iMotor = (bIsAuxFocuser) ? 3 : 1;
    sprintf(cmdGoThere, "%dSN %08d#", iMotor, iWhereToGo);
    if(SB_OK != (nErr = WriteCommand(cmdGoThere)))
        return nErr;
        
    const char* cmdStart = focusCommand("SM#");
    if(SB_OK != (nErr = WriteCommand(cmdStart)))
        return nErr;

    return nErr;
    }

/////////////////////////////////////////////////////////////////
// See above. If move is synchronous, then you know you're done
// when this get's called. If move is asynchronous, then this
// function needs to tell if the movement is complete.
// This function is called by the GUI thread, not the hardware thread...
// synchronize your device access...
int	NightCrawlerFocuserPlugIn::isCompleteFocGoto(bool& bComplete) const
    {
	X2MutexLocker ml(((NightCrawlerFocuserPlugIn*)this)->GetMutex());
    int nErr = SB_OK;
    
	if (!m_bLinked)
		return ERR_NOLINK;

    int iMotor = (bIsAuxFocuser) ? 3 : 1;
    char cmd[32];
    sprintf(cmd, "%dGM#", iMotor);
    char cResponse[8];
    if(SB_OK != (nErr = WriteCommand(cmd, cResponse, 3)))
        return nErr;
    
    bComplete = (cResponse[1] == '0');

	return nErr;
    }

//
int	NightCrawlerFocuserPlugIn::endFocGoto(void)
    {
    X2MutexLocker ml(GetMutex());

	return SB_OK;
    }


int NightCrawlerFocuserPlugIn::focTemperature(double &dTemperature)
{
	X2MutexLocker ml(GetMutex());
    int nErr = SB_OK;
    
    // Don't query for auxilary focuser
    if(bIsAuxFocuser)
        return ERR_NOT_IMPL;

	if (!m_bLinked)
        {
		dTemperature = -100.0;
		return SB_OK;
        }
    
    const char* cmdGetTemperature = "GT#";
    char cResponse[8];
    if(SB_OK != (nErr = WriteCommand(cmdGetTemperature, cResponse, 4)))
        return nErr;

    cResponse[4] = NULL;
    int t = atoi(cResponse);
    dTemperature = double(t) * 0.1f;

	return nErr;
}

int NightCrawlerFocuserPlugIn::amountCountFocGoto(void) const
    {
	return 4;
    }

int	NightCrawlerFocuserPlugIn::amountNameFromIndexFocGoto(const int& nZeroBasedIndex, BasicStringInterface& strDisplayName, int& nAmount)

{
	X2MutexLocker ml(GetMutex());

	switch (nZeroBasedIndex)
	{
		default:
		case 0: strDisplayName="Tiny (10 step)";		nAmount=10;break;
		case 1: strDisplayName="Small (100 steps)";		nAmount=100;break;
		case 2: strDisplayName="Medium (1000 steps)";	nAmount=1000;break;
		case 3: strDisplayName="Large (10000 steps)";	nAmount=10000;break;
	}
	return SB_OK;
}

int	NightCrawlerFocuserPlugIn::amountIndexFocGoto(void)
{
	X2MutexLocker ml(GetMutex());

	return 0;
}



int NightCrawlerFocuserPlugIn::execModalSettingsDialog()
{
	X2MutexLocker ml(GetMutex());

	int nErr = SB_OK;
	X2ModalUIUtil uiutil(this, GetTheSkyXFacadeForDrivers());
	X2GUIInterface*					ui = uiutil.X2UI();
	X2GUIExchangeInterface*			dx = NULL;//Comes after ui is loaded
	bool bPressedOK = false;
    
    char cOutBuffer[32];
    char cResponse[16];
    int nVoltage = 0;
    int nBrightnessOn = 0;
    int nBrightnessSleep = 0;
    int nStepDelay = 0;
    
	if (NULL == ui)
		return ERR_POINTER;

	if ((nErr = ui->loadUserInterface("NCFocuser.ui", deviceType(), m_nPrivateISIndex)))
		return nErr;

	if (NULL == (dx = uiutil.X2DX()))
		return ERR_POINTER;
    
    if(!m_bLinked) {
        dx->setEnabled("groupBox_System", false);
        dx->setEnabled("groupBox_Focuser", false);
        }
    else
        {
        // These are read from the device.
        // Voltage
        if(SB_OK != (nErr = WriteCommand("GV#", cResponse, 4)))
            return nErr;
            
        cResponse[3] = NULL;
        nVoltage = atoi(cResponse);
        sprintf(cOutBuffer, "%.1f v", float(nVoltage)*0.1f);
        dx->setText("label_Voltage", cOutBuffer);
        
        // Inverted display?
        WriteCommand("C 20#", cResponse, 8);
        if(strncmp(cResponse, "01", 2) == 0)
            dx->setChecked("checkBox", true);

        // Locked knobs?
        WriteCommand("PE#", cResponse, 8);
        if(strncmp(cResponse, "00", 2) == 0)
            dx->setChecked("checkBox_2", true);
        
        // Display brightness
        WriteCommand("PD#", cResponse, 8);
        dx->setPropertyInt("horizontalSlider_1", "value", atoi(cResponse));
    
        // Sleep brightness
        WriteCommand("PL#", cResponse, 8);
        dx->setPropertyInt("horizontalSlider_2", "value", atoi(cResponse));

        // Temperature offset
        WriteCommand("Pt#", cResponse, 8);
            
        int tempOffset;
        sscanf(cResponse, "%x", &tempOffset);
        if(tempOffset > 32768)
            tempOffset -= 65536;
            
        double fTempOffset = double(tempOffset) * 0.1;
        dx->setPropertyDouble("doubleSpinBox_Temp", "value", fTempOffset);
        
        // Step Delay
        if(SB_OK != (nErr = WriteCommand("1GR#", cResponse, 4)))
            return nErr;
        
        cResponse[3] = NULL;
        nStepDelay = atoi(cResponse);
        dx->setPropertyInt("spinBox_MotorStep", "value", nStepDelay);
        
        // These have been read previously
        dx->setText("label_SN", cSerialNumber);
        }
    
	//Display the user interface
	if((nErr = ui->exec(bPressedOK)))
		return nErr;

	//Retrieve values from the user interface
	if (bPressedOK && m_bLinked)
        {
        dx->propertyInt("horizontalSlider_Brightness", "value", nBrightnessOn);
        dx->propertyInt("horizontalSlider_SleepBrightness", "value", nBrightnessSleep);
        double dTempOffset = 0.0;
        dx->propertyDouble("doubleSpinBox_Temp", "value", dTempOffset);
        int nMotorStep;
        dx->propertyInt("spinBox_MotorStep", "value", nMotorStep);
            
        // Write remaining values to device
        
        // Motor step values
        char cCmdOut[32];
        sprintf(cCmdOut, "1SR %3d#", nMotorStep);
        if(SB_OK != (nErr = WriteCommand(cCmdOut)))
            return nErr;
            
        // Tempterature offset
        sprintf(cCmdOut, "Pt %03d#", int(dTempOffset * 10.0));
        
        if(SB_OK != (nErr = WriteCommand(cCmdOut))) 
            return nErr;
        }

	return nErr;
}

void NightCrawlerFocuserPlugIn::uiEvent(X2GUIExchangeInterface* uiex, const char* pszEvent)
    {
	X2MutexLocker ml(GetMutex());

    char cCmdOut[32];

//    printf("Received an event: %s\r\n", pszEvent);
    
    /////////////////////////////////////////////////////// HOME Focuser
    if(strcmp(pszEvent, "on_pushButton_3_clicked") == 0) {
        const char *szCommand = "SH 01#";
        WriteCommand(szCommand);
        return;
        }
    
    /////////////////////////////////////////////////////// HOME Rotator
    if(strcmp(pszEvent, "on_pushButton_4_clicked") == 0) {
        const char *szCommand = "SH 02#";
        WriteCommand(szCommand);
        return;
        }

    /////////////////////////////////////////////////////// HOME AUX
    if(strcmp(pszEvent, "on_pushButton_5_clicked") == 0) {
        const char *szCommand = "SH 04#";
        WriteCommand(szCommand);
        return;
        }

    /////////////////////////////////////////////////////// Flip display
    if(strcmp(pszEvent, "on_checkBox_stateChanged") == 0) {
        // Invert display?
        if(uiex->isChecked("checkBox")) {
            strcpy(cCmdOut, "C 20 01#");
            WriteCommand(cCmdOut);
            }
        else {
            strcpy(cCmdOut, "C 20 00#");
            WriteCommand(cCmdOut);
            }
        
        return;
        }
        
    /////////////////////////////////////////////////////// Lock knobs
    if(strcmp(pszEvent, "on_checkBox_2_stateChanged") == 0) {
        // Invert display?
        if(uiex->isChecked("checkBox_2"))
            WriteCommand("PE 00#");
        else
            WriteCommand("PE 01#");
        
        return;
        }
        
    //////////////////////////////////////////////////////// Sliders
    if(strcmp(pszEvent, "on_timer") == 0) {
        int nBrightness, nSleepBrightness;
        uiex->propertyInt("horizontalSlider_1", "value", nBrightness);
        uiex->propertyInt("horizontalSlider_2", "value", nSleepBrightness);
    
        sprintf(cCmdOut, "PD %03d#", nBrightness);
        WriteCommand(cCmdOut);
        
        sprintf(cCmdOut, "PL %03d#", nSleepBrightness);
        WriteCommand(cCmdOut);
        }
    	 
    }




